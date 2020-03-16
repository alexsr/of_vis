//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "stb_image.hpp"
#include <stb/stb_image_write.h>

tostf::tex_res tostf::stb::get_image_size_1d(const std::filesystem::path& path) {
    check_file_validity(path);
    tex_res res(1);
    stbi_info(path.string().c_str(), &res.x, &res.y, nullptr);
    return {res.x * res.y, 1, 1};

}

tostf::tex_res tostf::stb::get_image_size(const std::filesystem::path& path) {
    check_file_validity(path);
    tex_res res(1);
    stbi_info(path.string().c_str(), &res.x, &res.y, nullptr);
    return res;
}

tostf::stbi_data<stbi_uc> tostf::stb::load_image(const std::filesystem::path& path, const tex::format desired_format) {
    check_file_validity(path);
    stbi_data<stbi_uc> image{};
    stbi_set_flip_vertically_on_load(1);
    image.data_ptr = std::unique_ptr<stbi_uc>(stbi_load(path.string().c_str(), &image.width, &image.height,
                                                        &image.channels, to_stbi_channels(desired_format)));
    return image;
}

tostf::stbi_data<float> tostf::stb::load_f_image(const std::filesystem::path& path, const tex::format desired_format) {
    check_file_validity(path);
    stbi_data<float> image{};
    stbi_set_flip_vertically_on_load(1);
    image.data_ptr = std::unique_ptr<float>(stbi_loadf(path.string().c_str(), &image.width, &image.height,
                                                       &image.channels, to_stbi_channels(desired_format)));
    return image;
}

tostf::stbi_data<stbi_us> tostf::stb::load_16bit_image(const std::filesystem::path& path,
                                                       const tex::format desired_format) {
    check_file_validity(path);
    stbi_data<stbi_us> image{};
    stbi_set_flip_vertically_on_load(1);
    image.data_ptr = std::unique_ptr<stbi_us>(stbi_load_16(path.string().c_str(), &image.width, &image.height,
                                                           &image.channels, to_stbi_channels(desired_format)));
    return image;
}

void tostf::stb::save_to_png(const std::vector<unsigned char>& data, const std::filesystem::path& path,
                             const tex_2d_res res, const tex::format format) {
    stbi_flip_vertically_on_write(true);
    const int channels = to_stbi_channels(format);
    const auto error = stbi_write_png(path.string().c_str(), res.x, res.y, channels, data.data(),
                                      channels * res.x);
    if (error == 0) {
        throw std::runtime_error{"Writing to " + path.string() + " failed."};
    }
}
