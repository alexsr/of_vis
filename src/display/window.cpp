#include "window.hpp"
//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "window.hpp"
#include "file/stb_image.hpp"
#include "utility/logging.hpp"

#include <future>
#include <imgui/imgui.h>
#include "gpu_interface/texture_util.hpp"

int tostf::Window::_next_id = 0; // _next_id has to be initialized.

tostf::Window::Window(const window_config& config, const gl_settings& gl_config, const win_mgr::monitor monitor)
    : _id(_next_id++), _decorated(config.flags & win_mgr::window_flags::borderless),
      _diag_size(_diag_size = glm::sqrt(static_cast<float>(config.width * config.width
                                                           + config.height * config.height))) {
    _res = {config.width, config.height};
    if (_id == 0) {
        win_mgr::init(); // glfw has to be initialized before being used
    }
    _window = std::make_shared<win_mgr::win_obj>(config, gl_config, monitor);
    _context.retrieve_info();
    _context.print_info();
}

tostf::Window::~Window() {
    _window->destroy();
}

void tostf::Window::make_current() const {
    _window->make_context_current();
}

bool tostf::Window::should_close() const {
    return _should_close || _window->should_close();
}

void tostf::Window::close() {
    _should_close = true;
}

void tostf::Window::swap_buffers() const {
    _window->swap_buffers();
}

void tostf::Window::poll_events() {
    if (_should_toggle_maximize) {
        internal_toggle_maximize();
    }
    _window->poll_events();
}

void tostf::Window::wait_events() {
    _window->wait_events();
}

void tostf::Window::minimize() const {
    _window->minimize();
}

void tostf::Window::toggle_maximize() {
    const auto current_time = win_mgr::get_time();
    if (current_time - _last_action_time > 0.5) {
        _last_action_time = current_time;
        _should_toggle_maximize = true;
    }
}

void tostf::Window::internal_toggle_maximize() {
    _should_toggle_maximize = false;
    if (is_maximized()) {
        _window->restore();
    }
    else {
        _window->maximize();
    }
}

bool tostf::Window::is_maximized() const {
    return _window->is_maximized();
}

bool tostf::Window::is_minimized() const {
    return _window->is_minimized();
}

void tostf::Window::register_key_callback(const std::string& name, const win_mgr::key key,
                                          const win_mgr::key_action action, const std::function<void()>& func,
                                          const win_mgr::key_mod mod) {
    const auto callback = [key, action, mod, func](GLFWwindow*, const int k, int, const int a, const int m) {
        if (k == to_underlying_type(key) && a == to_underlying_type(action) && m == to_underlying_type(mod)) {
            func();
        }
    };
    add_key_fun(name, callback);
}

void tostf::Window::register_mouse_callback(const std::string& name, const win_mgr::mouse button,
                                            const win_mgr::mouse_action action, const std::function<void()>& func,
                                            const win_mgr::key_mod mod) {
    const auto callback = [button, action, mod, func](GLFWwindow*, const int b, const int a, const int m) {
        if (b == to_underlying_type(button) && a == to_underlying_type(action) && m == to_underlying_type(mod)) {
            func();
        }
    };
    add_mouse_button_fun(name, callback);
}

bool tostf::Window::check_cursor_mode(const win_mgr::cursor_mode mode) const {
    return _window->check_cursor_mode(mode);
}

void tostf::Window::set_cursor_mode(const win_mgr::cursor_mode mode) const {
    _window->set_cursor_mode(mode);
}

int tostf::Window::get_id() const {
    return _id;
}

void tostf::Window::resize(const win_res res) {
    _window->set_res(res);
    _res = res;
    _diag_size = length(glm::vec2(_res));
}

void tostf::Window::set_minimum_size(win_res res) {
    _window->set_minimum_res(res);
}

void tostf::Window::add_resize_fun(const std::string& name, const win_mgr::window_size_callback& resize) {
    _window->add_resize_callback(name, resize);
}

void tostf::Window::add_mouse_button_fun(const std::string& name, const win_mgr::mouse_callback& fun) {
    _window->add_mouse_callback(name, fun);
}

void tostf::Window::add_scroll_fun(const std::string& name, const win_mgr::scroll_callback& fun) {
    _window->add_scroll_callback(name, fun);
}

void tostf::Window::add_key_fun(const std::string& name, const win_mgr::key_callback& fun) {
    _window->add_key_callback(name, fun);
}

void tostf::Window::add_char_fun(const std::string& name, const win_mgr::chr_callback& fun) {
    _window->add_char_callback(name, fun);
}

void tostf::Window::remove_resize_fun(const std::string& name) {
    _window->remove_resize_callback(name);
}

void tostf::Window::remove_mouse_button_fun(const std::string& name) {
    _window->remove_mouse_callback(name);
}

void tostf::Window::remove_scroll_fun(const std::string& name) {
    _window->remove_scroll_callback(name);
}

void tostf::Window::remove_key_fun(const std::string& name) {
    _window->remove_key_callback(name);
}

void tostf::Window::remove_char_fun(const std::string& name) {
    _window->remove_char_callback(name);
}

std::shared_ptr<tostf::win_mgr::win_obj> tostf::Window::get() const {
    return _window;
}

tostf::gl::Context tostf::Window::get_context() const {
    return _context;
}

tostf::win_pos tostf::Window::get_pos() const {
    return _window->get_pos();
}

void tostf::Window::set_pos(const win_pos pos) {
    _window->set_pos(pos);
}

tostf::cursor_pos tostf::Window::get_cursor_pos() const {
    return _window->get_cursor_pos();
}

void tostf::Window::set_cursor_pos(const cursor_pos pos) const {
    _window->set_cursor_pos(pos);
}

tostf::win_res tostf::Window::get_resolution() const {
    return _window->get_res();;
}

float tostf::Window::get_diag_size() const {
    return _diag_size;
}

double tostf::Window::get_scroll_y() const {
    if (ImGui::GetCurrentContext() == nullptr || !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        return _window->get_scroll_y();
    }
    return 0;
}

bool tostf::Window::check_mouse_action(const win_mgr::mouse button, const win_mgr::mouse_action action,
                                       const win_mgr::key_mod mod) const {
    if (ImGui::GetCurrentContext() == nullptr || !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        return _window->check_mouse(button, action, mod);
    }
    return false;
}

bool tostf::Window::check_key_action(const win_mgr::key key, const win_mgr::key_action action,
                                     const win_mgr::key_mod mod) const {
    if (ImGui::GetCurrentContext() == nullptr || !ImGui::IsWindowFocused(ImGuiFocusedFlags_AnyWindow)) {
        return _window->check_key(key, action, mod);
    }
    return false;
}

void tostf::Window::save_to_png(const std::filesystem::path& path) const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    auto res = _window->get_framebuffer_size();
    std::vector<unsigned char> image(res.x * res.y * 4);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(0, 0, res.x, res.y, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    log_info() << "Saving screenshot to " << path.string();
    auto future = std::async(std::launch::async, [&]() { stb::save_to_png(image, path, res, tex::format::rgba); });
}
