//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "volume_grid.hpp"
#include "utility/logging.hpp"
#include "math/interpolation.hpp"

tostf::foam::Volume_grid::Volume_grid(const Bounding_box& grid_bb, const float in_cell_size)
    : cell_size(in_cell_size) {
    bb.min = grid_bb.min;
    bb.max = grid_bb.max;
    cell_radius = sqrt(6.0f) * cell_size;
    const auto grid_size = bb.max - bb.min;
    grid_pos = bb.max + bb.min;
    cell_count = glm::ivec3(ceil(grid_size / cell_size));
    total_cell_count = cell_count.x * cell_count.y * cell_count.z;
}

glm::ivec3 tostf::foam::Volume_grid::calc_cell_index_3d(const glm::vec4& pos) const {
    return glm::ivec3((pos - bb.min) / cell_size);
}

glm::ivec3 tostf::foam::Volume_grid::calc_bound_cell_index_3d(const glm::vec4& pos) const {
    return min(max(glm::ivec3(0, 0, 0), calc_cell_index_3d(pos)), cell_count - glm::ivec3(1));
}

glm::ivec3 tostf::foam::Volume_grid::calc_bound_offset_cell_index_3d(const glm::vec4& pos,
                                                                     const glm::ivec3& offset) const {
    return min(max(glm::ivec3(0, 0, 0), calc_cell_index_3d(pos) + offset), cell_count - glm::ivec3(1));
}

int tostf::foam::Volume_grid::convert_3d_index_to_1d(const glm::ivec3& ijk) const {
    const int cell_index = ijk.z * cell_count.x * cell_count.y
                           + ijk.y * cell_count.x + ijk.x;
    if (cell_index < 0 || cell_index > total_cell_count) {
        return -1;
    }
    return cell_index;
}

glm::ivec3 tostf::foam::Volume_grid::convert_1d_index_to_3d(const int cell_index) const {
    glm::ivec3 ijk;
    ijk.z = cell_index / cell_count.x / cell_count.y;
    ijk.y = (cell_index - ijk.z * cell_count.x * cell_count.y) / cell_count.x;
    ijk.x = cell_index - ijk.z * cell_count.x * cell_count.y - ijk.y * cell_count.x;
    return ijk;
}

int tostf::foam::Volume_grid::calc_cell_index(const glm::vec4& pos) const {
    const auto ijk = calc_cell_index_3d(pos);
    return convert_3d_index_to_1d(ijk);
}

glm::vec4 tostf::foam::Volume_grid::calc_cell_pos(const int cell_index) const {
    const auto ijk = convert_1d_index_to_3d(cell_index);
    return bb.min + glm::vec4(ijk.x * cell_size, ijk.y * cell_size, ijk.z * cell_size, 0.0f)
           + glm::vec4(cell_size, cell_size, cell_size, 0.0f) * 0.5f;
}

void tostf::foam::Volume_grid::populate_cells(const std::vector<glm::vec4>& points, const std::vector<float>& radii) {
    cell_data_points.resize(total_cell_count);
    for (int data_point_id = 0; data_point_id < static_cast<int>(points.size()); ++data_point_id) {
        auto& data_point = points.at(data_point_id);
        auto cell_min = data_point - glm::vec4(glm::vec3(radii.at(data_point_id)), 1.0f);
        auto cell_max = data_point + glm::vec4(glm::vec3(radii.at(data_point_id)), 1.0f);
        auto offset = glm::vec4(1, 1, 1, 0) * cell_size;
        const auto min_index = calc_bound_cell_index_3d(cell_min);
        const auto max_index = calc_bound_cell_index_3d(cell_max);
#pragma omp parallel for schedule(dynamic,1)
        for (auto k = min_index.z; k <= max_index.z; ++k) {
            const auto cell_index_k = k * cell_count.x * cell_count.y;
#pragma omp parallel for schedule(dynamic,1)
            for (auto j = min_index.y; j <= max_index.y; ++j) {
                const auto cell_index_kj = cell_index_k + j * cell_count.x;
#pragma omp parallel for schedule(dynamic,1)
                for (auto i = min_index.x; i <= max_index.x; ++i) {
                    const auto cell_index = cell_index_kj + i;
                    cell_data_points.at(cell_index).push_back(data_point_id);
                }
            }
        }
    }
}

tostf::foam::Grid_info tostf::foam::Volume_grid::get_grid_info() const {
    const float cell_diag = sqrt(3.0f) * cell_size;
    return {bb.min, bb.max, cell_size, cell_diag};
}
