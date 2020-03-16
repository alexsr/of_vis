//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "gl.hpp"
#include <cstring>
#include <string>

namespace tostf
{
    // Buffer is the abstract main class for all types of OpenGL buffers providing all
    // necessary functionality.
    // The OpenGL buffer on the GPU can be either empty but of a certain size
    // or can hold the data provided in the constructor.
    // Data can be transfered both from the GPU to the CPU and from the CPU to the GPU
    // unless otherwise specified by the flags provided in the constructor.
    class Buffer {
    public:
        Buffer(const Buffer&) = default;
        Buffer& operator=(const Buffer&) = default;
        Buffer(Buffer&& other) noexcept;
        Buffer& operator=(Buffer&& other) noexcept;
        virtual ~Buffer();
        // Initialize the buffer's storage with the data provided.
        // If it already was initialized previously then simply update
        // the buffer's data from the CPU if the buffer flags include
        // both gl::storage::dynamic and gl::storage::write.
        template <typename T>
        void set_data(const T& data);
        
        template <typename T>
        std::enable_if_t<is_container_v<T>> set_data(const T& data, int offset);
        // Copy the data from the buffer to CPU memory provided by the data-reference.
        // This is only possible if the storage flag gl::storage::read is set.
        template <typename T>
        std::enable_if_t<!is_container_v<T>> get_data(T& data);
        // Copy the data from the buffer to CPU memory provided by the data-reference.
        // This is only possible if the storage flag gl::storage::read is set.
        template <typename T>
        std::enable_if_t<is_container_v<T>> get_data(T& data);
        GLuint get_name() const;
        unsigned int get_buffer_size() const;
        // Binds the buffer to its target.
        void bind() const;
        // Binds buffer 0 to the target of the buffer.
        void unbind() const;
        // Initializes storage of fixed size.
        void initialize_empty_storage(unsigned int size);
        bool is_initialized() const;
    protected:
        explicit Buffer(GLenum target, gl::storage flags = gl::storage::dynamic | gl::storage::write);
        template <typename T>
        explicit Buffer(GLenum target, const T& data, gl::storage flags = gl::storage::dynamic | gl::storage::write);
        // Initializes storage with provided data. This is possible regardless of the storage flags.
        template <typename T>
        std::enable_if_t<!is_container_v<T>> initialize_storage(const T& data);
        // Initializes storage with provided data. This is possible regardless of the storage flags.
        template <typename T>
        std::enable_if_t<is_container_v<T>> initialize_storage(const T& data);
        // Update the buffer's data from the CPU if the buffer flags include
        // both gl::storage::dynamic and gl::storage::write.
        template <typename T>
        std::enable_if_t<!is_container_v<T>> update_data(const T& data);
        // Update the buffer's data from the CPU if the buffer flags include
        // both gl::storage::dynamic and gl::storage::write.
        template <typename T>
        std::enable_if_t<is_container_v<T>> update_data(const T& data);
        // Update a certain chunk of the buffers data if the buffer flags include
        // both gl::storage::dynamic and gl::storage::write.
        template <typename T>
        std::enable_if_t<!is_container_v<T>> update_data(const T& data, int offset, int size);
        // Update a certain chunk of the buffers data if the buffer flags include
        // both gl::storage::dynamic and gl::storage::write.
        template <typename T>
        std::enable_if_t<is_container_v<T>> update_data(const T& data, int offset);
        template <typename T>
        void set_buffer_data(const T& data, int offset, unsigned int size);
        template <typename T>
        void get_buffer_data(T* data, int offset, size_t size);

        GLuint _name = 0;
        GLenum _target = 0;
        gl::storage _storage_flags;
        bool _dynamically_updatable = false;
        bool _readable = false;
        bool _storage_initialized = false;
        unsigned int _buffer_size = 0;
    };

    template <typename T>
    Buffer::Buffer(const GLenum target, const T& data, const gl::storage flags)
        : _target(target), _storage_flags(flags) {
        glCreateBuffers(1, &_name);
        initialize_storage(data);
        _dynamically_updatable = _storage_flags & gl::storage::dynamic && _storage_flags & gl::storage::write;
        _readable = _storage_flags & gl::storage::read;
    }

    template <typename T>
    std::enable_if_t<!is_container_v<T>> Buffer::update_data(const T& data) {
        set_buffer_data(&data, 0, _buffer_size);
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> Buffer::update_data(const T& data) {
        auto size = static_cast<unsigned>(data.size() * sizeof(T::value_type));
        if (size > _buffer_size) {
            size = _buffer_size;
        }
        set_buffer_data(data.data(), 0, size);
    }

    template <typename T>
    std::enable_if_t<!is_container_v<T>> Buffer::update_data(const T& data, int offset, int size) {
        if (offset < 0 || size < 0 || offset + size > _buffer_size) {
            throw std::runtime_error{
                "Offset or size are impractical to update buffer" + std::to_string(_name) + "."
            };
        }
        set_buffer_data(&data, offset, size);
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> Buffer::update_data(const T& data, int offset) {
        size_t size = data.size() * sizeof(T::value_type);
        if (offset < 0) {
            throw std::runtime_error{"Offset is impractical to update buffer" + std::to_string(_name) + "."};
        }
        // Prevent overflow access to uninitialized GPU data.
        if (offset + size > _buffer_size) {
            size = _buffer_size - offset;
        }
        set_buffer_data(data.data(), offset, size);
    }

    template <typename T>
    void Buffer::set_data(const T& data) {
        if (!_storage_initialized) {
            initialize_storage(data);
        }
        else {
            update_data(data);
        }
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> Buffer::set_data(const T& data, int offset) {
        if (!_storage_initialized) {
            initialize_storage(data);
        }
        else {
            update_data(data, offset);
        }        
    }

    template <typename T>
    std::enable_if_t<!is_container_v<T>> Buffer::get_data(T& data) {
        auto size = sizeof(data);
        if (size > _buffer_size) {
            size = _buffer_size;
        }
        get_buffer_data(&data, 0, size);
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> Buffer::get_data(T& data) {
        size_t size = data.size() * sizeof(T::value_type);
        if (size > _buffer_size) {
            size = _buffer_size;
        }
        get_buffer_data(data.data(), 0, size);
    }


    template <typename T>
    std::enable_if_t<!is_container_v<T>> Buffer::initialize_storage(const T& data) {
        if (_storage_initialized) {
            throw std::runtime_error{"Buffer " + std::to_string(_name) + " was initialized already."};
        }
        _buffer_size = sizeof(T);
        glNamedBufferStorage(_name, _buffer_size, &data, to_underlying_type(_storage_flags));
        _storage_initialized = true;
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> Buffer::initialize_storage(const T& data) {
        if (_storage_initialized) {
            throw std::runtime_error{"Buffer " + std::to_string(_name) + " was initialized already."};
        }
        _buffer_size = sizeof(T::value_type) * static_cast<unsigned int>(data.size());
        glNamedBufferStorage(_name, _buffer_size, data.data(), to_underlying_type(_storage_flags));
        _storage_initialized = true;
    }

    template <typename T>
    void Buffer::set_buffer_data(const T& data, const int offset, const unsigned int size) {
        if (!_dynamically_updatable) {
            throw std::runtime_error{"Buffer " + std::to_string(_name) + " is not dynamically updatable."};
        }
        const auto buffer_ptr = glMapNamedBufferRange(_name, offset, size,
                                                      to_underlying_type(gl::map_access::write
                                                                         | gl::map_access::invalidate_buffer));
        std::memcpy(buffer_ptr, data, static_cast<size_t>(size));
        glUnmapNamedBuffer(_name);
        glFinish();
    }

    template <typename T>
    void Buffer::get_buffer_data(T* data, const int offset, const size_t size) {
        if (!_readable) {
            throw std::runtime_error{"Buffer " + std::to_string(_name) + " is not readable."};
        }
        const auto buffer_ptr = glMapNamedBufferRange(_name, offset, size, to_underlying_type(gl::storage::read));
        std::memcpy(data, buffer_ptr, size);
        glUnmapNamedBuffer(_name);
        glFinish();
    }
}
