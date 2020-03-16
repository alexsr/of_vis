//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "utility/utility.hpp"
#include "math/glm_helper.hpp"
#include <sstream>
#include <filesystem>
#include <array>

#if __has_include(<windows.h>)
#undef APIENTRY
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace tostf
{
    namespace cmd
    {
#if __has_include(<windows.h>)
        // Color support is not enabled by default in the windows terminal
        // Therefore this method has to be called before using colors and styles.
        inline void enable_color() {
            const auto out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD md = 0;
            GetConsoleMode(out_handle, &md);
            SetConsoleMode(out_handle, md | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        }
#else
        // This method is not necessary on anything but windows.
        inline void enable_color() { }
#endif
        // Its value corresponds to the equivalent ansi color code
        enum class color {
            black = 30,
            red = 31,
            green = 32,
            yellow = 33,
            blue = 34,
            purple = 35,
            cyan = 36,
            white = 37,
            h_black = 90,
            h_red = 91,
            h_green = 92,
            h_yellow = 93,
            h_blue = 94,
            h_purple = 95,
            h_cyan = 96,
            h_white = 97
        };

        // Its value corresponds to the equivalent ansi color code
        enum class bg_color {
            black = 40,
            red = 41,
            green = 42,
            yellow = 43,
            blue = 44,
            purple = 45,
            cyan = 46,
            white = 47,
            h_black = 100,
            h_red = 101,
            h_green = 102,
            h_yellow = 103,
            h_blue = 104,
            h_purple = 105,
            h_cyan = 106,
            h_white = 107
        };

        // Its value corresponds to the equivalent ansi code
        enum class style {
            regular = 0,
            bright = 1,
            underline = 4
        };

        // It is necessary to combine color and style to prevent accidential resetting
        // when using style::regular. to_ansi(style::regular) translates to \033[0m which resets the formatting.
        // to_ansi of Color_style is able to circumvent this issue because it can combine style and color into
        // one ansi formatting code.
        struct Color_style {
            color c;
            style s;
        };

        // It is necessary to combine bg_color and style to prevent accidential resetting
        // when using style::regular. to_ansi(style::regular) translates to \033[0m which resets the formatting.
        // to_ansi of Bg_color_style is able to circumvent this issue because it can combine style and bg_color into
        // one ansi formatting code.
        struct Bg_color_style {
            bg_color c;
            style s;
        };

        // Combines color and style into less error prone structure.
        constexpr Color_style operator<<(const color&& c, const style&& s) {
            return {c, s};
        }

        // Combines bg_color and style into less error prone structure.
        constexpr Bg_color_style operator<<(const bg_color&& c, const style&& s) {
            return {c, s};
        }

        static constexpr const char* reset_style() {
            return "\033[0m";
        }

        // Converts color style to ansi code as char array
        static constexpr std::array<char, 8> to_ansi(const Color_style& c_s) {
            const std::array<char, 8> ansi_code{
                '\033', '[',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c_s.s)),
                ';',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c_s.c) / 10),
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c_s.c) % 10),
                'm'
            };
            return ansi_code;
        }

        // Converts bg_color style to ansi code as char array
        static constexpr std::array<char, 9> to_ansi(const Bg_color_style& bg_s) {
            const std::array<char, 9> ansi_code{
                '\033', '[',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(bg_s.s)),
                ';',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(bg_s.c) / 100),
                static_cast<char>(static_cast<int>('0') + to_underlying_type(bg_s.c) % 100 / 10),
                static_cast<char>(static_cast<int>('0') + to_underlying_type(bg_s.c) % 10
                ),
                'm'
            };
            return ansi_code;
        }

        // Converts color to ansi code as char array
        static constexpr std::array<char, 6> to_ansi(const color& c) {
            const std::array<char, 6> ansi_code{
                '\033', '[',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c) / 10),
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c) % 10),
                'm'
            };
            return ansi_code;
        }

        // Converts bg_color to ansi code as char array
        static constexpr std::array<char, 7> to_ansi(const bg_color& c) {
            const std::array<char, 7> ansi_code{
                '\033', '[',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c) / 100),
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c) % 100 / 10),
                static_cast<char>(static_cast<int>('0') + to_underlying_type(c) % 10),
                'm'
            };
            return ansi_code;
        }

        // Converts style to ansi code as char array
        static constexpr std::array<char, 5> to_ansi(const style& s) {
            const std::array<char, 5> ansi_code{
                '\033', '[',
                static_cast<char>(static_cast<int>('0') + to_underlying_type(s)),
                'm'
            };
            return ansi_code;
        }
    }

    // A log which stores its contents in a ostringstream and writes its contents
    // to std::cout or a file when destroyed.
    class Logger {
    public:
        // Creates a Logger which will write to cout when destroyed.
        // _will_be_moved prevents that a newline will be appended when the logger is destroyed
        // while being moved.
        explicit Logger(bool will_be_moved = false);
        // Creates a Logger which will write to a file at path out.
        // _will_be_moved prevents that a newline will be appended when the logger is destroyed
        // while being moved.
        explicit Logger(std::filesystem::path out, bool moved = false);
        Logger(const Logger& other) = delete;
        Logger& operator=(const Logger& a) = delete;
        Logger(Logger&&) noexcept;
        Logger& operator=(Logger&&) noexcept;
        ~Logger();
        template <typename T>
        Logger& operator<<(const T&& v);
        template <typename T>
        Logger& operator<<(const T& v);
        template <glm::length_t L, typename T, glm::qualifier Q>
        Logger& operator<<(const glm::vec<L, T, Q>&& v);
        template <glm::length_t L, typename T, glm::qualifier Q>
        Logger& operator<<(const glm::vec<L, T, Q>& v);

    protected:
        bool _to_file;
        bool _will_be_moved;
        std::filesystem::path _out;
        std::ostringstream _oss;
    };

    template <typename T>
    Logger& Logger::operator<<(const T&& v) {
        _oss << v;
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::Color_style>(const cmd::Color_style&& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::Bg_color_style>(const cmd::Bg_color_style&& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::color>(const cmd::color&& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::bg_color>(const cmd::bg_color&& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::style>(const cmd::style&& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <typename T>
    Logger& Logger::operator<<(const T& v) {
        _oss << v;
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::Color_style>(const cmd::Color_style& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::Bg_color_style>(const cmd::Bg_color_style& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::color>(const cmd::color& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::bg_color>(const cmd::bg_color& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <>
    inline Logger& Logger::operator<<<cmd::style>(const cmd::style& v) {
        _oss << to_ansi(v).data();
        return *this;
    }

    template <glm::length_t L, typename T, glm::qualifier Q>
    Logger& Logger::operator<<(const glm::vec<L, T, Q>&& v) {
        _oss << glm::to_string(v);
        return *this;
    }

    template <glm::length_t L, typename T, glm::qualifier Q>
    Logger& Logger::operator<<(const glm::vec<L, T, Q>& v) {
        _oss << glm::to_string(v);
        return *this;
    }


    // Starts log which writes to std::cout. Line endings are appended.
    // Example usage:
    // log() << "Put your string here";
    Logger log();
    // Starts log with predefined style which writes to std::cout. Line endings are appended.
    // Example usage:
    // log(cmd::color::blue) << "Put your string here";
    Logger log(cmd::color c, cmd::style s = cmd::style::regular, cmd::bg_color b = cmd::bg_color::black);
    // Starts log writing to a file at path p. Line endings are appended.
    // Example usage:
    // log("path.txt") << "Put your string here";
    Logger log(const std::filesystem::path& p);
    // Prepends [tag] with bg_color to input stream of log
    Logger log_tag(const std::string& tag, cmd::bg_color bg_color, size_t max_length = 10);
    // Prepends [INFO] tag with cyan background to input stream of log
    Logger log_info();
    // Prepends [ERROR] tag with red background to input stream of log
    Logger log_error();
    // Prepends [SUCCESS] tag with green background to input stream of log
    Logger log_success();
    // Prepends [WARNING] tag with green background to input stream of log
    Logger log_warning();
    // Prepends [TIME] tag with magenta background to input stream of log
    Logger log_time();
    // Starts logging section with a banner of bg_color and colored text with prepended newline
    void log_section(const std::string& title, cmd::bg_color bg_color = cmd::bg_color::h_blue,
                     cmd::color color = cmd::color::h_white, size_t max_length = 80);
    // Starts logging section with a banner of bg_color and colored text without prepending newline;
    void log_section_no_newline(const std::string& title, cmd::bg_color bg_color = cmd::bg_color::h_blue,
                                cmd::color color = cmd::color::h_white, size_t max_length = 80);
}
