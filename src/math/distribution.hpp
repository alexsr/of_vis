// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "glm_helper.hpp"

namespace tostf
{
    namespace math
    {
        template<typename T>
        T calc_gauss_rayleigh(T x, T gauss_variance, T rayleigh_variance, T rayleigh_shift) {
            const auto rayleigh_dist = x + rayleigh_shift;
            const auto rayleigh = rayleigh_dist / rayleigh_variance * glm::exp(
                                      -rayleigh_dist * rayleigh_dist / (2.0f * rayleigh_variance
                                      ));
            const auto gauss = glm::exp(-x * x / (2.0f * gauss_variance))
                               / sqrt(2.0f * glm::pi<T>() * gauss_variance);
            return gauss * rayleigh;
        }
    }
}
