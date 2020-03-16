//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "vbo.hpp"

tostf::VBO::VBO(const GLint vertex_size, const gl::v_type type, const gl::storage flags)
    : Buffer(GL_ARRAY_BUFFER, flags), _vertex_size(vertex_size), _type(type) {
    _stride = static_cast<unsigned int>(size_of(_type)) * _vertex_size;
}

GLint tostf::VBO::get_vertex_size() const {
    return _vertex_size;
}

tostf::gl::v_type tostf::VBO::get_type() const {
    return _type;
}

size_t tostf::VBO::get_format_size() const {
    return size_of(_type);
}

unsigned int tostf::VBO::get_stride() const {
    return _stride;
}

void tostf::VBO::bind_as_ssbo(const GLuint binding) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, _name);
}

unsigned long tostf::VBO::get_count() const {
    return _buffer_size / _stride;
}
