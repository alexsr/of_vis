//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "utility/utility.hpp"
#include <glad/glad.h>
#include <string>

namespace tostf
{
    namespace gl
    {
        inline std::string shader_type_to_string(const GLenum type) {
            switch (type) {
                case GL_VERTEX_SHADER:
                    return "vertex shader";
                case GL_FRAGMENT_SHADER:
                    return "fragment shader";
                case GL_GEOMETRY_SHADER:
                    return "geometry shader";
                case GL_TESS_CONTROL_SHADER:
                    return "tessellation control shader";
                case GL_TESS_EVALUATION_SHADER:
                    return "tessellation evaluation shader";
                case GL_COMPUTE_SHADER:
                    return "compute shader";
                default:
                    return "unknown type of shader";
            }
        }

        enum class introspection : GLbitfield {
            none = 1,
            basic = 2,
            ubo = 4,
            ssbo = 8
        };

        constexpr introspection operator|(const introspection i1, const introspection i2) {
            return static_cast<introspection>(to_underlying_type(i1) | to_underlying_type(i2));
        }

        constexpr bool operator&(const introspection i1, const introspection i2) {
            return (to_underlying_type(i1) & to_underlying_type(i2)) != 0;
        }
    }
}
