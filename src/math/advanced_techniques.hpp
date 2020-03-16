//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "glm_helper.hpp"
#include <vector>

namespace tostf
{
    inline std::vector<glm::vec4> generate_points_on_sphere(const int samples, const float radius = 1.0f,
                                                            const glm::vec3& center = glm::vec3(0.0f)) {
        std::vector<glm::vec4> points(samples, glm::vec4(center, 1.0f));
        const auto lat_factor = 2.0f / samples;
        const auto pi_times_golden_ratio = glm::pi<float>() * (3.0f - sqrt(5.0f));
#pragma omp parallel for
        for (int i = 0; i < samples; ++i) {
            const auto phi = glm::acos(1.0f - i * lat_factor);
            const auto theta = pi_times_golden_ratio * i;
            points.at(i) += radius * glm::vec4(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi), 0.0f);
        }
        return points;
    }
}
