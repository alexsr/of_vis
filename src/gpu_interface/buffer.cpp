//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "buffer.hpp"

tostf::Buffer::Buffer(const GLenum target, const gl::storage flags) : _target(target), _storage_flags(flags) {
    glCreateBuffers(1, &_name);
    _dynamically_updatable = _storage_flags & gl::storage::dynamic && _storage_flags & gl::storage::write;
    _readable = _storage_flags & gl::storage::read;
    _storage_initialized = false;
}

tostf::Buffer::Buffer(Buffer&& other) noexcept {
    _name = other._name;
    _target = other._target;
    _storage_flags = other._storage_flags;
    _dynamically_updatable = other._dynamically_updatable;
    _readable = other._readable;
    _storage_initialized = other._storage_initialized;
    _buffer_size = other._buffer_size;
    other._name = 0;
}

tostf::Buffer& tostf::Buffer::operator=(Buffer&& other) noexcept {
    _name = other._name;
    _target = other._target;
    _storage_flags = other._storage_flags;
    _dynamically_updatable = other._dynamically_updatable;
    _readable = other._readable;
    _storage_initialized = other._storage_initialized;
    _buffer_size = other._buffer_size;
    other._name = 0;
    return *this;
}

tostf::Buffer::~Buffer() {
    if (glIsBuffer(_name)) {
        unbind();
        glDeleteBuffers(1, &_name);
    }
}

GLuint tostf::Buffer::get_name() const {
    return _name;
}

unsigned int tostf::Buffer::get_buffer_size() const {
    return _buffer_size;
}

void tostf::Buffer::bind() const {
    glBindBuffer(_target, _name);
}

void tostf::Buffer::unbind() const {
    glBindBuffer(_target, 0);
}

bool tostf::Buffer::is_initialized() const {
    return _storage_initialized;
}

void tostf::Buffer::initialize_empty_storage(const unsigned int size) {
    if (_storage_initialized) {
        throw std::runtime_error{"Buffer " + std::to_string(_name) + " was initialized already."};
    }
    _buffer_size = size;
    glNamedBufferStorage(_name, _buffer_size, nullptr, to_underlying_type(_storage_flags));
    _storage_initialized = true;
}
