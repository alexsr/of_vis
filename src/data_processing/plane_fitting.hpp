//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "data_analysis.hpp"

namespace tostf
{
    struct Plane {
        Plane() = default;
        Plane(glm::vec4 in_center, glm::vec4 in_normal);
        Plane(glm::vec4 in_center, glm::vec4 in_normal, glm::vec4 in_tangent, glm::vec4 in_bitangent);
        glm::vec4 center{};
        glm::vec4 normal{};
        glm::vec4 tangent{};
        glm::vec4 bitangent{};
        std::vector<glm::vec4> to_points() const;
        bool is_on_plane(glm::vec4 p, float bias = 0.001) const;
    };

    Plane find_plane(const std::vector<glm::vec4>& points);
    void remove_outliers(std::vector<glm::vec4>& points, glm::vec4 center, float bias = 0);
}
