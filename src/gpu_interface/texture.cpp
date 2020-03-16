//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "texture.hpp"
#include "utility/logging.hpp"
#include <future>
#include "debug.hpp"

tostf::Texture::Texture(tex::target target, const Texture_definition& tex_def, const Texture_params& params)
    : _target(target), _tex_def(tex_def), _params(params) {
    glCreateTextures(to_underlying_type(target), 1, &_name);
    set_min_filter(_params.min_filter);
    set_mag_filter(_params.mag_filter);
    set_wrap(params.wrap_s, params.wrap_t, params.wrap_r);
}

tostf::Texture::Texture(tex::target target, tex_res res, const Texture_definition& tex_def,
                        const Texture_params& params)
    : _target(target), _tex_def(tex_def), _params(params) {
    glCreateTextures(to_underlying_type(target), 1, &_name);
    set_min_filter(_params.min_filter);
    set_mag_filter(_params.mag_filter);
    set_wrap(params.wrap_s, params.wrap_t, params.wrap_r);
    initialize_storage(res);
}

tostf::Texture::Texture(Texture&& other) noexcept {
    _name = other._name;
    _texture_handle = other._texture_handle;
    _target = other._target;
    _tex_def = other._tex_def;
    _res = other._res;
    other._name = 0;
}

tostf::Texture& tostf::Texture::operator=(Texture&& other) noexcept {
    _name = other._name;
    _texture_handle = other._texture_handle;
    _target = other._target;
    _tex_def = other._tex_def;
    _res = other._res;
    other._name = 0;
    return *this;
}

tostf::Texture::~Texture() {
    if (glIsTexture(_name)) {
        glDeleteTextures(1, &_name);
    }
}

void tostf::Texture::save_to_png(const std::filesystem::path& path, const unsigned layer,
                                 const unsigned level) const {
    std::vector<GLubyte> image_data(_res.x * _res.y * 4);
    glGetTextureSubImage(_name, level, 0, 0, 0, _res.x, _res.y, layer, GL_RGBA, GL_UNSIGNED_BYTE,
                         static_cast<GLsizei>(image_data.size()), image_data.data());
    auto future = std::async(std::launch::async, [&]() {
        stb::save_to_png(image_data, path, _res, tex::format::rgba);
    });
}

void tostf::Texture::bind_to_unit(const GLuint unit) {
    glBindImageTexture(unit, _name, 1, false, 1, GL_READ_ONLY, to_underlying_type(_tex_def.format));
}

GLuint tostf::Texture::get_name() const {
    return _name;
}

tostf::FBO_texture_def tostf::Texture::get_fbo_tex_def() const {
    return {_name, _tex_def.format};
}

GLuint64 tostf::Texture::get_tex_handle() {
    if (_texture_handle == 0) {
        _texture_handle = glGetTextureHandleARB(_name);
        glMakeTextureHandleResidentARB(_texture_handle);
    }
    return _texture_handle;
}

tostf::tex::target tostf::Texture::get_target() const {
    return _target;
}

GLuint64 tostf::Texture::get_image_handle(const gl::img_access access, const int layer,
                                          const int level, const bool layered) {
    if (!_is_resident) {
        _image_handle = glGetImageHandleARB(_name, level, static_cast<GLboolean>(layered), layer,
                                            to_underlying_type(_tex_def.internal_format));
        glMakeImageHandleResidentARB(_image_handle, to_underlying_type(access));
        _is_resident = true;
    }
    return _image_handle;
}

tostf::tex_res tostf::Texture::get_res() const {
    return _res;
}

void tostf::Texture::set_min_filter(const tex::min_filter filter) const {
    glTextureParameteri(_name, GL_TEXTURE_MIN_FILTER, to_underlying_type(filter));
}

void tostf::Texture::set_mag_filter(const tex::mag_filter filter) const {
    glTextureParameteri(_name, GL_TEXTURE_MAG_FILTER, to_underlying_type(filter));
}

void tostf::Texture::set_wrap(const tex::wrap s, tex::wrap t, tex::wrap r) {
    switch (_target) {
        case tex::target::tex_1d:
            glTextureParameteri(_name, GL_TEXTURE_WRAP_S, to_underlying_type(s));
            break;
        case tex::target::tex_2d:
        case tex::target::cube_map:
        case tex::target::tex_1d_array:
            glTextureParameteri(_name, GL_TEXTURE_WRAP_S, to_underlying_type(s));
            glTextureParameteri(_name, GL_TEXTURE_WRAP_T, to_underlying_type(t));
            break;
        case tex::target::tex_3d:
        case tex::target::tex_2d_array:
        case tex::target::cube_map_array:
            glTextureParameteri(_name, GL_TEXTURE_WRAP_S, to_underlying_type(s));
            glTextureParameteri(_name, GL_TEXTURE_WRAP_T, to_underlying_type(t));
            glTextureParameteri(_name, GL_TEXTURE_WRAP_R, to_underlying_type(r));
            break;
        default:
            break;
    }
}

void tostf::Texture::initialize_storage(const tex_res res) {
    _res = res;
    _storage_initialized = true;
    switch (_target) {
        case tex::target::tex_1d:
            glTextureStorage1D(_name, _params.mipmap_levels, to_underlying_type(_tex_def.internal_format), _res.x);
            break;
        case tex::target::tex_2d:
        case tex::target::cube_map:
        case tex::target::tex_1d_array:
            glTextureStorage2D(_name, _params.mipmap_levels, to_underlying_type(_tex_def.internal_format), _res.x,
                               _res.y);
            break;
        case tex::target::tex_3d:
        case tex::target::tex_2d_array:
        case tex::target::cube_map_array:
            glTextureStorage3D(_name, _params.mipmap_levels, to_underlying_type(_tex_def.internal_format), _res.x,
                               _res.y, _res.z);
            break;
        default:
            break;
    }
}

void tostf::Texture::load_texture_from_file(const std::filesystem::path& path, const unsigned layer,
                                            const unsigned level, const stbi_type image_type) {
    log_info() << "Loading texture from file: " << path.string();
    switch (image_type) {
        case stbi_type::f: {
            const auto image_data = stb::load_f_image(path, _tex_def.format);
            set_data(image_data.data_ptr, layer, level);
        }
        break;
        case stbi_type::ushort: {
            const auto image_data = stb::load_16bit_image(path, _tex_def.format);
            set_data(image_data.data_ptr, layer, level);
        }
        break;
        default:
            const auto image_data = stb::load_image(path, _tex_def.format);
            set_data(image_data.data_ptr, layer, level);
    }
}

void tostf::Texture::generate_mipmaps() const {
    glGenerateTextureMipmap(_name);
}

tostf::Texture1D::Texture1D(const tex_res res, const Texture_definition& tex_def, const Texture_params& params)
    : Texture(tex::target::tex_1d, res, tex_def, params) {}

tostf::Texture1D::Texture1D(const std::filesystem::path& path, const Texture_definition& tex_def,
                            const Texture_params& params)
    : Texture(tex::target::tex_1d, stb::get_image_size_1d(path), tex_def, params) {
    load_texture_from_file(path);
}

tostf::Texture2D::Texture2D(const tex_res res, const Texture_definition& tex_def, const Texture_params& params)
    : Texture(tex::target::tex_2d, res, tex_def, params) {}

tostf::Texture2D::Texture2D(const std::filesystem::path& path, const Texture_definition& tex_def,
                            const Texture_params& params)
    :
    Texture(tex::target::tex_2d, stb::get_image_size(path), tex_def, params) {
    load_texture_from_file(path);
}

tostf::Texture3D::Texture3D(const tex_res res, const Texture_definition& tex_def, const Texture_params& params)
    : Texture(tex::target::tex_3d, res, tex_def, params) {}
