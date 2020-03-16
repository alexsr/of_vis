//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "geometry/geometry.hpp"

namespace tostf
{
    namespace foam
    {
        struct Grid_info {
            glm::vec4 bb_min;
            glm::vec4 bb_max;
            float cell_size;
            float cell_diag;
        };
        struct Volume_grid {
            Volume_grid(const Bounding_box& grid_bb, float in_cell_size);
            glm::ivec3 calc_cell_index_3d(const glm::vec4& pos) const;
            glm::ivec3 calc_bound_cell_index_3d(const glm::vec4& pos) const;
            glm::ivec3 calc_bound_offset_cell_index_3d(const glm::vec4& pos, const glm::ivec3& offset) const;
            int convert_3d_index_to_1d(const glm::ivec3& ijk) const;
            glm::ivec3 convert_1d_index_to_3d(int cell_index) const;
            int calc_cell_index(const glm::vec4& pos) const;
            glm::vec4 calc_cell_pos(int cell_index) const;
            void populate_cells(const std::vector<glm::vec4>& points, const std::vector<float>& radii);
            Grid_info get_grid_info() const;
            std::vector<std::vector<int>> cell_data_points;
            glm::ivec3 cell_count{};
            Bounding_box bb;
            float cell_size;
            float cell_radius;
            glm::vec3 grid_pos{};
            int total_cell_count;
        };
    }
}
