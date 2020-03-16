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
    // Base class for UBOs and SSBOs.
    class Storage_buffer : public Buffer {
    public:
        GLuint get_binding() const;
        void bind_base(GLuint binding);
    protected:
        explicit Storage_buffer(GLenum target, GLuint binding = 0,
                                gl::storage flags = gl::storage::dynamic | gl::storage::write);
        template <typename T>
        explicit Storage_buffer(GLenum target, GLuint binding, const T& data,
                                gl::storage flags = gl::storage::dynamic | gl::storage::write);
        GLuint _binding;
    };

    // A UBO or SSBO might be initialized without data, but constructor signatures necessitate
    // a different base class to prevent issues.
    class Empty_storage_buffer : public Storage_buffer {
    protected:
        explicit Empty_storage_buffer(unsigned int size, GLenum target, GLuint binding = 0,
                                      gl::storage flags = gl::storage::dynamic | gl::storage::write);
    };

    // CPU representation of 
    class UBO : public Storage_buffer {
    public:
        // Creates an uninitialized GL_UNIFORM_BUFFER.
        explicit UBO(GLuint binding = 0, gl::storage flags = gl::storage::dynamic | gl::storage::write);
        // Creates a GL_UNIFORM_BUFFER initialized with provided data.
        template <typename T>
        explicit UBO(GLuint binding, const T& data, gl::storage flags = gl::storage::dynamic | gl::storage::write);
    };

    class SSBO : public Storage_buffer {
    public:
        // Creates an uninitialized GL_SHADER_STORAGE_BUFFER.
        explicit SSBO(GLuint binding, gl::storage flags = gl::storage::dynamic | gl::storage::write);
        // Creates a GL_SHADER_STORAGE_BUFFER initialized with provided data.
        template <typename T>
        explicit SSBO(GLuint binding, const T& data, gl::storage flags = gl::storage::dynamic | gl::storage::write);
    };
}

template <typename T>
tostf::Storage_buffer::Storage_buffer(const GLenum target, const GLuint binding, const T& data, const gl::storage flags)
    : Buffer(target, data, flags), _binding(binding) {
    bind_base(binding);
}

template <typename T>
tostf::UBO::UBO(const GLuint binding, const T& data, const gl::storage flags)
    : Storage_buffer(GL_UNIFORM_BUFFER, binding, data, flags) {}

template <typename T>
tostf::SSBO::SSBO(const GLuint binding, const T& data, const gl::storage flags)
    : Storage_buffer(GL_SHADER_STORAGE_BUFFER, binding, data, flags) {}
