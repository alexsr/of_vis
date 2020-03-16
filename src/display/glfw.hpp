//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#define GLFW_INCLUDE_NONE
#define _GLFW_USE_HYBRID_HPG
#include <glfw/glfw3.h>

namespace tostf
{
    enum class gl_profile {
        core = GLFW_OPENGL_CORE_PROFILE,
        compat = GLFW_OPENGL_COMPAT_PROFILE,
        any = GLFW_OPENGL_ANY_PROFILE,
    };

    namespace win_mgr
    {
        using monitor = unsigned int; // GLFWmonitor
        using raw_win_ptr = GLFWwindow *;
        using raw_cursor_ptr = GLFWcursor *;

        enum class gl_attr {
            major = GLFW_CONTEXT_VERSION_MAJOR,
            minor = GLFW_CONTEXT_VERSION_MINOR,
            profile = GLFW_OPENGL_PROFILE,
            stereo = GLFW_STEREO,
            samples = GLFW_SAMPLES,
            srgb = GLFW_SRGB_CAPABLE,
            double_buffer = GLFW_DOUBLEBUFFER,
        };

        enum class window_flags {
            none = 0,
            hidden = 1,
            borderless = 2,
            auto_iconify = 4,
            always_top = 8,
            maximized = 16,
            fullscreen = 32,
            focused = 64,
            resizable = 128,
        };

        enum class cursor_mode {
            disabled = GLFW_CURSOR_DISABLED,
            normal = GLFW_CURSOR_NORMAL,
            hidden = GLFW_CURSOR_HIDDEN
        };

        enum class cursor_type {
            arrow = GLFW_ARROW_CURSOR,
            ibeam = GLFW_IBEAM_CURSOR,
            resize_all = GLFW_ARROW_CURSOR,
            resize_ns = GLFW_VRESIZE_CURSOR,
            resize_we = GLFW_HRESIZE_CURSOR,
            resize_nesw = GLFW_ARROW_CURSOR,
            resize_nwse = GLFW_ARROW_CURSOR,
            hand = GLFW_HAND_CURSOR,
            wait = GLFW_ARROW_CURSOR,
            crosshair = GLFW_ARROW_CURSOR,
            wait_arrow = GLFW_ARROW_CURSOR,
            no = GLFW_ARROW_CURSOR,
        };

        enum class mouse {
            left = GLFW_MOUSE_BUTTON_LEFT,
            right = GLFW_MOUSE_BUTTON_RIGHT,
            middle = GLFW_MOUSE_BUTTON_MIDDLE
        };

        enum class mouse_action {
            press = GLFW_PRESS,
            release = GLFW_RELEASE
        };

        constexpr mouse to_mouse_button(const int i) {
            switch (i) {
            case 0:
                return mouse::left;
            case 1:
                return mouse::right;
            case 2:
                return mouse::middle;
            default:
                return mouse::left;
            }
        }

        enum class key : int {
            unknown = GLFW_KEY_UNKNOWN,
            space = GLFW_KEY_SPACE,
            apostrophe = GLFW_KEY_APOSTROPHE,
            comma = GLFW_KEY_COMMA,
            minus = GLFW_KEY_MINUS,
            period = GLFW_KEY_PERIOD,
            slash = GLFW_KEY_SLASH,
            zero = GLFW_KEY_0,
            one = GLFW_KEY_1,
            two = GLFW_KEY_2,
            three = GLFW_KEY_3,
            four = GLFW_KEY_4,
            five = GLFW_KEY_5,
            six = GLFW_KEY_6,
            seven = GLFW_KEY_7,
            eight = GLFW_KEY_8,
            nine = GLFW_KEY_9,
            semicolon = GLFW_KEY_SEMICOLON,
            equal = GLFW_KEY_EQUAL,
            a = GLFW_KEY_A,
            b = GLFW_KEY_B,
            c = GLFW_KEY_C,
            d = GLFW_KEY_D,
            e = GLFW_KEY_E,
            f = GLFW_KEY_F,
            g = GLFW_KEY_G,
            h = GLFW_KEY_H,
            i = GLFW_KEY_I,
            j = GLFW_KEY_J,
            k = GLFW_KEY_K,
            l = GLFW_KEY_L,
            m = GLFW_KEY_M,
            n = GLFW_KEY_N,
            o = GLFW_KEY_O,
            p = GLFW_KEY_P,
            q = GLFW_KEY_Q,
            r = GLFW_KEY_R,
            s = GLFW_KEY_S,
            t = GLFW_KEY_T,
            u = GLFW_KEY_U,
            v = GLFW_KEY_V,
            w = GLFW_KEY_W,
            x = GLFW_KEY_X,
            y = GLFW_KEY_Y,
            z = GLFW_KEY_Z,
            left_bracket = GLFW_KEY_LEFT_BRACKET,
            backslash = GLFW_KEY_BACKSLASH,
            right_bracket = GLFW_KEY_RIGHT_BRACKET,
            grave_accent = GLFW_KEY_GRAVE_ACCENT,
            escape = GLFW_KEY_ESCAPE,
            enter = GLFW_KEY_ENTER,
            tab = GLFW_KEY_TAB,
            backspace = GLFW_KEY_BACKSPACE,
            insert = GLFW_KEY_INSERT,
            del = GLFW_KEY_DELETE,
            right = GLFW_KEY_RIGHT,
            left = GLFW_KEY_LEFT,
            down = GLFW_KEY_DOWN,
            up = GLFW_KEY_UP,
            page_up = GLFW_KEY_PAGE_UP,
            page_down = GLFW_KEY_PAGE_DOWN,
            home = GLFW_KEY_HOME,
            end = GLFW_KEY_END,
            caps_lock = GLFW_KEY_CAPS_LOCK,
            scroll_lock = GLFW_KEY_SCROLL_LOCK,
            num_lock = GLFW_KEY_NUM_LOCK,
            print_screen = GLFW_KEY_PRINT_SCREEN,
            pause = GLFW_KEY_PAUSE,
            f1 = GLFW_KEY_F1,
            f2 = GLFW_KEY_F2,
            f3 = GLFW_KEY_F3,
            f4 = GLFW_KEY_F4,
            f5 = GLFW_KEY_F5,
            f6 = GLFW_KEY_F6,
            f7 = GLFW_KEY_F7,
            f8 = GLFW_KEY_F8,
            f9 = GLFW_KEY_F9,
            f10 = GLFW_KEY_F10,
            f11 = GLFW_KEY_F11,
            f12 = GLFW_KEY_F12,
            kp_0 = GLFW_KEY_KP_0,
            kp_1 = GLFW_KEY_KP_1,
            kp_2 = GLFW_KEY_KP_2,
            kp_3 = GLFW_KEY_KP_3,
            kp_4 = GLFW_KEY_KP_4,
            kp_5 = GLFW_KEY_KP_5,
            kp_6 = GLFW_KEY_KP_6,
            kp_7 = GLFW_KEY_KP_7,
            kp_8 = GLFW_KEY_KP_8,
            kp_9 = GLFW_KEY_KP_9,
            kp_decimal = GLFW_KEY_KP_DECIMAL,
            kp_divide = GLFW_KEY_KP_DIVIDE,
            kp_multiply = GLFW_KEY_KP_MULTIPLY,
            kp_substract = GLFW_KEY_KP_SUBTRACT,
            kp_add = GLFW_KEY_KP_ADD,
            kp_enter = GLFW_KEY_KP_ENTER,
            kp_equal = GLFW_KEY_KP_EQUAL,
            left_shift = GLFW_KEY_LEFT_SHIFT,
            left_ctrl = GLFW_KEY_LEFT_CONTROL,
            left_alt = GLFW_KEY_LEFT_ALT,
            left_super = GLFW_KEY_LEFT_SUPER,
            right_shift = GLFW_KEY_RIGHT_SHIFT,
            right_ctrl = GLFW_KEY_RIGHT_CONTROL,
            right_alt = GLFW_KEY_RIGHT_ALT,
            right_super = GLFW_KEY_RIGHT_SUPER,
            menu = GLFW_KEY_MENU
        };

        enum class key_mod : int {
            none = 0,
            shift = GLFW_MOD_SHIFT,
            ctrl = GLFW_MOD_CONTROL,
            alt = GLFW_MOD_ALT,
            super = GLFW_MOD_SUPER
        };

        enum class key_action : int {
            press = GLFW_PRESS,
            release = GLFW_RELEASE,
            repeat = GLFW_REPEAT
        };
    }
}
