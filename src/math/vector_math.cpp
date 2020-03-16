//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "vector_math.hpp"

glm::vec4 tostf::math::calc_center(const std::vector<glm::vec4>& points) {
    glm::vec4 center{};
    for (int i = 0; i < static_cast<int>(points.size()); i++) {
        center += points.at(i);
    }
    return center / static_cast<float>(points.size());
}

glm::vec3 tostf::math::calc_normal(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c) {
    const auto ab = a - b;
    const auto ac = a - c;
    return cross(glm::vec3(ab), glm::vec3(ac));
}

glm::vec3 tostf::math::calc_normalized_normal(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c) {
    return normalize(calc_normal(a, b, c));
}
