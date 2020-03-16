//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "gl.hpp"
#include "math/glm_helper.hpp"

namespace tostf
{
    using tex_1d_res = int;

    using tex_2d_res = glm::ivec2;

    using tex_res = glm::ivec3;

    namespace gl
    {
        enum class img_access : GLenum {
            read = GL_READ_ONLY,
            write = GL_WRITE_ONLY,
            read_write = GL_READ_WRITE
        };
    }

    namespace tex
    {
        enum class target : GLenum {
            tex_1d = GL_TEXTURE_1D,
            tex_2d = GL_TEXTURE_2D,
            tex_3d = GL_TEXTURE_3D,
            cube_map = GL_TEXTURE_CUBE_MAP,
            tex_1d_array = GL_TEXTURE_1D_ARRAY,
            tex_2d_array = GL_TEXTURE_2D_ARRAY,
            cube_map_array = GL_TEXTURE_CUBE_MAP_ARRAY
        };

        enum class format : GLenum {
            r = GL_RED,
            g = GL_GREEN,
            b = GL_BLUE,
            rg = GL_RG,
            rgb = GL_RGB,
            bgr = GL_BGR,
            rgba = GL_RGBA,
            bgra = GL_BGRA,
            r_int = GL_RED_INTEGER,
            g_int = GL_GREEN_INTEGER,
            b_int = GL_BLUE_INTEGER,
            rg_int = GL_RG_INTEGER,
            rgb_int = GL_RGB_INTEGER,
            bgr_int = GL_BGR_INTEGER,
            rgba_int = GL_RGBA_INTEGER,
            bgra_int = GL_BGRA_INTEGER,
            depth = GL_DEPTH_COMPONENT,
            stencil = GL_STENCIL_INDEX,
            depth_stencil = GL_DEPTH_STENCIL
        };

        constexpr bool is_integer_format(const format f) {
            switch (f) {
                case format::r_int:
                case format::g_int:
                case format::b_int:
                case format::rg_int:
                case format::rgb_int:
                case format::bgr_int:
                case format::rgba_int:
                case format::bgra_int:
                case format::stencil:
                    return true;
                default:
                    return false;
            }
        }

        constexpr int get_format_channels(const format f) {
            switch (f) {
                case format::r:
                case format::g:
                case format::b:
                case format::r_int:
                case format::g_int:
                case format::b_int:
                case format::depth:
                case format::stencil:
                case format::depth_stencil:
                    return 1;
                case format::rg:
                case format::rg_int:
                    return 2;
                case format::rgb:
                case format::bgr:
                case format::rgb_int:
                case format::bgr_int:
                    return 3;
                case format::rgba:
                case format::bgra:
                case format::rgba_int:
                case format::bgra_int:
                    return 4;
                default:
                    return 1;
            }
        }

        enum class type : GLenum {
            ubyte = GL_UNSIGNED_BYTE,
            byte = GL_BYTE,
            ushort = GL_UNSIGNED_SHORT,
            s = GL_SHORT,
            uint = GL_UNSIGNED_INT,
            i = GL_INT,
            half_f = GL_HALF_FLOAT,
            f = GL_FLOAT,
            ubyte_3_3_2 = GL_UNSIGNED_BYTE_3_3_2,
            ubyte_2_3_3_r = GL_UNSIGNED_BYTE_2_3_3_REV,
            ushort_5_6_5 = GL_UNSIGNED_SHORT_5_6_5,
            ushort_5_6_5_r = GL_UNSIGNED_SHORT_5_6_5_REV,
            ushort_4_4_4_4 = GL_UNSIGNED_SHORT_4_4_4_4,
            ushort_4_4_4_4_r = GL_UNSIGNED_SHORT_4_4_4_4_REV,
            ushort_5_5_5_1 = GL_UNSIGNED_SHORT_5_5_5_1,
            ushort_1_5_5_5_r = GL_UNSIGNED_SHORT_1_5_5_5_REV,
            uint_8_8_8_8 = GL_UNSIGNED_INT_8_8_8_8,
            uint_8_8_8_8_r = GL_UNSIGNED_INT_8_8_8_8_REV,
            uint_10_10_10_2 = GL_UNSIGNED_INT_10_10_10_2,
            uint_2_10_10_10_r = GL_UNSIGNED_INT_2_10_10_10_REV,
            uint_24_8 = GL_UNSIGNED_INT_24_8,
            uint_10f_11f_11f_r = GL_UNSIGNED_INT_10F_11F_11F_REV,
            uint_5_9_9_9_r = GL_UNSIGNED_INT_5_9_9_9_REV,
            f32_uint_24_8_r = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
        };

        enum class intern {
            r8 = GL_R8,
            r8_n = GL_R8_SNORM,
            r16 = GL_R16,
            r16_n = GL_R16_SNORM,
            r8i = GL_R8I,
            r8ui = GL_R8UI,
            r16i = GL_R16I,
            r16ui = GL_R16UI,
            r32i = GL_R32I,
            r32ui = GL_R32UI,
            r16f = GL_R16F,
            r32f = GL_R32F,
            rg8 = GL_RG8,
            rg8_n = GL_RG8_SNORM,
            rg16 = GL_RG16,
            rg16_n = GL_RG16_SNORM,
            rg8i = GL_RG8I,
            rg8ui = GL_RG8UI,
            rg16i = GL_RG16I,
            rg16ui = GL_RG16UI,
            rg32i = GL_RG32I,
            rg32ui = GL_RG32UI,
            rg16f = GL_RG16F,
            rg32f = GL_RG32F,
            r3_g3_b2 = GL_R3_G3_B2,
            rgb4 = GL_RGB4,
            rgb5 = GL_RGB5,
            rgb565 = GL_RGB565,
            rgb8 = GL_RGB8,
            rgb8_n = GL_RGB8_SNORM,
            rgb10 = GL_RGB10,
            rgb12 = GL_RGB12,
            rgb16 = GL_RGB16,
            rgb16_n = GL_RGB16_SNORM,
            srgb8 = GL_SRGB8,
            r11f_g11f_b10f = GL_R11F_G11F_B10F,
            rgb9_e5 = GL_RGB9_E5,
            rgb8i = GL_RGB8I,
            rgb8ui = GL_RGB8UI,
            rgb16i = GL_RGB16I,
            rgb16ui = GL_RGB16UI,
            rgb32i = GL_RGB32I,
            rgb32ui = GL_RGB32UI,
            rgb16f = GL_RGB16F,
            rgb32f = GL_RGB32F,
            rgba2 = GL_RGBA2,
            rgba4 = GL_RGBA4,
            rgb5_a1 = GL_RGB5_A1,
            rgba8 = GL_RGBA8,
            rgba8_n = GL_RGBA8_SNORM,
            rgb10_a2 = GL_RGB10_A2,
            rgb10_a2ui = GL_RGB10_A2UI,
            rgba12 = GL_RGBA12,
            rgba16 = GL_RGBA16,
            rgba16_n = GL_RGBA16_SNORM,
            srgb8_a8 = GL_SRGB8_ALPHA8,
            rgba8i = GL_RGBA8I,
            rgba8ui = GL_RGBA8UI,
            rgba16i = GL_RGBA16I,
            rgba16ui = GL_RGBA16UI,
            rgba32i = GL_RGBA32I,
            rgba32ui = GL_RGBA32UI,
            rgba16f = GL_RGBA16F,
            rgba32f = GL_RGBA32F,
            depth16 = GL_DEPTH_COMPONENT16,
            depth24 = GL_DEPTH_COMPONENT24,
            depth32 = GL_DEPTH_COMPONENT32,
            depth32f = GL_DEPTH_COMPONENT32F,
            stencil1 = GL_STENCIL_INDEX1,
            stencil4 = GL_STENCIL_INDEX4,
            stencil8 = GL_STENCIL_INDEX8,
            stencil16 = GL_STENCIL_INDEX16,
            depth24_stencil8 = GL_DEPTH24_STENCIL8,
            depth32f_stencil8 = GL_DEPTH32F_STENCIL8
        };

        constexpr format to_format(const intern internal_format) {
            switch (internal_format) {
                case intern::r8:
                case intern::r8_n:
                case intern::r16:
                case intern::r16_n:
                case intern::r16f:
                case intern::r32f:
                    return format::r;
                case intern::r8i:
                case intern::r8ui:
                case intern::r16i:
                case intern::r16ui:
                case intern::r32i:
                case intern::r32ui:
                    return format::r_int;
                case intern::rg8:
                case intern::rg8_n:
                case intern::rg16:
                case intern::rg16_n:
                case intern::rg16f:
                case intern::rg32f:
                    return format::rg;
                case intern::rg8i:
                case intern::rg8ui:
                case intern::rg16i:
                case intern::rg16ui:
                case intern::rg32i:
                case intern::rg32ui:
                    return format::rg_int;
                case intern::r3_g3_b2:
                case intern::rgb4:
                case intern::rgb5:
                case intern::rgb565:
                case intern::rgb8:
                case intern::rgb8_n:
                case intern::rgb10:
                case intern::rgb12:
                case intern::rgb16:
                case intern::rgb16_n:
                case intern::srgb8:
                case intern::r11f_g11f_b10f:
                case intern::rgb9_e5:
                case intern::rgb16f:
                case intern::rgb32f:
                    return format::rgb;
                case intern::rgb8i:
                case intern::rgb8ui:
                case intern::rgb16i:
                case intern::rgb16ui:
                case intern::rgb32i:
                case intern::rgb32ui:
                    return format::rgb_int;
                case intern::rgba2:
                case intern::rgba4:
                case intern::rgb5_a1:
                case intern::rgba8:
                case intern::rgba8_n:
                case intern::rgb10_a2:
                case intern::rgba12:
                case intern::rgba16:
                case intern::rgba16_n:
                case intern::srgb8_a8:
                case intern::rgba16f:
                case intern::rgba32f:
                    return format::rgba;
                case intern::rgb10_a2ui:
                case intern::rgba8i:
                case intern::rgba8ui:
                case intern::rgba16i:
                case intern::rgba16ui:
                case intern::rgba32i:
                case intern::rgba32ui:
                    return format::rgba_int;
                case intern::depth16:
                case intern::depth24:
                case intern::depth32:
                case intern::depth32f:
                    return format::depth;
                case intern::stencil1:
                case intern::stencil4:
                case intern::stencil8:
                case intern::stencil16:
                    return format::stencil;
                case intern::depth24_stencil8:
                case intern::depth32f_stencil8:
                    return format::depth_stencil;
                default:
                    return format::rgba;
            }
        }

        constexpr type to_type(const intern internal_format) {
            switch (internal_format) {
                case intern::r8:
                case intern::r8ui:
                case intern::rg8:
                case intern::rg8ui:
                case intern::rgb8:
                case intern::rgb8ui:
                case intern::rgba8:
                case intern::rgba8ui:
                case intern::srgb8:
                case intern::srgb8_a8:
                case intern::stencil8:
                    return type::ubyte;
                case intern::r8i:
                case intern::r8_n:
                case intern::rg8i:
                case intern::rg8_n:
                case intern::rgb8i:
                case intern::rgb8_n:
                case intern::rgba8i:
                case intern::rgba8_n:
                    return type::byte;
                case intern::r16:
                case intern::r16ui:
                case intern::rg16:
                case intern::rg16ui:
                case intern::rgb16:
                case intern::rgb16ui:
                case intern::rgba16:
                case intern::rgba16ui:
                case intern::depth16:
                case intern::stencil16:
                    return type::ushort;
                case intern::r16i:
                case intern::r16_n:
                case intern::rg16i:
                case intern::rg16_n:
                case intern::rgb16i:
                case intern::rgb16_n:
                case intern::rgba16i:
                case intern::rgba16_n:
                    return type::s;
                case intern::r32ui:
                case intern::rg32ui:
                case intern::rgb32ui:
                case intern::rgba32ui:
                case intern::depth32:
                    return type::uint;
                case intern::r32i:
                case intern::rg32i:
                case intern::rgb32i:
                case intern::rgba32i:
                    return type::i;
                case intern::r16f:
                case intern::rg16f:
                case intern::rgb16f:
                case intern::rgba16f:
                    return type::half_f;
                case intern::r32f:
                case intern::rg32f:
                case intern::rgb32f:
                case intern::rgba32f:
                case intern::depth32f:
                    return type::f;
                case intern::r11f_g11f_b10f:
                    return type::uint_10f_11f_11f_r;
                case intern::r3_g3_b2:
                    return type::ubyte_2_3_3_r;
                case intern::rgba4:
                    return type::ushort_4_4_4_4_r;
                case intern::rgb565:
                    return type::ushort_5_6_5_r;
                case intern::rgb9_e5:
                    return type::uint_5_9_9_9_r;
                case intern::rgb5_a1:
                    return type::ushort_1_5_5_5_r;
                case intern::rgb10_a2:
                case intern::rgb10_a2ui:
                    return type::uint_2_10_10_10_r;
                case intern::depth24_stencil8:
                    return type::uint_24_8;
                case intern::depth32f_stencil8:
                    return type::f32_uint_24_8_r;
                case intern::depth24:
                case intern::stencil1:
                case intern::stencil4:
                case intern::rgb4:
                case intern::rgb5:
                case intern::rgb10:
                case intern::rgb12:
                case intern::rgba2:
                case intern::rgba12:
                    throw std::exception{"These types cannot be deduced."};
                default:
                    return type::f;
            }
        }

        template <typename T>
        constexpr format deduce_format(const T&, const intern internal_format) {
            if constexpr (std::is_integral<T>::value) {
                if constexpr (is_integer_format(to_format(internal_format))) {
                    return format::r_int;
                }
                else {
                    return format::r;
                }
            }
            return format::r;
        }

        template <typename T, size_t N, glm::precision P>
        constexpr format deduce_format(const glm::vec<N, T, P>&, const intern internal_format) {
            switch (N) {
                case 1:
                    if constexpr (std::is_integral<T>::value) {
                        if constexpr (is_integer_format(to_format(internal_format))) {
                            return format::r_int;
                        }
                        else {
                            return format::r;
                        }
                    }
                    return format::r;
                case 2:
                    if constexpr (std::is_integral<T>::value) {
                        if constexpr (is_integer_format(to_format(internal_format))) {
                            return format::rg_int;
                        }
                        else {
                            return format::rg;
                        }
                    }
                    return format::rg;
                case 3:
                    if constexpr (std::is_integral<T>::value) {
                        if constexpr (is_integer_format(to_format(internal_format))) {
                            return format::rgb_int;
                        }
                        else {
                            return format::rgb;
                        }
                    }
                    return format::rgb;
                case 4:
                    if constexpr (std::is_integral<T>::value) {
                        if constexpr (is_integer_format(to_format(internal_format))) {
                            return format::rgba_int;
                        }
                        else {
                            return format::rgba;
                        }
                    }
                    return format::rgba;
                default:
                    if constexpr (std::is_integral<T>::value) {
                        if constexpr (is_integer_format(to_format(internal_format))) {
                            return format::r_int;
                        }
                        else {
                            return format::r;
                        }
                    }
                    return format::r;
            }
        }

        enum class min_filter : GLenum {
            linear = GL_LINEAR,
            nearest = GL_NEAREST,
            nearest_mipmap_linear = GL_NEAREST_MIPMAP_LINEAR,
            nearest_mipmap_nearest = GL_NEAREST_MIPMAP_NEAREST,
            linear_mipmap_linear = GL_LINEAR_MIPMAP_LINEAR,
            linear_mipmap_nearest = GL_LINEAR_MIPMAP_NEAREST,
        };

        enum class mag_filter : GLenum {
            linear = GL_LINEAR,
            nearest = GL_NEAREST
        };

        enum class wrap : GLenum {
            border = GL_CLAMP_TO_BORDER,
            edge = GL_CLAMP_TO_EDGE,
            mirror_edge = GL_MIRROR_CLAMP_TO_EDGE,
            repeat = GL_REPEAT,
            mirror_repeat = GL_MIRRORED_REPEAT
        };
    }

    struct Texture_definition {
        Texture_definition(const tex::intern internal_format = tex::intern::rgba8)
            : internal_format(internal_format) {
            format = to_format(internal_format);
            type = to_type(internal_format);
        }

        Texture_definition(const tex::intern internal_format, const tex::format format)
            : internal_format(internal_format), format(format) {
            type = to_type(internal_format);
        }

        Texture_definition(const tex::intern internal_format, const tex::format format, const tex::type type)
            : internal_format(internal_format), format(format), type(type) { }

        tex::intern internal_format = tex::intern::rgba8;
        tex::format format = tex::format::rgba;
        tex::type type = tex::type::ubyte;
    };

    struct Texture_params {
        tex::min_filter min_filter = tex::min_filter::linear;
        tex::mag_filter mag_filter = tex::mag_filter::linear;
        tex::wrap wrap_s = tex::wrap::edge;
        tex::wrap wrap_t = tex::wrap::edge;
        tex::wrap wrap_r = tex::wrap::edge;
        unsigned int mipmap_levels = 1;
        bool auto_gen_mipmap = true;
        unsigned int samples = 1;
        bool fixed_sample_points = true;
    };
}
