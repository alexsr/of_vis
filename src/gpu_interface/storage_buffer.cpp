//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "storage_buffer.hpp"

tostf::Storage_buffer::Storage_buffer(const GLenum target, const GLuint binding, const gl::storage flags)
    : Buffer(target, flags), _binding(binding) {
    bind_base(binding);
}

GLuint tostf::Storage_buffer::get_binding() const {
    return _binding;
}

void tostf::Storage_buffer::bind_base(const GLuint binding) {
    _binding = binding;
    glBindBufferBase(_target, binding, _name);
}

tostf::UBO::UBO(const GLuint binding, const gl::storage flags)
    : Storage_buffer(GL_UNIFORM_BUFFER, binding, flags) {
}

tostf::SSBO::SSBO(const GLuint binding, const gl::storage flags)
    : Storage_buffer(GL_SHADER_STORAGE_BUFFER, binding, flags) {
}
