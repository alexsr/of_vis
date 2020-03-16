//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "utility/utility.hpp"
#include "math/glm_helper.hpp"
#include <glad/glad.h>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>

namespace tostf
{
    namespace gl
    {
        struct Context {
            GLint major_version{};
            GLint minor_version{};
            std::string vendor;
            std::string renderer;
            std::string extended_glsl_version;
            std::string glsl_version;

            //    // Buffer information

            //    GLint max_ubo_bindings;
            //    GLint max_ssbo_bindings;

            GLint max_framebuffer_width{};
            GLint max_framebuffer_height{};
            GLint max_framebuffer_layers{};
            GLint max_framebuffer_samples{};
            GLint max_renderbuffer_size{};
            GLint max_color_attachments{};

            GLint max_texture_size{};
            GLint max_array_texture_layers{};
            GLint max_cube_map_texture_size{};
            GLint max_rectangle_texture_size{};
            GLint max_texture_buffer_size{};
            GLint max_3d_texture_size{};
            GLint max_texture_lod_bias{};
            GLint max_color_texture_samples{};
            GLint max_depth_texture_samples{};

            std::array<GLint, 3> max_workgroup_count{};
            std::array<GLint, 3> max_workgroup_size{};
            GLint max_workgroup_invocations{};
            GLint max_compute_shared_memory_size{};

            void retrieve_info();
            void print_info();
        };

        enum class clear_bit : GLbitfield {
            color = GL_COLOR_BUFFER_BIT,
            depth = GL_DEPTH_BUFFER_BIT,
            stencil = GL_STENCIL_BUFFER_BIT
        };

        constexpr clear_bit operator|(const clear_bit b1, const clear_bit b2) {
            return static_cast<clear_bit>(to_underlying_type(b1) | to_underlying_type(b2));
        }

        constexpr bool operator&(const clear_bit b1, const clear_bit b2) {
            return (to_underlying_type(b1) & to_underlying_type(b2)) != 0;
        }

        enum class cap {
            depth_test = GL_DEPTH_TEST,
            stencil_test = GL_STENCIL_TEST,
            blend = GL_BLEND,
            multisample = GL_MULTISAMPLE,
            dither = GL_DITHER,
            scissor = GL_SCISSOR_TEST,
            cull_face = GL_CULL_FACE
        };

        constexpr cap operator|(const cap b1, const cap b2) {
            return static_cast<cap>(to_underlying_type(b1) | to_underlying_type(b2));
        }

        constexpr bool operator&(const cap b1, const clear_bit b2) {
            return (to_underlying_type(b1) & to_underlying_type(b2)) != 0;
        }

        void enable(cap capability);
        void enable(const std::vector<cap>& capabilities);
        void disable(cap capability);
        void disable(const std::vector<cap>& capabilities);
        void set_clear_color(glm::vec4 color);
        void set_clear_color(float r, float g, float b, float a);

        struct Viewport {
            int x;
            int y;
            int width;
            int height;
        };

        void set_viewport(Viewport vp);
        void set_viewport(int width, int height);
        void set_viewport(int x, int y, int width, int height);
        Viewport get_viewport();

        void set_scissor(Viewport vp);
        void set_scissor(int width, int height);
        void set_scissor(int x, int y, int width, int height);

        void clear(clear_bit bits);
        void clear_all();
        void set_wireframe(bool mode);


        // draw utils

        enum class draw : GLenum {
            points = GL_POINTS,
            line_strip = GL_LINE_STRIP,
            line_loop = GL_LINE_LOOP,
            lines = GL_LINES,
            line_strip_adj = GL_LINE_STRIP_ADJACENCY,
            lines_adj = GL_LINES_ADJACENCY,
            triangle_strip = GL_TRIANGLE_STRIP,
            triangle_fan = GL_TRIANGLE_FAN,
            triangles = GL_TRIANGLES,
            triangle_strip_adj = GL_TRIANGLE_STRIP_ADJACENCY,
            triangles_adj = GL_TRIANGLES_ADJACENCY,
            patches = GL_PATCHES
        };


        // storage utils

        enum class v_type : GLenum {
            byte = GL_BYTE,
            ubyte = GL_UNSIGNED_BYTE,
            shrt = GL_SHORT,
            ushrt = GL_UNSIGNED_SHORT,
            integer = GL_INT,
            uint = GL_UNSIGNED_INT,
            half_float = GL_HALF_FLOAT,
            flt = GL_FLOAT,
            dbl = GL_DOUBLE,
            fixed = GL_FIXED,
            int_2_10_10_10 = GL_INT_2_10_10_10_REV,
            uint_2_10_10_10 = GL_UNSIGNED_INT_2_10_10_10_REV,
            uint_10f_11f_11f = GL_UNSIGNED_INT_10F_11F_11F_REV
        };

        constexpr auto size_of(const v_type t) {
            switch (t) {
                case v_type::dbl:
                    return sizeof(double);
                case v_type::byte:
                case v_type::ubyte:
                    return sizeof(char);
                case v_type::shrt:
                case v_type::ushrt:
                case v_type::half_float:
                    return sizeof(short);
                case v_type::integer:
                case v_type::uint:
                case v_type::int_2_10_10_10:
                case v_type::uint_2_10_10_10:
                case v_type::uint_10f_11f_11f:
                    return sizeof(int32_t);
                default:
                    return sizeof(float);
            }
        }

        enum class storage : GLbitfield {
            dynamic = GL_DYNAMIC_STORAGE_BIT,
            read = GL_MAP_READ_BIT,
            write = GL_MAP_WRITE_BIT,
            persistent_read = GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
            persistent_write = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
            coherent_read = GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
            coherent_write = GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
            client = GL_CLIENT_STORAGE_BIT
        };

        constexpr storage operator|(const storage s1, const storage s2) {
            return static_cast<storage>(to_underlying_type(s1) | to_underlying_type(s2));
        }

        constexpr bool operator&(const storage s1, const storage s2) {
            return (to_underlying_type(s1) & to_underlying_type(s2)) != 0;
        }

        enum class map_access : GLbitfield {
            read = GL_MAP_READ_BIT,
            write = GL_MAP_WRITE_BIT,
            persistent_read = GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
            persistent_write = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
            coherent_read = GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT,
            coherent_write = GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT,
            invalidate_buffer = GL_MAP_INVALIDATE_BUFFER_BIT,
            invalidate_range = GL_MAP_INVALIDATE_RANGE_BIT,
            flush = GL_MAP_FLUSH_EXPLICIT_BIT,
            unsynchronized = GL_MAP_UNSYNCHRONIZED_BIT
        };

        constexpr map_access operator|(const map_access s1, const map_access s2) {
            return static_cast<map_access>(to_underlying_type(s1) | to_underlying_type(s2));
        }

        constexpr bool operator&(const map_access s1, const map_access s2) {
            return (to_underlying_type(s1) & to_underlying_type(s2)) != 0;
        }
    }
}
