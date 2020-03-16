//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "gl.hpp"
#include "texture_util.hpp"
#include "file/stb_image.hpp"
#include "gl_types.hpp"
#include <filesystem>

namespace tostf
{
    struct FBO_texture_def {
        GLuint name;
        tex::format format;
    };

    class Texture {
    public:
        Texture(const Texture&) = default;
        Texture& operator=(const Texture&) = default;
        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;
        virtual ~Texture();
        void save_to_png(const std::filesystem::path& path, unsigned layer = 1, unsigned level = 0) const;
        void bind_to_unit(GLuint unit);
        GLuint get_name() const;
        FBO_texture_def get_fbo_tex_def() const;
        GLuint64 get_tex_handle();
        tex::target get_target() const;
        GLuint64 get_image_handle(gl::img_access access = gl::img_access::read_write, int layer = 0, int level = 0,
                                  bool layered = false);
        tex_res get_res() const;
        template <typename T>
        T get_pixel_data(int x, int y = 0, int z = 0);
        void generate_mipmaps() const;
        template <typename T>
        void set_data(const std::unique_ptr<T>& data, unsigned layer = 0, unsigned level = 0);
        template <typename T>
        std::enable_if_t<is_container_v<T>> set_data(const T& data, unsigned layer = 0, unsigned level = 0);
    protected:
        explicit Texture(tex::target target, const Texture_definition& tex_def = {}, const Texture_params& params = {});
        Texture(tex::target target, tex_res res, const Texture_definition& tex_def = {},
                const Texture_params& params = {});
        void set_min_filter(tex::min_filter filter) const;
        void set_mag_filter(tex::mag_filter filter) const;
        void set_wrap(tex::wrap s, tex::wrap t = tex::wrap::border, tex::wrap r = tex::wrap::border);
        void initialize_storage(tex_res res);
        void load_texture_from_file(const std::filesystem::path& path, unsigned layer = 0, unsigned level = 0,
                                    stbi_type image_type = stbi_type::ubyte);
        GLuint _name{};
        GLuint64 _texture_handle = 0;
        GLuint64 _image_handle = 0;
        tex::target _target;
        tex_res _res{};
        Texture_definition _tex_def;
        Texture_params _params;
        bool _storage_initialized = false;
        bool _is_resident = false;
    };

    template <typename T>
    T Texture::get_pixel_data(const int x, const int y, const int z) {
        std::array<T, 1> image_data;
        glGetTextureSubImage(_name, 0, x, y, z, 1, 1, 1, to_underlying_type(_tex_def.format),
                             to_underlying_type(_tex_def.type), sizeof(T) * static_cast<int>(image_data.size()),
                             image_data.data());
        return *reinterpret_cast<T*>(image_data.data());
    }

    template <typename T>
    void Texture::set_data(const std::unique_ptr<T>& data, const unsigned layer, const unsigned level) {
        switch (_target) {
            case tex::target::tex_1d:
                glTextureSubImage1D(_name, level, 0, _res.x, to_underlying_type(_tex_def.format),
                                    to_underlying_type(_tex_def.type), data.get());
                break;
            case tex::target::tex_2d:
            case tex::target::tex_1d_array:
                glTextureSubImage2D(_name, level, 0, 0, _res.x, _res.y, to_underlying_type(_tex_def.format),
                                    to_underlying_type(_tex_def.type), data.get());
                break;
            case tex::target::cube_map:
            case tex::target::tex_3d:
            case tex::target::tex_2d_array:
            case tex::target::cube_map_array:
                glTextureSubImage3D(_name, level, 0, 0, layer, _res.x, _res.y, _res.z,
                                    to_underlying_type(_tex_def.format), to_underlying_type(_tex_def.type), data.get());
                break;
            default:
                break;
        }
    }

    template <typename T>
    std::enable_if_t<is_container_v<T>> Texture::set_data(const T& data, const unsigned layer, const unsigned level) {
        switch (_target) {
            case tex::target::tex_1d:
                glTextureSubImage1D(_name, level, 0, _res.x, to_underlying_type(_tex_def.format),
                                    to_underlying_type(_tex_def.type), data.data());
                break;
            case tex::target::tex_2d:
            case tex::target::tex_1d_array:
                glTextureSubImage2D(_name, level, 0, 0, _res.x, _res.y, to_underlying_type(_tex_def.format),
                                    to_underlying_type(_tex_def.type), data.data());
                break;
            case tex::target::cube_map:
            case tex::target::tex_3d:
            case tex::target::tex_2d_array:
            case tex::target::cube_map_array:
                glTextureSubImage3D(_name, level, 0, 0, layer, _res.x, _res.y, _res.z,
                                    to_underlying_type(_tex_def.format), to_underlying_type(_tex_def.type),
                                    data.data());
                break;
            default:
                break;
        }
    }

    class Texture1D : public Texture {
    public:
        explicit Texture1D(tex_res res, const Texture_definition& tex_def = {}, const Texture_params& params = {});
        explicit Texture1D(const std::filesystem::path& path, const Texture_definition& tex_def = {},
                           const Texture_params& params = {});
    };

    class Texture2D : public Texture {
    public:
        explicit Texture2D(tex_res res, const Texture_definition& tex_def = {}, const Texture_params& params = {});
        explicit Texture2D(const std::filesystem::path& path, const Texture_definition& tex_def = {},
                           const Texture_params& params = {});
    };

    class Texture3D : public Texture {
    public:
        explicit Texture3D(tex_res res, const Texture_definition& tex_def = {}, const Texture_params& params = {});
    };
}
