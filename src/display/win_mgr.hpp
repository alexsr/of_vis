//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "glfw.hpp"

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#undef APIENTRY
#include <windows.h>
#endif

#include "gpu_interface/gl.hpp"
#include "utility/utility.hpp"
#include "math/glm_helper.hpp"
#include <functional>
#include <map>

namespace tostf
{
    struct gl_settings {
        int major;
        int minor;
        bool debug;
        gl_profile profile;
        unsigned int samples;
        glm::vec4 color;
        std::vector<gl::cap> capabilities;
    };

    struct window_config {
        std::string title;
        unsigned int width{};
        unsigned int height{};
        int swap_interval{};
        win_mgr::window_flags flags;
    };

    using cursor_pos = glm::dvec2;
    using win_res = glm::ivec2;
    using win_pos = glm::ivec2;

    namespace win_mgr
    {
#if defined(_WIN32)
        using win32_handle_type = HWND;
#endif

        using chr_callback = std::function<void(raw_win_ptr, unsigned int)>;
        using chrmods_callback = std::function<void(raw_win_ptr, unsigned int, int)>;
        using cursor_enter_callback = std::function<void(raw_win_ptr, int)>;
        using cursor_pos_callback = std::function<void(raw_win_ptr, double, double)>;
        using drop_callback = std::function<void(raw_win_ptr, int, const char**)>;
        using framebuffer_size_callback = std::function<void(raw_win_ptr, int, int)>;
        using key_callback = std::function<void(raw_win_ptr, int, int, int, int)>;
        using mouse_callback = std::function<void(raw_win_ptr, int, int, int)>;
        using scroll_callback = std::function<void(raw_win_ptr, double, double)>;
        using window_close_callback = std::function<void(raw_win_ptr)>;
        using window_content_scale_callback = std::function<void(raw_win_ptr, float, float)>;
        using window_focus_callback = std::function<void(raw_win_ptr, int)>;
        using window_minimize_callback = std::function<void(raw_win_ptr, int)>;
        using window_maximize_callback = std::function<void(raw_win_ptr, int)>;
        using window_pos_callback = std::function<void(raw_win_ptr, int, int)>;
        using window_refresh_callback = std::function<void(raw_win_ptr)>;
        using window_size_callback = std::function<void(raw_win_ptr, int, int)>;

        struct callbacks {
            std::map<std::string, chr_callback> chr;
            std::map<std::string, chrmods_callback> chrmods;
            std::map<std::string, cursor_enter_callback> cursor_enter;
            std::map<std::string, cursor_pos_callback> cursor_pos;
            std::map<std::string, drop_callback> drop;
            std::map<std::string, framebuffer_size_callback> framebuffer_size;
            std::map<std::string, key_callback> key;
            std::map<std::string, mouse_callback> mouse;
            std::map<std::string, scroll_callback> scroll;
            std::map<std::string, window_close_callback> window_close;
            std::map<std::string, window_content_scale_callback> window_content_scale;
            std::map<std::string, window_focus_callback> window_focus;
            std::map<std::string, window_minimize_callback> window_minimize;
            std::map<std::string, window_maximize_callback> window_maximize;
            std::map<std::string, window_pos_callback> window_pos;
            std::map<std::string, window_refresh_callback> window_refresh;
            std::map<std::string, window_size_callback> window_size;
        };

        void init();
        void terminate();
        double get_time();
        raw_cursor_ptr create_cursor(cursor_type type);
        void destroy_cursor(raw_cursor_ptr cursor);
        void extern_set_clipboard_string(void* window_ptr, const char* str);
        const char* extern_get_clipboard_string(void* window_ptr);

        struct win_obj {
            win_obj(const window_config& config, monitor monitor);
            win_obj(const window_config& config, const gl_settings& gl, monitor monitor);
            void destroy() const;
            raw_win_ptr get() const;
#if defined(_WIN32)
            win32_handle_type get_win32() const;
#endif
            void set_swap_interval(int swap_interval) const;
            void swap_buffers() const;
            void poll_events();
            void wait_events();
            void make_context_current() const;
            bool should_close() const;
            bool is_minimized() const;
            bool is_maximized() const;
            bool is_focused() const;
            void minimize() const;
            void maximize() const;
            void restore() const;
            void set_res(win_res res) const;
            void set_minimum_res(win_res res) const;
            void set_pos(win_pos pos) const;
            win_res get_res() const;
            win_pos get_pos() const;
            win_res get_framebuffer_size() const;
            bool check_key(key k, key_action action, key_mod mod = key_mod::none) const;
            bool check_mouse(mouse m, mouse_action action, key_mod mod = key_mod::none) const;
            void add_mouse_callback(const std::string& name, const mouse_callback& fun);
            void add_scroll_callback(const std::string& name, const scroll_callback& fun);
            void add_key_callback(const std::string& name, const key_callback& fun);
            void add_char_callback(const std::string& name, const chr_callback& fun);
            void add_resize_callback(const std::string& name, const window_size_callback& fun);
            void remove_mouse_callback(const std::string& name);
            void remove_scroll_callback(const std::string& name);
            void remove_key_callback(const std::string& name);
            void remove_char_callback(const std::string& name);
            void remove_resize_callback(const std::string& name);

            // cursor stuff

            void set_cursor(raw_cursor_ptr c) const;
            void set_cursor_pos(cursor_pos pos);
            cursor_pos get_cursor_pos() const;
            cursor_pos get_cursor_pos_prev() const;
            cursor_pos get_cursor_motion() const;
            bool check_cursor_mode(cursor_mode m) const;
            void set_cursor_mode(cursor_mode m);
            double get_scroll_y() const;
        private:
            void create_window(const window_config& config, monitor monitor);
            bool check_key_action(key k, key_action a) const;
            bool check_mouse_action(mouse m, mouse_action a) const;
            bool check_mod(key_mod m) const;

            raw_win_ptr _ptr = nullptr;
            cursor_pos _cursor_pos{};
            cursor_pos _last_cursor_pos{};
            callbacks _callbacks;
            double _scroll_y{};
            void init_callbacks();
        };

        constexpr key_mod operator|(const key_mod m1, const key_mod m2) {
            return static_cast<key_mod>(to_underlying_type(m1) | to_underlying_type(m2));
        }

        constexpr bool operator&(const key_mod m1, const key_mod m2) {
            return (to_underlying_type(m1) & to_underlying_type(m2)) != 0;
        }

        constexpr window_flags operator|(const window_flags m1, const window_flags m2) {
            return static_cast<window_flags>(to_underlying_type(m1) | to_underlying_type(m2));
        }

        constexpr bool operator&(const window_flags m1, const window_flags m2) {
            return (to_underlying_type(m1) & to_underlying_type(m2)) != 0;
        }

        constexpr window_flags default_window_flags =
            window_flags::focused | window_flags::resizable | window_flags::auto_iconify;
    }
}
