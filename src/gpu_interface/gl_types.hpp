//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <glad/glad.h>
#include "math/glm_helper.hpp"

namespace tostf
{
    namespace gl
    {
        template <typename T>
        constexpr GLenum to_gl_type(const T&) { return GL_INVALID_ENUM; }

        template <typename T, size_t N, glm::precision P>
        GLenum to_gl_type(const glm::vec<N, T, P>&) {
            return to_gl_type(T());
        }

        template <>
        constexpr GLenum to_gl_type(const GLubyte&) { return GL_UNSIGNED_BYTE; }

        template <>
        constexpr GLenum to_gl_type(const GLbyte&) { return GL_BYTE; }

        template <>
        constexpr GLenum to_gl_type(const GLushort&) { return GL_UNSIGNED_SHORT; }

        template <>
        constexpr GLenum to_gl_type(const GLshort&) { return GL_SHORT; }

        template <>
        constexpr GLenum to_gl_type(const GLuint&) { return GL_UNSIGNED_INT; }

        template <>
        constexpr GLenum to_gl_type(const GLint&) { return GL_INT; }

        template <>
        constexpr GLenum to_gl_type(const GLfloat&) { return GL_FLOAT; }

        template <>
        constexpr GLenum to_gl_type(const GLdouble&) { return GL_DOUBLE; }

        template <typename T>
        constexpr size_t type_to_channels(const T&) {
            return 1;
        }

        template <typename T, size_t N, glm::precision P>
        constexpr size_t type_to_channels(const glm::vec<N, T, P>&) {
            return N;
        }
    }
}
