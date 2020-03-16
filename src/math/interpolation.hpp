//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "math/glm_helper.hpp"
#include <vector>
#include <array>

namespace tostf
{
    // https://en.wikipedia.org/wiki/Inverse_distance_weighting
    float modified_shepard_weight(const glm::vec4& a, const glm::vec4& b, float radius);
    float shepard_weight(const glm::vec4& a, const glm::vec4& b, float power);
}
