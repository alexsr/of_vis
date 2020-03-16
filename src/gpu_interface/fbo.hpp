//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "texture.hpp"
#include <vector>

namespace tostf
{
    class Renderbuffer {
    public:
        Renderbuffer(tex::intern internal_format, tex_res res);
        Renderbuffer(tex::intern internal_format, unsigned int samples, tex_res res);
        Renderbuffer(const Renderbuffer&) = default;
        Renderbuffer& operator=(const Renderbuffer&) = default;
        Renderbuffer(Renderbuffer&& other) noexcept;
        Renderbuffer& operator=(Renderbuffer&& other) noexcept;
        ~Renderbuffer();
        GLuint get_name() const;
    private:
        GLuint _name = 0;
    };

    enum class fbo_attachment : GLenum {
        color = GL_COLOR_ATTACHMENT0,
        depth = GL_DEPTH_ATTACHMENT,
        stencil = GL_STENCIL_ATTACHMENT,
        depth_stencil = GL_DEPTH_STENCIL_ATTACHMENT
    };

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    fbo_attachment operator+(const fbo_attachment& a, T& v) {
        return static_cast<fbo_attachment>(to_underlying_type(a) + v);
    }

    template <typename T, typename = std::enable_if_t<std::is_integral<T>::value>>
    fbo_attachment operator+(T& v, const fbo_attachment& a) {
        return static_cast<fbo_attachment>(to_underlying_type(a) + v);
    }

    struct FBO_renderbuffer_def {
        tex::intern internal_format = tex::intern::rgba8;
        bool is_multisampling = false;
        unsigned int samples = 1;
    };

    class FBO {
    public:
        explicit FBO(tex_res res, const FBO_texture_def& texture);
        explicit FBO(tex_res res, const std::vector<FBO_texture_def>& textures = {},
                     const std::vector<FBO_renderbuffer_def>& renderbuffers = {});
        FBO(const FBO&) = default;
        FBO& operator=(const FBO&) = default;
        FBO(FBO&& other) noexcept;
        FBO& operator=(FBO&& other) noexcept;
        ~FBO();
        void bind() const;
        static void unbind();
        void attach_texture(FBO_texture_def t, fbo_attachment attachment);
        void attach_color_texture(FBO_texture_def t, int i);
    private:
        void process_textures(const std::vector<FBO_texture_def>& textures);
        void process_renderbuffers(const std::vector<FBO_renderbuffer_def>& renderbuffers);
        void attach_component(FBO_texture_def t, fbo_attachment attachment) const;
        void attach_component(FBO_renderbuffer_def t, fbo_attachment attachment);
        void check_fbo_status() const;
        GLuint _name = 0;
        tex_res _res;
        std::vector<Renderbuffer> _renderbuffers;
        std::vector<GLenum> _color_attachments;
        bool _has_depth_attachment = false;
        bool _has_stencil_attachment = false;
    };
}
