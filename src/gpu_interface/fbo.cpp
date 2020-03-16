//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "fbo.hpp"

tostf::Renderbuffer::Renderbuffer(const tex::intern internal_format, const tex_res res) {
    glCreateRenderbuffers(1, &_name);
    glNamedRenderbufferStorage(_name, to_underlying_type(internal_format), res.x, res.y);
}

tostf::Renderbuffer::Renderbuffer(const tex::intern internal_format, const unsigned int samples,
                                  const tex_res res) {
    glCreateRenderbuffers(1, &_name);
    glNamedRenderbufferStorageMultisample(_name, samples, to_underlying_type(internal_format),
                                          res.x, res.y);
}

tostf::Renderbuffer::Renderbuffer(Renderbuffer&& other) noexcept {
    _name = other._name;
    other._name = 0;
}

tostf::Renderbuffer& tostf::Renderbuffer::operator=(Renderbuffer&& other) noexcept {
    _name = other._name;
    other._name = 0;
    return *this;
}

tostf::Renderbuffer::~Renderbuffer() {
    if (glIsRenderbuffer(_name)) {
        glDeleteRenderbuffers(1, &_name);
    }
}

GLuint tostf::Renderbuffer::get_name() const {
    return _name;
}

tostf::FBO::FBO(const tex_res res, const FBO_texture_def& texture) : _res(res) {
    glCreateFramebuffers(1, &_name);
    std::vector<GLenum> attachments;
    process_textures({texture});
    glNamedFramebufferDrawBuffers(_name, static_cast<GLsizei>(_color_attachments.size()), _color_attachments.data());
    check_fbo_status();

}

tostf::FBO::FBO(const tex_res res, const std::vector<FBO_texture_def>& textures,
                const std::vector<FBO_renderbuffer_def>& renderbuffers) : _res(res) {
    glCreateFramebuffers(1, &_name);
    std::vector<GLenum> attachments;
    process_renderbuffers(renderbuffers);
    process_textures(textures);
    glNamedFramebufferDrawBuffers(_name, static_cast<GLsizei>(_color_attachments.size()), _color_attachments.data());
    check_fbo_status();
}

tostf::FBO::FBO(FBO&& other) noexcept {
    _name = other._name;
    _res = other._res;
    _renderbuffers = std::move(other._renderbuffers);
    _color_attachments = std::move(other._color_attachments);
    _has_depth_attachment = other._has_depth_attachment;
    _has_stencil_attachment = other._has_stencil_attachment;
    other._name = 0;
}

tostf::FBO& tostf::FBO::operator=(FBO&& other) noexcept {
    _name = other._name;
    _res = other._res;
    _renderbuffers = std::move(other._renderbuffers);
    _color_attachments = std::move(other._color_attachments);
    _has_depth_attachment = other._has_depth_attachment;
    _has_stencil_attachment = other._has_stencil_attachment;
    other._name = 0;
    return *this;
}

tostf::FBO::~FBO() {
    if (glIsFramebuffer(_name)) {
        unbind();
        glDeleteFramebuffers(1, &_name);
    }
}

void tostf::FBO::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, _name);
}

void tostf::FBO::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void tostf::FBO::attach_texture(const FBO_texture_def t, const fbo_attachment attachment) {
    if (attachment == fbo_attachment::color) {
        auto i = _color_attachments.size();
        attach_component(t, fbo_attachment::color + i);
        _color_attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
    }
    else if (attachment == fbo_attachment::depth_stencil) {
        attach_component(t, fbo_attachment::depth);
        attach_component(t, fbo_attachment::stencil);
    }
    else {
        attach_component(t, attachment);
    }
}

void tostf::FBO::attach_color_texture(const FBO_texture_def t, const int i) {
    if (i < static_cast<int>(_color_attachments.size())) {
        _color_attachments.at(i) = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);
    }
    else {
        _color_attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        glNamedFramebufferDrawBuffers(_name, static_cast<GLsizei>(_color_attachments.size()),
                                      _color_attachments.data());
    }
    attach_component(t, fbo_attachment::color + i);
}

void tostf::FBO::process_textures(const std::vector<FBO_texture_def>& textures) {
    for (const auto t : textures) {
        if (t.format == tex::format::depth) {
            if (_has_depth_attachment) {
                throw std::runtime_error{"FBO " + std::to_string(_name) + " already has a depth attachment."};
            }
            attach_component(t, fbo_attachment::depth);
            _has_depth_attachment = true;
        }
        else if (t.format == tex::format::stencil) {
            if (_has_stencil_attachment) {
                throw std::runtime_error{"FBO " + std::to_string(_name) + " already has a depth attachment."};
            }
            attach_component(t, fbo_attachment::stencil);
            _has_stencil_attachment = true;
        }
        else if (t.format == tex::format::depth_stencil) {
            if (_has_stencil_attachment) {
                throw std::runtime_error{"FBO " + std::to_string(_name) + " already has a depth attachment."};
            }
            attach_component(t, fbo_attachment::depth);
            attach_component(t, fbo_attachment::stencil);
            _has_depth_attachment = true;
            _has_stencil_attachment = true;
        }
        else {
            auto i = _color_attachments.size();
            attach_component(t, fbo_attachment::color + i);
            _color_attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        }
    }
}

void tostf::FBO::process_renderbuffers(const std::vector<FBO_renderbuffer_def>& renderbuffers) {
    for (const auto r : renderbuffers) {
        if (to_format(r.internal_format) == tex::format::depth) {
            if (_has_depth_attachment) {
                throw std::runtime_error{"FBO " + std::to_string(_name) + " already has a depth attachment."};
            }
            attach_component(r, fbo_attachment::depth);
            _has_depth_attachment = true;
        }
        else if (to_format(r.internal_format) == tex::format::stencil) {
            if (_has_stencil_attachment) {
                throw std::runtime_error{"FBO " + std::to_string(_name) + " already has a depth attachment."};
            }
            attach_component(r, fbo_attachment::stencil);
            _has_stencil_attachment = true;
        }
        else if (to_format(r.internal_format) == tex::format::depth_stencil) {
            if (_has_stencil_attachment) {
                throw std::runtime_error{"FBO " + std::to_string(_name) + " already has a depth attachment."};
            }
            attach_component(r, fbo_attachment::depth_stencil);
            _has_depth_attachment = true;
            _has_stencil_attachment = true;
        }
        else {
            auto i = _color_attachments.size();
            attach_component(r, fbo_attachment::color + i);
            _color_attachments.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
        }
    }
}

void tostf::FBO::attach_component(const FBO_texture_def t, const fbo_attachment attachment) const {
    glNamedFramebufferTexture(_name, to_underlying_type(attachment), t.name, 0);
}

void tostf::FBO::attach_component(FBO_renderbuffer_def t, const fbo_attachment attachment) {
    if (t.is_multisampling) {
        const auto rb = _renderbuffers.emplace_back(t.internal_format, t.samples, _res);
        glNamedFramebufferRenderbuffer(_name, to_underlying_type(attachment), GL_RENDERBUFFER, rb.get_name());
    }
    else {
        const auto rb = _renderbuffers.emplace_back(t.internal_format, _res);
        glNamedFramebufferRenderbuffer(_name, to_underlying_type(attachment), GL_RENDERBUFFER, rb.get_name());
    }
}

void tostf::FBO::check_fbo_status() const {
    if (glCheckNamedFramebufferStatus(_name, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error{"Framebuffer " + std::to_string(_name) + " was not created."};
    }
}
