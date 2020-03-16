//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "compact_grid.hpp"
#include "utility/logging.hpp"
#include "math/interpolation.hpp"

tostf::Compact_grid::Compact_grid(const Bounding_box& grid_bb, const float in_cell_size)
    : cell_size(in_cell_size) {
    bb.min = grid_bb.min - glm::vec4(cell_size, cell_size, cell_size, 0.0f) / 2.0f;
    bb.max = grid_bb.max + glm::vec4(cell_size, cell_size, cell_size, 0.0f) / 2.0f;
    cell_radius = sqrt(6.0f) * cell_size;
    const auto grid_size = bb.max - bb.min;
    grid_pos = bb.max + bb.min;
    cell_count = glm::ivec3(ceil(grid_size / cell_size));
    total_cell_count = cell_count.x * cell_count.y * cell_count.z;
}

glm::ivec3 tostf::Compact_grid::calc_cell_index_3d(const glm::vec4& pos) const {
    return glm::ivec3((pos - bb.min) / cell_size);
}

glm::ivec3 tostf::Compact_grid::calc_bound_cell_index_3d(const glm::vec4& pos) const {
    return min(max(glm::ivec3(0, 0, 0), calc_cell_index_3d(pos)), cell_count - glm::ivec3(1));
}

glm::ivec3 tostf::Compact_grid::calc_bound_offset_cell_index_3d(const glm::vec4& pos,
                                                                     const glm::ivec3& offset) const {
    return min(max(glm::ivec3(0, 0, 0), calc_cell_index_3d(pos) + offset), cell_count - glm::ivec3(1));
}

int tostf::Compact_grid::convert_3d_index_to_1d(const glm::ivec3& ijk) const {
    const int cell_index = ijk.z * cell_count.x * cell_count.y
                           + ijk.y * cell_count.x + ijk.x;
    if (cell_index < 0 || cell_index > total_cell_count) {
        return -1;
    }
    return cell_index;
}

glm::ivec3 tostf::Compact_grid::convert_1d_index_to_3d(const int cell_index) const {
    glm::ivec3 ijk;
    ijk.z = cell_index / cell_count.x / cell_count.y;
    ijk.y = (cell_index - ijk.z * cell_count.x * cell_count.y) / cell_count.x;
    ijk.x = cell_index - ijk.z * cell_count.x * cell_count.y - ijk.y * cell_count.x;
    return ijk;
}

int tostf::Compact_grid::calc_cell_index(const glm::vec4& pos) const {
    const auto ijk = calc_cell_index_3d(pos);
    return convert_3d_index_to_1d(ijk);
}

glm::vec4 tostf::Compact_grid::calc_cell_pos(const int cell_index) const {
    const auto ijk = convert_1d_index_to_3d(cell_index);
    return bb.min + glm::vec4(ijk.x * cell_size, ijk.y * cell_size, ijk.z * cell_size, 0.0f)
           + glm::vec4(cell_size, cell_size, cell_size, 0.0f) * 0.5f;
}

void tostf::Compact_grid::populate_cells(const std::vector<glm::vec4>& points) {
    for (int point_id = 0; point_id < static_cast<int>(points.size()); ++point_id) {
        const auto cell_index = calc_cell_index(points.at(point_id));
        if (cells.find(cell_index) == cells.end()) {
            cells[cell_index] = {point_id};
        }
        else {
            cells.at(cell_index).push_back(point_id);
        }
    }
}
