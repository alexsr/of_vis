//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <type_traits>
#include <future>

namespace tostf
{
    template <typename T>
    constexpr typename std::underlying_type<T>::type to_underlying_type(T value) {
        return static_cast<typename std::underlying_type<T>::type>(value);
    }

    template <typename T, typename = void>
    struct is_container : std::false_type {};

    template <typename T>
    struct is_container<T, std::void_t<decltype(std::declval<T>().data()), decltype(std::declval<T>().size())>>
        : std::true_type {};

    template <typename T>
    inline constexpr bool is_container_v = is_container<T>::value;

    template <typename... Ls>
    struct overloaded_lambda : Ls... {
        explicit overloaded_lambda(Ls&&... l) : Ls(std::move(l))... {}
    };

    auto const make_overloaded_lambda = [](auto ... ls) {
        return overloaded_lambda<decltype(ls)...>{std::move(ls)...};
    };

    template <typename T>
    bool is_future_ready(std::future<T>& t) {
        return t.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
}
