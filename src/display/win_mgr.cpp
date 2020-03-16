//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//


#include "win_mgr.hpp"
#define GLFW_INCLUDE_NONE
#define _GLFW_USE_HYBRID_HPG
#include <glfw/glfw3.h>
#include <stdexcept>
#include "utility/logging.hpp"

#if defined(_WIN32)
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>   // for glfwGetWin32Window
#endif

// global functions

void tostf::win_mgr::init() {
    const auto glfw_error = glfwInit();
    if (glfw_error == 0) {
        throw std::runtime_error{"Failed to initialize GLFW."};
    }
}

void tostf::win_mgr::terminate() {
    glfwTerminate();
}

double tostf::win_mgr::get_time() {
    return glfwGetTime();
}

tostf::win_mgr::raw_cursor_ptr tostf::win_mgr::create_cursor(const cursor_type type) {
    return glfwCreateStandardCursor(to_underlying_type(type));
}

void tostf::win_mgr::destroy_cursor(const raw_cursor_ptr cursor) {
    glfwDestroyCursor(cursor);
}

void tostf::win_mgr::extern_set_clipboard_string(void* window_ptr, const char* str) {
    glfwSetClipboardString(static_cast<raw_win_ptr>(window_ptr), str);
}

const char* tostf::win_mgr::extern_get_clipboard_string(void* window_ptr) {
    return glfwGetClipboardString(static_cast<GLFWwindow*>(window_ptr));
}

// utility methods for readability

namespace
{
    void set_gl_attr(const tostf::win_mgr::gl_attr attr, const int value) {
        glfwWindowHint(tostf::to_underlying_type(attr), value);
    }

    void enable_msaa(const unsigned int samples) {
        set_gl_attr(tostf::win_mgr::gl_attr::samples, samples);
    }

    void enable_debug() {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    }

    void set_gl_version(const int major, const int minor) {
        set_gl_attr(tostf::win_mgr::gl_attr::major, major);
        set_gl_attr(tostf::win_mgr::gl_attr::minor, minor);
    }

    void set_gl_profile(const tostf::gl_profile value) {
        set_gl_attr(tostf::win_mgr::gl_attr::profile, to_underlying_type(value));
    }
}

//window functions
tostf::win_mgr::win_obj::win_obj(const window_config& config, const monitor monitor) {
    create_window(config, monitor);
}

tostf::win_mgr::win_obj::win_obj(const window_config& config, const gl_settings& gl, const monitor monitor) {
    if (gl.debug) {
        enable_debug();
    }
    create_window(config, monitor);
    set_gl_version(gl.major, gl.minor);
    set_gl_profile(gl.profile);
    if (gl.samples > 0) {
        enable_msaa(gl.samples);
    }
    make_context_current();
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        throw std::runtime_error{"Failed to initialize OpenGL."};
    }
    gl::set_viewport(config.width, config.height);
    enable(gl.capabilities);
    gl::set_clear_color(gl.color);
}

void tostf::win_mgr::win_obj::create_window(const window_config& config, const monitor monitor) {
    GLFWmonitor* monitor_ptr = glfwGetPrimaryMonitor();
    if (monitor != 0) {
        int count = 0;
        const auto monitors = glfwGetMonitors(&count);
        if (static_cast<int>(monitor) < count - 1) {
            monitor_ptr = monitors[monitor];
        }
    }
    if (config.flags & window_flags::borderless) {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }
    auto vid_mode = glfwGetVideoMode(monitor_ptr);
    if (!(config.flags & window_flags::fullscreen)) {
        monitor_ptr = nullptr;
    }
    _ptr = glfwCreateWindow(config.width, config.height, config.title.c_str(), monitor_ptr, nullptr);
    if (_ptr == nullptr) {
        terminate();
        throw std::runtime_error{"Failed to create window."};
    }
    const win_pos window_pos{(vid_mode->width - config.width) / 2, (vid_mode->height - config.height) / 2};
    set_pos(window_pos);
    if (config.flags & window_flags::always_top) {
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    }
    if (!(config.flags & window_flags::focused)) {
        glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    }
    if (config.flags & window_flags::maximized) {
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);
    }
    if (!(config.flags & window_flags::auto_iconify)) {
        glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
    }
    if (!(config.flags & window_flags::resizable)) {
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }
    if (config.flags & window_flags::hidden) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    set_swap_interval(config.swap_interval);
    add_scroll_callback("default", [this](GLFWwindow*, const double, const double y) {
        this->_scroll_y = y;
    });
    init_callbacks();
}


void tostf::win_mgr::win_obj::destroy() const {
    glfwDestroyWindow(_ptr);
}

#if defined(_WIN32)
tostf::win_mgr::win32_handle_type tostf::win_mgr::win_obj::get_win32() const {
    return glfwGetWin32Window(_ptr);
}
#endif

void tostf::win_mgr::win_obj::set_swap_interval(const int swap_interval) const {
    make_context_current();
    glfwSwapInterval(swap_interval);
}

void tostf::win_mgr::win_obj::swap_buffers() const {
    glfwSwapBuffers(_ptr);
}

void tostf::win_mgr::win_obj::poll_events() {
    _last_cursor_pos = _cursor_pos;
    _scroll_y = 0;
    glfwGetCursorPos(_ptr, &_cursor_pos.x, &_cursor_pos.y);
    glfwPollEvents();
}

void tostf::win_mgr::win_obj::wait_events() {
    glfwWaitEvents();
}

void tostf::win_mgr::win_obj::make_context_current() const {
    glfwMakeContextCurrent(_ptr);
}

bool tostf::win_mgr::win_obj::should_close() const {
    return glfwWindowShouldClose(_ptr);
}

bool tostf::win_mgr::win_obj::is_minimized() const {
    return glfwGetWindowAttrib(_ptr, GLFW_ICONIFIED) == GLFW_TRUE;
}

bool tostf::win_mgr::win_obj::is_maximized() const {
    return glfwGetWindowAttrib(_ptr, GLFW_MAXIMIZED) == GLFW_TRUE;
}

bool tostf::win_mgr::win_obj::is_focused() const {
    return glfwGetWindowAttrib(_ptr, GLFW_FOCUSED);
}

void tostf::win_mgr::win_obj::minimize() const {
    glfwIconifyWindow(_ptr);
}

void tostf::win_mgr::win_obj::maximize() const {
    glfwMaximizeWindow(_ptr);
}

void tostf::win_mgr::win_obj::restore() const {
    glfwRestoreWindow(_ptr);
}

void tostf::win_mgr::win_obj::set_res(const win_res res) const {
    glfwSetWindowSize(_ptr, res.x, res.y);
}

void tostf::win_mgr::win_obj::set_minimum_res(win_res res) const {
    glfwSetWindowSizeLimits(_ptr, res.x, res.y, GLFW_DONT_CARE, GLFW_DONT_CARE);
}

void tostf::win_mgr::win_obj::set_pos(const win_pos pos) const {
    glfwSetWindowPos(_ptr, pos.x, pos.y);
}

tostf::win_res tostf::win_mgr::win_obj::get_res() const {
    win_res res;
    glfwGetWindowSize(_ptr, &res.x, &res.y);
    return res;
}

tostf::win_pos tostf::win_mgr::win_obj::get_pos() const {
    win_pos pos;
    glfwGetWindowPos(_ptr, &pos.x, &pos.y);
    return pos;
}

tostf::win_res tostf::win_mgr::win_obj::get_framebuffer_size() const {
    win_res res;
    glfwGetFramebufferSize(_ptr, &res.x, &res.y);
    return res;
}

bool tostf::win_mgr::win_obj::check_key_action(const key k, const key_action a) const {
    return glfwGetKey(_ptr, to_underlying_type(k)) == to_underlying_type(a);
}

bool tostf::win_mgr::win_obj::check_mouse_action(const mouse m, const mouse_action a) const {
    return glfwGetMouseButton(_ptr, to_underlying_type(m) == to_underlying_type(a));
}

bool tostf::win_mgr::win_obj::check_mod(const key_mod m) const {
    bool mods_active = true;
    if (m & key_mod::shift) {
        mods_active = mods_active && (check_key_action(key::left_shift, key_action::press)
                                      || check_key_action(key::right_shift, key_action::press));
    }
    if (m & key_mod::ctrl) {
        mods_active = mods_active && (check_key_action(key::left_ctrl, key_action::press)
                                      || check_key_action(key::right_ctrl, key_action::press));
    }
    if (m & key_mod::alt) {
        mods_active = mods_active && (check_key_action(key::left_alt, key_action::press)
                                      || check_key_action(key::right_alt, key_action::press));
    }
    if (m & key_mod::super) {
        mods_active = mods_active && (check_key_action(key::left_super, key_action::press)
                                      || check_key_action(key::right_super, key_action::press));
    }
    return mods_active;
}

void tostf::win_mgr::win_obj::set_cursor(const raw_cursor_ptr c) const {
    glfwSetCursor(_ptr, c);
}

void tostf::win_mgr::win_obj::set_cursor_pos(const cursor_pos pos) {
    glfwSetCursorPos(_ptr, pos.x, pos.y);
    _cursor_pos = pos;
}

tostf::cursor_pos tostf::win_mgr::win_obj::get_cursor_pos() const {
    return _cursor_pos;
}

tostf::cursor_pos tostf::win_mgr::win_obj::get_cursor_pos_prev() const {
    return _last_cursor_pos;
}

tostf::cursor_pos tostf::win_mgr::win_obj::get_cursor_motion() const {
    return _cursor_pos - _last_cursor_pos;
}

bool tostf::win_mgr::win_obj::check_cursor_mode(const cursor_mode m) const {
    return glfwGetInputMode(_ptr, GLFW_CURSOR) == to_underlying_type(m);
}

void tostf::win_mgr::win_obj::set_cursor_mode(const cursor_mode m) {
    glfwSetInputMode(_ptr, GLFW_CURSOR, to_underlying_type(m));
}

double tostf::win_mgr::win_obj::get_scroll_y() const {
    return _scroll_y;
}

void tostf::win_mgr::win_obj::init_callbacks() {
    glfwSetWindowUserPointer(_ptr, static_cast<void*>(&_callbacks));
    if (!_callbacks.chr.empty()) {
        glfwSetCharCallback(_ptr, [](GLFWwindow* w, const unsigned c) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->chr) {
                func(w, c);
            }
        });
    }
    if (!_callbacks.chrmods.empty()) {
        glfwSetCharModsCallback(_ptr, [](GLFWwindow* w, const unsigned c, const int m) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->chrmods) {
                func(w, c, m);
            }
        });
    }
    if (!_callbacks.cursor_enter.empty()) {
        glfwSetCursorEnterCallback(_ptr, [](GLFWwindow* w, const int e) {
            for (auto& [k,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->cursor_enter) {
                func(w, e);
            }
        });
    }
    if (!_callbacks.cursor_pos.empty()) {
        glfwSetCursorPosCallback(_ptr, [](GLFWwindow* w, const double x, const double y) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->cursor_pos) {
                func(w, x, y);
            }
        });
    }
    if (!_callbacks.drop.empty()) {
        glfwSetDropCallback(_ptr, [](GLFWwindow* w, const int c, const char** f) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->drop) {
                func(w, c, f);
            }
        });
    }
    if (!_callbacks.framebuffer_size.empty()) {
        glfwSetFramebufferSizeCallback(_ptr, [](GLFWwindow* w, const int x, const int y) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->framebuffer_size) {
                func(w, x, y);
            }
        });
    }
    if (!_callbacks.key.empty()) {
        glfwSetKeyCallback(_ptr, [](GLFWwindow* w, const int k, const int s, const int a, const int m) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->key) {
                func(w, k, s, a, m);
            }
        });
    }
    if (!_callbacks.mouse.empty()) {
        glfwSetMouseButtonCallback(_ptr, [](GLFWwindow* w, const int k, const int a, const int m) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->mouse) {
                func(w, k, a, m);
            }
        });
    }
    if (!_callbacks.scroll.empty()) {
        glfwSetScrollCallback(_ptr, [](GLFWwindow* w, const double x, const double y) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->scroll) {
                func(w, x, y);
            }
        });
    }
    if (!_callbacks.window_close.empty()) {
        glfwSetWindowCloseCallback(_ptr, [](GLFWwindow* w) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_close) {
                func(w);
            }
        });
    }
    if (!_callbacks.window_content_scale.empty()) {
        glfwSetWindowContentScaleCallback(_ptr, [](GLFWwindow* w, const float x, const float y) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_content_scale) {
                func(w, x, y);
            }
        });
    }
    if (!_callbacks.window_focus.empty()) {
        glfwSetWindowFocusCallback(_ptr, [](GLFWwindow* w, const int f) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_focus) {
                func(w, f);
            }
        });
    }
    if (!_callbacks.window_minimize.empty()) {
        glfwSetWindowIconifyCallback(_ptr, [](GLFWwindow* w, const int i) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_minimize) {
                func(w, i);
            }
        });
    }
    if (!_callbacks.window_maximize.empty()) {
        glfwSetWindowMaximizeCallback(_ptr, [](GLFWwindow* w, const int m) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_maximize) {
                func(w, m);
            }
        });
    }
    if (!_callbacks.window_pos.empty()) {
        glfwSetWindowPosCallback(_ptr, [](GLFWwindow* w, const int x, const int y) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_pos) {
                func(w, x, y);
            }
        });
    }
    if (!_callbacks.window_refresh.empty()) {
        glfwSetWindowRefreshCallback(_ptr, [](GLFWwindow* w) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_refresh) {
                func(w);
            }
        });
    }
    if (!_callbacks.window_size.empty()) {
        glfwSetWindowSizeCallback(_ptr, [](GLFWwindow* w, const int x, const int y) {
            for (auto& [key,func] : static_cast<callbacks*>(glfwGetWindowUserPointer(w))->window_size) {
                func(w, x, y);
            }
        });
    }
}

tostf::win_mgr::raw_win_ptr tostf::win_mgr::win_obj::get() const {
    return _ptr;
}

bool tostf::win_mgr::win_obj::check_key(const key k, const key_action action, const key_mod mod) const {
    if (!check_key_action(k, action)) {
        return false;
    }
    return check_mod(mod);
}

bool tostf::win_mgr::win_obj::check_mouse(const mouse m, const mouse_action action, const key_mod mod) const {
    if (!check_mouse_action(m, action)) {
        return false;
    }
    return check_mod(mod);
}

void tostf::win_mgr::win_obj::add_mouse_callback(const std::string& name, const mouse_callback& fun) {
    const auto res = _callbacks.mouse.insert_or_assign(name, fun);
    init_callbacks();
    if (res.second) {
        log_info() << "Mouse callback \"" << name << "\" registered.";
    }
    else {
        log_warning() << "Mouse callback \"" << name << "\" was overwritten.";
    }
}

void tostf::win_mgr::win_obj::add_scroll_callback(const std::string& name, const scroll_callback& fun) {
    const auto res = _callbacks.scroll.insert_or_assign(name, fun);
    init_callbacks();
    if (res.second) {
        log_info() << "Scroll callback \"" << name << "\" registered.";
    }
    else {
        log_warning() << "Scroll callback \"" << name << "\" was overwritten.";
    }
}

void tostf::win_mgr::win_obj::add_key_callback(const std::string& name, const key_callback& fun) {
    const auto res = _callbacks.key.insert_or_assign(name, fun);
    init_callbacks();
    if (res.second) {
        log_info() << "Key callback \"" << name << "\" registered.";
    }
    else {
        log_warning() << "Key callback \"" << name << "\" was overwritten.";
    }
}

void tostf::win_mgr::win_obj::add_char_callback(const std::string& name, const chr_callback& fun) {
    const auto res = _callbacks.chr.insert_or_assign(name, fun);
    init_callbacks();
    if (res.second) {
        log_info() << "Char callback \"" << name << "\" registered.";
    }
    else {
        log_warning() << "Char callback \"" << name << "\" was overwritten.";
    }
}

void tostf::win_mgr::win_obj::add_resize_callback(const std::string& name, const window_size_callback& fun) {
    const auto res = _callbacks.window_size.insert_or_assign(name, fun);
    init_callbacks();
    if (res.second) {
        log_info() << "Resize callback \"" << name << "\" registered.";
    }
    else {
        log_warning() << "Resize callback \"" << name << "\" was overwritten.";
    }
}

void tostf::win_mgr::win_obj::remove_mouse_callback(const std::string& name) {
    const auto it = _callbacks.mouse.find(name);
    if (it != _callbacks.mouse.end()) {
        _callbacks.mouse.erase(it);
        log_info() << "Mouse callback \"" << name << "\" was removed.";
    }
    else {
        log_warning() << "Mouse callback \"" << name << "\" could not be found.";
    }
}

void tostf::win_mgr::win_obj::remove_scroll_callback(const std::string& name) {
    const auto it = _callbacks.scroll.find(name);
    if (it != _callbacks.scroll.end()) {
        _callbacks.scroll.erase(it);
        log_info() << "Scroll callback \"" << name << "\" was removed.";
    }
    else {
        log_warning() << "Scroll callback \"" << name << "\" could not be found.";
    }
}

void tostf::win_mgr::win_obj::remove_key_callback(const std::string& name) {
    const auto it = _callbacks.key.find(name);
    if (it != _callbacks.key.end()) {
        _callbacks.key.erase(it);
        log_info() << "Key callback \"" << name << "\" was removed.";
    }
    else {
        log_warning() << "Key callback \"" << name << "\" could not be found.";
    }
}

void tostf::win_mgr::win_obj::remove_char_callback(const std::string& name) {
    const auto it = _callbacks.chr.find(name);
    if (it != _callbacks.chr.end()) {
        _callbacks.chr.erase(it);
        log_info() << "Char callback \"" << name << "\" was removed.";
    }
    else {
        log_warning() << "Char callback \"" << name << "\" could not be found.";
    }
}

void tostf::win_mgr::win_obj::remove_resize_callback(const std::string& name) {
    const auto it = _callbacks.window_size.find(name);
    if (it != _callbacks.window_size.end()) {
        _callbacks.window_size.erase(it);
        log_info() << "Resize callback \"" << name << "\" was removed.";
    }
    else {
        log_warning() << "Resize callback \"" << name << "\" could not be found.";
    }
}
