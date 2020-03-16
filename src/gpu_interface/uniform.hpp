//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "math/glm_helper.hpp"
#include "utility/logging.hpp"
#include <glad/glad.h>
#include <vector>
#include <typeinfo>

namespace tostf
{
    // A CPU representation of a shader uniform with specific type T
    // storing its shader uniform location and providing update functionality.
    class Uniform {
    public:
        explicit Uniform(GLint location);
        template <typename T>
        void update(const T& value, GLuint program) const;
        // Update the buffer's data from the CPU if the buffer flags include
        // both gl::storage::dynamic and gl::storage::write.
        template <typename T>
        void update(const std::vector<T>& values, GLuint program) const;
    private:
        GLint _location;
    };

    inline Uniform::Uniform(const GLint location) : _location(location) {}

    template <typename T>
    void Uniform::update(const T& value, const GLuint program) const {
        log_error() << "Uniform type " << typeid(T).name() << "does not exist.\n";
    }

    template <>
    inline void Uniform::update(const GLuint64& value, const GLuint program) const {
        glProgramUniformHandleui64ARB(program, _location, value);
    }

    template <>
    inline void Uniform::update(const std::vector<GLuint64>& values, const GLuint program) const {
        glProgramUniformHandleui64vARB(program, _location, static_cast<GLsizei>(values.size()), values.data());
    }

    template <>
    inline void Uniform::update(const bool& value, const GLuint program) const {
        glProgramUniform1i(program, _location, value);
    }

    template <>
    inline void Uniform::update(const int& value, const GLuint program) const {
        glProgramUniform1i(program, _location, value);
    }

    template <>
    inline void Uniform::update(const glm::ivec2& value, const GLuint program) const {
        glProgramUniform2i(program, _location, value.x, value.y);
    }

    template <>
    inline void Uniform::update(const glm::ivec3& value, const GLuint program) const {
        glProgramUniform3i(program, _location, value.x, value.y, value.z);
    }

    template <>
    inline void Uniform::update(const glm::ivec4& value, const GLuint program) const {
        glProgramUniform4i(program, _location, value.x, value.y, value.z, value.w);
    }

    template <>
    inline void Uniform::update(const glm::uint& value, const GLuint program) const {
        glProgramUniform1ui(program, _location, value);
    }

    template <>
    inline void Uniform::update(const glm::uvec2& value, const GLuint program) const {
        glProgramUniform2ui(program, _location, value.x, value.y);
    }

    template <>
    inline void Uniform::update(const glm::uvec3& value, const GLuint program) const {
        glProgramUniform3ui(program, _location, value.x, value.y, value.z);
    }

    template <>
    inline void Uniform::update(const glm::uvec4& value, const GLuint program) const {
        glProgramUniform4ui(program, _location, value.x, value.y, value.z, value.w);
    }

    template <>
    inline void Uniform::update(const float& value, const GLuint program) const {
        glProgramUniform1f(program, _location, value);
    }

    template <>
    inline void Uniform::update(const glm::vec2& value, const GLuint program) const {
        glProgramUniform2f(program, _location, value.x, value.y);
    }

    template <>
    inline void Uniform::update(const glm::vec3& value, const GLuint program) const {
        glProgramUniform3f(program, _location, value.x, value.y, value.z);
    }

    template <>
    inline void Uniform::update(const glm::vec4& value, const GLuint program) const {
        glProgramUniform4f(program, _location, value.x, value.y, value.z, value.w);
    }

    template <>
    inline void Uniform::update(const glm::mat2& value, const GLuint program) const {
        glProgramUniformMatrix2fv(program, _location, 1, GL_FALSE, value_ptr(value));
    }

    template <>
    inline void Uniform::update(const glm::mat3& value, const GLuint program) const {
        glProgramUniformMatrix3fv(program, _location, 1, GL_FALSE, value_ptr(value));
    }

    template <>
    inline void Uniform::update(const glm::mat4& value, const GLuint program) const {
        glProgramUniformMatrix4fv(program, _location, 1, GL_FALSE, value_ptr(value));
    }

    template <typename T>
    void Uniform::update(const std::vector<T>& values, const GLuint program) const {
        log_error() << "Uniform type " << typeid(T).name() << "does not exist.\n";
    }

    template <>
    inline void Uniform::update(const std::vector<int>& values, const GLuint program) const {
        glProgramUniform1iv(program, _location, static_cast<GLsizei>(values.size()), values.data());
    }

    template <>
    inline void Uniform::update(const std::vector<glm::ivec2>& values, const GLuint program) const {
        glProgramUniform2iv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::ivec3>& values, const GLuint program) const {
        glProgramUniform3iv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::ivec4>& values, const GLuint program) const {
        glProgramUniform4iv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::uint>& values, const GLuint program) const {
        glProgramUniform1uiv(program, _location, static_cast<GLsizei>(values.size()), values.data());
    }

    template <>
    inline void Uniform::update(const std::vector<glm::uvec2>& values, const GLuint program) const {
        glProgramUniform2uiv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::uvec3>& values, const GLuint program) const {
        glProgramUniform3uiv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::uvec4>& values, const GLuint program) const {
        glProgramUniform4uiv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<float>& values, const GLuint program) const {
        glProgramUniform1fv(program, _location, static_cast<GLsizei>(values.size()), values.data());
    }

    template <>
    inline void Uniform::update(const std::vector<glm::vec2>& values, const GLuint program) const {
        glProgramUniform2fv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::vec3>& values, const GLuint program) const {
        glProgramUniform3fv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::vec4>& values, const GLuint program) const {
        glProgramUniform4fv(program, _location, static_cast<GLsizei>(values.size()), value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::mat2>& values, const GLuint program) const {
        glProgramUniformMatrix2fv(program, _location, 1, GL_FALSE, value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::mat3>& values, const GLuint program) const {
        glProgramUniformMatrix3fv(program, _location, 1, GL_FALSE, value_ptr(values.data()[0]));
    }

    template <>
    inline void Uniform::update(const std::vector<glm::mat4>& values, const GLuint program) const {
        glProgramUniformMatrix4fv(program, _location, 1, GL_FALSE, value_ptr(values.data()[0]));
    }
}
