//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <string>
#include <vector>
#include "math/glm_helper.hpp"
#include "gpu_interface/texture.hpp"

namespace tostf {
    struct Deformed_mesh {
        std::string name{};
        std::vector<glm::vec4> vertices{};
        std::vector<float> distances_to_reference{};
        float min_dist_to_ref{};
        float max_dist_to_ref{};
        float avg_dist_to_ref{};
        float sd_dist_to_ref{};
        std::unique_ptr<Texture2D> tex = nullptr;
    };
}
