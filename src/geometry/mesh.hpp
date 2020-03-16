//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <filesystem>
#include "geometry.hpp"

namespace tostf
{
    // Mesh_data is used to store data which is commonly part of a mesh definition.
    struct Mesh : Geometry {
        void join_vertices();
        void remove_indices(const std::vector<unsigned>& ids);
        glm::vec4 find_random_point_in_mesh(float offset = 1.192092896e-07F);
        bool is_inside(glm::vec4 p);
        std::vector<float> surface_distance(const std::shared_ptr<Mesh>& m);
        std::vector<float> vertex_wise_distance(const std::shared_ptr<Mesh>& m);
    };
}
