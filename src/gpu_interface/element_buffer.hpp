//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "buffer.hpp"

namespace tostf
{
    // CPU representation of GL_ELEMENT_ARRAY_BUFFER. Exclusively stores unsigned ints.
    class Element_buffer : public Buffer {
    public:
        explicit Element_buffer(gl::storage flags = gl::storage::dynamic | gl::storage::write);
        template <typename T, typename = std::enable_if_t<is_container_v<T> && std::is_unsigned_v<typename T::value_type>>>
        explicit Element_buffer(const T& indices, gl::storage flags = gl::storage::dynamic | gl::storage::write);
        template <typename T, typename = std::enable_if_t<is_container_v<T> && std::is_unsigned_v<typename T::value_type>>>
        void set_data(const T& data);
        unsigned long get_count() const;
    private:
        unsigned long _count = 0; // count of indices in buffer
    };

    template <typename T, typename>
    Element_buffer::Element_buffer(const T& indices, const gl::storage flags)
        : Buffer(GL_ELEMENT_ARRAY_BUFFER, indices, flags) {
        _count = static_cast<unsigned int>(indices.size());
    }

    template <typename T, typename>
    void Element_buffer::set_data(const T& data) {
        Buffer::set_data(data);
        _count = static_cast<int>(data.size());
    }
}
