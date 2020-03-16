//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#include <chrono>
#include <string>

namespace tostf
{
    template <typename T>
    struct is_duration : std::false_type {};

    template <typename T, typename P>
    struct is_duration<std::chrono::duration<T, P>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_duration_v = is_duration<T>::value;

    using high_res_clock = std::chrono::high_resolution_clock;
    using steady_time_point = std::chrono::time_point<std::chrono::steady_clock>;

    template <typename T>
    std::string get_duration_string() {
        return "";
    }

    template <>
    inline std::string get_duration_string<std::chrono::nanoseconds>() {
        return "ns";
    }

    template <>
    inline std::string get_duration_string<std::chrono::microseconds>() {
        return u8"µs";
    }

    template <>
    inline std::string get_duration_string<std::chrono::milliseconds>() {
        return "ms";
    }

    template <>
    inline std::string get_duration_string<std::chrono::seconds>() {
        return "s";
    }

    template <>
    inline std::string get_duration_string<std::chrono::minutes>() {
        return "m";
    }

    template <>
    inline std::string get_duration_string<std::chrono::hours>() {
        return "h";
    }
}
