//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <random>
#include "math/glm_helper.hpp"

namespace tostf
{
    template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    T gen_random_int_value(T lower, T upper) {
        std::random_device rd;
        std::mt19937 rng(rd());
        const std::uniform_int_distribution<T> uni(lower, upper);
        return uni(rng);
    }

    template <typename T, typename = std::enable_if_t<std::is_floating_point_v<T>>>
    T gen_random_float_value(T lower, T upper) {
        std::random_device rd;
        std::mt19937 rng(rd());
        const std::uniform_real_distribution<T> uni(lower, upper);
        return uni(rng);
    }

    template <typename T, typename = std::enable_if_t<is_glm_vec_v<T>
                                                      && std::is_floating_point_v<typename T::value_type>>>
    T gen_random_vec(T lower, T upper, typename T::value_type alpha = 1.0f) {
        T res{};
        for (int i = 0; i < T::length(); i++) {
            res[i] = gen_random_float_value(lower[i], upper[i]);
        }
        if (T::length() == 4) {
            res[3] = alpha;
        }
        return res;
    }
}
