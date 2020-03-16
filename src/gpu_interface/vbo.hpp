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
    // CPU representation of GL_ARRAY_BUFFER.
    class VBO : public Buffer {
    public:
        explicit VBO(GLint vertex_size = 4, gl::v_type type = gl::v_type::flt,
                     gl::storage flags = gl::storage::dynamic | gl::storage::write);
        template <typename T>
        explicit VBO(const T& data, GLint vertex_size, gl::v_type type = gl::v_type::flt,
                     gl::storage flags = gl::storage::dynamic | gl::storage::write);
        GLint get_vertex_size() const;
        gl::v_type get_type() const;
        size_t get_format_size() const;
        unsigned int get_stride() const;
        void bind_as_ssbo(GLuint binding) const;
        unsigned long get_count() const;
    private:
        GLint _vertex_size;
        gl::v_type _type;
        unsigned int _stride;
    };

    template <typename T>
    VBO::VBO(const T& data, const GLint vertex_size, const gl::v_type type, const gl::storage flags)
        : Buffer(GL_ARRAY_BUFFER, data, flags), _vertex_size(vertex_size), _type(type) {
        _stride = static_cast<unsigned int>(size_of(_type)) * _vertex_size;
    }
}
