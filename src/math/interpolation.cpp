//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "interpolation.hpp"

float tostf::modified_shepard_weight(const glm::vec4& a, const glm::vec4& b, const float radius) {
    const auto dist_ab = distance(glm::vec3(a), glm::vec3(b));
    if (dist_ab == 0.0f) {
        return -1.0f;
    }
    const auto w_sqrt = glm::max(0.0f, radius - dist_ab) / (radius * dist_ab);
    return w_sqrt * w_sqrt;
}

float tostf::shepard_weight(const glm::vec4& a, const glm::vec4& b, const float power) {
    const auto dist_ab = distance(glm::vec3(a), glm::vec3(b));
    if (dist_ab == 0) {
        return -1.0f;
    }
    return 1.0f / pow(dist_ab, power);
}
