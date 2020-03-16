//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "gl.hpp"
#include "utility/logging.hpp"
#include <vector>

void tostf::gl::Context::retrieve_info() {
    glGetIntegerv(GL_MAJOR_VERSION, &major_version);
    glGetIntegerv(GL_MINOR_VERSION, &minor_version);
    vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    extended_glsl_version = reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    glsl_version = std::to_string(major_version * 100 + minor_version * 10);

    // framebuffer queries
    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &max_framebuffer_width);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &max_framebuffer_height);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &max_framebuffer_layers);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, &max_framebuffer_samples);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer_size);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_texture_layers);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &max_cube_map_texture_size);
    glGetIntegerv(GL_MAX_RECTANGLE_TEXTURE_SIZE, &max_rectangle_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &max_texture_buffer_size);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max_3d_texture_size);
    glGetIntegerv(GL_MAX_TEXTURE_LOD_BIAS, &max_texture_lod_bias);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &max_color_texture_samples);
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &max_depth_texture_samples);

    // compute shader queries
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_workgroup_count.at(0));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &max_workgroup_count.at(1));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &max_workgroup_count.at(2));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_workgroup_size.at(0));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_workgroup_size.at(1));
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_workgroup_size.at(2));
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_workgroup_invocations);
    glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &max_compute_shared_memory_size);
}

void tostf::gl::Context::print_info() {
    log_section_no_newline("OpenGL basic context info");
    log_info() << "OpenGL Version: " << major_version << "." << minor_version;
    log_info() << "Vendor: " << vendor;
    log_info() << "Renderer: " << renderer;
    log_info() << "GLSL Version: " << extended_glsl_version;
    log_section("Framebuffer info");
    log_info() << "Max framebuffer width: " << max_framebuffer_width;
    log_info() << "Max framebuffer height: " << max_framebuffer_height;
    log_info() << "Max framebuffer layers: " << max_framebuffer_layers;
    log_info() << "Max framebuffer samples: " << max_framebuffer_samples;
    log_info() << "Max renderbuffer size: " << max_renderbuffer_size;
    log_info() << "Max color attachments: " << max_color_attachments;
    log_section("Texture info");
    log_info() << "Max texture size: " << max_texture_size;
    log_info() << "Max array texture layers: " << max_array_texture_layers;
    log_info() << "Max cube map texture size: " << max_cube_map_texture_size;
    log_info() << "Max rectangle texture size: " << max_rectangle_texture_size;
    log_info() << "Max texture buffer size: " << max_texture_buffer_size;
    log_info() << "Max 3D texture size: " << max_3d_texture_size;
    log_info() << "Max texture LOD bias: " << max_texture_lod_bias;
    log_info() << "Max color texture samples: " << max_color_texture_samples;
    log_info() << "Max depth texture samples: " << max_depth_texture_samples;
    log_section("Compute shader info");
    log_info() << "Max workgroup count: " << max_workgroup_count.at(0) << ", "
        << max_workgroup_count.at(1) << ", " << max_workgroup_count.at(2);
    log_info() << "Max workgroup size: " << max_workgroup_size.at(0) << ", "
        << max_workgroup_size.at(1) << ", " << max_workgroup_size.at(2);
    log_info() << "Max workgroup invocations: " << max_workgroup_invocations;
    log_info() << "Max shared memory: " << max_compute_shared_memory_size;
}

void tostf::gl::clear(const clear_bit bits) {
    glClear(to_underlying_type(bits));
}

void tostf::gl::clear_all() {
    glClear(to_underlying_type(clear_bit::color | clear_bit::depth | clear_bit::stencil));
}

void tostf::gl::set_wireframe(const bool mode) {
    if (mode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void tostf::gl::enable(const cap capability) {
    glEnable(to_underlying_type(capability));
}

void tostf::gl::enable(const std::vector<cap>& capabilities) {
    for (auto& c : capabilities) {
        enable(c);
    }
}

void tostf::gl::disable(const cap capability) {
    glDisable(to_underlying_type(capability));
}

void tostf::gl::disable(const std::vector<cap>& capabilities) {
    for (auto& c : capabilities) {
        disable(c);
    }
}

void tostf::gl::set_clear_color(const glm::vec4 color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void tostf::gl::set_clear_color(const float r, const float g, const float b, const float a) {
    glClearColor(r, g, b, a);
}

void tostf::gl::set_viewport(const Viewport vp) {
    glViewport(vp.x, vp.y, vp.width, vp.height);
}

void tostf::gl::set_viewport(const int width, const int height) {
    glViewport(0, 0, width, height);
}

void tostf::gl::set_viewport(const int x, const int y, const int width, const int height) {
    glViewport(x, y, width, height);
}

tostf::gl::Viewport tostf::gl::get_viewport() {
    std::array<int, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    return {viewport.at(0), viewport.at(1), viewport.at(2), viewport.at(3)};
}

void tostf::gl::set_scissor(const Viewport vp) {
    glScissor(vp.x, vp.y, vp.width, vp.height);
}

void tostf::gl::set_scissor(const int width, const int height) {
    glScissor(0, 0, width, height);
}

void tostf::gl::set_scissor(const int x, const int y, const int width, const int height) {
    glScissor(x, y, width, height);
}