//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "gpu_interface/gl.hpp"
#include "win_mgr.hpp"
#include <functional>
#include <filesystem>

namespace tostf
{
    // Creates a GLFW window with an associated OpenGL context.
    // This window is necessary to display anything related to OpenGL.
    class Window {
    public:
        explicit Window(const window_config& config, const gl_settings& gl_config, win_mgr::monitor monitor = 0);
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = default;
        Window& operator=(Window&&) = default;
        virtual ~Window();
        // Sets this window as the current OpenGL context.
        void make_current() const;
        // Checks if the window close button was pressed.
        bool should_close() const;
        void close();
        void swap_buffers() const;
        void poll_events();
        void wait_events();
        void minimize() const;
        void toggle_maximize();
        bool is_maximized() const;
        bool is_minimized() const;
        void register_key_callback(const std::string& name, win_mgr::key key, win_mgr::key_action action,
                                   const std::function<void()>& func, win_mgr::key_mod mod = win_mgr::key_mod::none);
        void register_mouse_callback(const std::string& name, win_mgr::mouse button, win_mgr::mouse_action action,
                                     const std::function<void()>& func, win_mgr::key_mod mod = win_mgr::key_mod::none);
        bool check_cursor_mode(win_mgr::cursor_mode mode) const;
        void set_cursor_mode(win_mgr::cursor_mode mode) const;
        void resize(win_res res);
        void set_minimum_size(win_res res);
        // Sets the resize function which GLFW should use.
        void add_resize_fun(const std::string& name, const win_mgr::window_size_callback& resize);
        void add_mouse_button_fun(const std::string& name, const win_mgr::mouse_callback& fun);
        void add_scroll_fun(const std::string& name, const win_mgr::scroll_callback& fun);
        void add_key_fun(const std::string& name, const win_mgr::key_callback& fun);
        void add_char_fun(const std::string& name, const win_mgr::chr_callback& fun);

        void remove_resize_fun(const std::string& name);
        void remove_mouse_button_fun(const std::string& name);
        void remove_scroll_fun(const std::string& name);
        void remove_key_fun(const std::string& name);
        void remove_char_fun(const std::string& name);
        // Get the unique id of this window.
        [[maybe_unused]]
        int get_id() const;
        std::shared_ptr<win_mgr::win_obj> get() const;
        gl::Context get_context() const;
        win_pos get_pos() const;
        void set_pos(win_pos pos);
        cursor_pos get_cursor_pos() const;
        void set_cursor_pos(cursor_pos pos) const;
        win_res get_resolution() const;
        float get_diag_size() const;
        double get_scroll_y() const;
        // Checks if a certain action is performed on a certain mouse button.
        virtual bool check_mouse_action(win_mgr::mouse button, win_mgr::mouse_action action,
                                        win_mgr::key_mod mod = win_mgr::key_mod::none) const;
        // Checks if a certain action is performed on a certain keyboard key.
        virtual bool check_key_action(win_mgr::key key, win_mgr::key_action action,
                                      win_mgr::key_mod mod = win_mgr::key_mod::none) const;
        // Saves contents of this windows framebuffer to png at path
        void save_to_png(const std::filesystem::path& path) const;
    protected:
        void internal_toggle_maximize();

        // _next_id stores the id of the next window that might be created.
        static int _next_id;
        int _id;
        std::shared_ptr<win_mgr::win_obj> _window;
        win_res _res{};
        bool _decorated;
        bool _should_toggle_maximize = false;
        bool _drag = false;
        bool _should_close = false;
        double _last_action_time = 0.0;
        float _diag_size;
        float _drag_buffer{};
        gl::Context _context;
    };
}
