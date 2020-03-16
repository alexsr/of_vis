//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#pragma warning(push)
#pragma warning(disable: 4100)
#include <stb/stb_image.h>
#pragma warning(pop)
#include "gpu_interface/texture_util.hpp"
#include "file/file_handling.hpp"

namespace tostf
{
    enum class stbi_channels {
        def = STBI_default,
        grey = STBI_grey,
        grey_a = STBI_grey_alpha,
        rgb = STBI_rgb,
        rgba = STBI_rgb_alpha
    };

    enum class stbi_type {
        ubyte,
        f,
        ushort // 16bit
    };

    template <typename T>
    struct stbi_data {
        int channels;
        int width;
        int height;
        std::unique_ptr<T> data_ptr;
    };

    constexpr unsigned int to_stbi_channels(const tex::format format) {
        switch (format) {
            case tex::format::r:
            case tex::format::g:
            case tex::format::b:
            case tex::format::r_int:
            case tex::format::g_int:
            case tex::format::b_int:
            case tex::format::depth:
            case tex::format::stencil:
                return STBI_grey;
            case tex::format::rg:
            case tex::format::rg_int:
            case tex::format::depth_stencil:
                return STBI_grey_alpha;
            case tex::format::rgb_int:
            case tex::format::rgb:
            case tex::format::bgr:
                return STBI_rgb;
            case tex::format::rgba_int:
            case tex::format::rgba:
            case tex::format::bgra:
            case tex::format::bgra_int:
                return STBI_rgb_alpha;
            default:
                return STBI_default;
        }
    }

    namespace stb
    {
        tex_res get_image_size_1d(const std::filesystem::path & path);
        tex_res get_image_size(const std::filesystem::path & path);
        stbi_data<stbi_uc> load_image(const std::filesystem::path& path, tex::format desired_format);
        stbi_data<float> load_f_image(const std::filesystem::path& path, tex::format desired_format);
        stbi_data<stbi_us> load_16bit_image(const std::filesystem::path& path, tex::format desired_format);

        void save_to_png(const std::vector<unsigned char>& data, const std::filesystem::path& path,
                         tex_2d_res res, tex::format format);
    }
}
