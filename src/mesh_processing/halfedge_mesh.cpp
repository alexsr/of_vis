//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "halfedge_mesh.hpp"
#include "math/vector_math.hpp"
#include <utility>
#include <algorithm>
#include <set>

tostf::Halfedge_mesh::Halfedge_mesh(std::shared_ptr<Geometry> geometry) : _geometry(std::move(geometry)) {
    build();
}

void tostf::Halfedge_mesh::build() {
    _halfedges.resize(_geometry->indices.size());
    const auto vertex_count = _geometry->vertices.size();
    _vertices.resize(vertex_count);
    _halfedge_map.resize(vertex_count);
    const auto face_count = static_cast<int>(_halfedges.size());
    for (int i = 0; i < face_count; i++) {
        const int a = _geometry->indices.at(i);
        const int next_i = 3 * (i / 3) + (i + 1) % 3;
        const int b = _geometry->indices.at(next_i);
        _halfedge_map.at(a).push_back(i);
        _halfedges.at(i).id = i;
        _halfedges.at(i).vertex_ref = b;
        _vertices.at(a) = {i};
    }
    for (int i = 0; i < face_count; i++) {
        const auto v_b = _halfedges.at(i).vertex_ref;
        for (int edge_from_a : _halfedge_map.at(v_b)) {
            if (_halfedges.at(edge_from_a).vertex_ref == _geometry->indices.at(i)) {
                _halfedges.at(i).opposite = edge_from_a;
                break;
            }
        }
    }
}

std::vector<glm::vec4> tostf::Halfedge_mesh::calculate_normals() const {
    std::vector<glm::vec4> normals(_vertices.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(normals.size()); i++) {
        const auto he_start = _vertices.at(i).halfedge_ref;
        auto he_a = he_start;
        glm::vec3 normal(0);
        do {
            const auto ab = _geometry->vertices.at(i) - _geometry->vertices.at(_halfedges.at(he_a).vertex_ref);
            const auto he_b = _halfedges.at(he_a).next();
            const auto ac = _geometry->vertices.at(i) - _geometry->vertices.at(_halfedges.at(he_b).vertex_ref);
            const auto weight = glm::acos(dot(ab, ac) / (length(ab) * length(ac)));
            const auto triangle_normal = math::calc_normal(_geometry->vertices.at(i),
                                                           _geometry->vertices.at(_halfedges.at(he_a).vertex_ref),
                                                           _geometry->vertices.at(_halfedges.at(he_b).vertex_ref));

            const auto he_a_op = _halfedges.at(he_a).opposite;
            normal += triangle_normal * 0.5f * weight;
            if (he_a_op < 0) {
                break;
            }
            he_a = _halfedges.at(he_a_op).next();
        }
        while (he_a != he_start && he_a > 0);
        normals.at(i) = glm::vec4(normalize(normal), 0.0f);
    }
    return normals;
}

tostf::Mesh_stats tostf::Halfedge_mesh::calculate_mesh_stats() {
    Mesh_stats stats;
    const auto face_count = static_cast<int>(_halfedges.size());
    for (int i = 0; i < face_count; i++) {
        const auto he_a = i;
        const auto v_a = _halfedges.at(he_a).vertex_ref;
        const auto he_b = _halfedges.at(he_a).next();
        const auto v_b = _halfedges.at(he_b).vertex_ref;
        const auto he_c = _halfedges.at(he_b).next();
        const auto v_c = _halfedges.at(he_c).vertex_ref;
        const auto a = _geometry->vertices.at(v_a);
        const auto b = _geometry->vertices.at(v_b);
        const auto c = _geometry->vertices.at(v_c);
        const auto ab = b - a;
        const auto ac = a - c;
        const auto bc = b - c;
        const auto length_ab = length(ab);
        const auto length_ac = length(ac);
        const auto length_bc = length(bc);
        // Volume calc http://chenlab.ece.cornell.edu/Publication/Cha/icip01_Cha.pdf
        const auto v321 = c.x * b.y * a.z;
        const auto v231 = b.x * c.y * a.z;
        const auto v312 = c.x * a.y * b.z;
        const auto v132 = a.x * c.y * b.z;
        const auto v213 = b.x * a.y * c.z;
        const auto v123 = a.x * b.y * c.z;
        stats.volume += (1.0f / 6.0f) * (-v321 + v231 + v312 - v132 - v213 + v123);
        // edge length
        float edge_length = length_ab + length_ac + length_bc;
        stats.edge_length.avg += edge_length;
        if (stats.edge_length.min > length_ab) {
            stats.edge_length.min = length_ab;
        }
        if (stats.edge_length.min > length_ac) {
            stats.edge_length.min = length_ac;
        }
        if (stats.edge_length.min > length_bc) {
            stats.edge_length.min = length_bc;
        }
        if (stats.edge_length.max < length_ab) {
            stats.edge_length.max = length_ab;
        }
        if (stats.edge_length.max < length_ac) {
            stats.edge_length.max = length_ac;
        }
        if (stats.edge_length.max < length_bc) {
            stats.edge_length.max = length_bc;
        }
        // area calc
        float area = glm::acos(dot(ab, ac) / (length_ab * length_ac));
        stats.area.avg += area;
        if (stats.area.min > area) {
            stats.area.min = area;
        }
        if (stats.area.max < area) {
            stats.area.max = area;
        }
    }
    stats.edge_length.avg = stats.edge_length.avg / static_cast<float>(face_count * 3);
    stats.area.avg = stats.area.avg / static_cast<float>(face_count);
    stats.volume /= 3.0f;
    return stats;
}

std::vector<glm::vec4> tostf::Halfedge_mesh::find_hard_edge_candidates() const {
    std::vector<glm::vec4> edge_points;
    for (int i = 0; i < static_cast<int>(_vertices.size()); i++) {
        const auto he_start_id = _vertices.at(i).halfedge_ref;
        auto v = _geometry->vertices.at(i);
        auto he_next = _halfedges.at(he_start_id).next();
        auto triangle_normal = math::calc_normalized_normal(
            v, _geometry->vertices.at(_halfedges.at(he_start_id).vertex_ref),
            _geometry->vertices.at(_halfedges.at(he_next).vertex_ref));
        auto opp = _halfedges.at(he_start_id).opposite;
        if (opp < 0) {
            continue;
        }
        auto he_op_next = _halfedges.at(opp).next();
        while (he_start_id != he_op_next) {
            he_next = _halfedges.at(he_op_next).next();
            auto triangle_normal_comp = math::calc_normalized_normal(
                v, _geometry->vertices.at(_halfedges.at(he_op_next).vertex_ref),
                _geometry->vertices.at(_halfedges.at(he_next).vertex_ref));
            if (dot(triangle_normal_comp, triangle_normal) <= 0) {
                edge_points.push_back(v);
                break;
            }
            opp = _halfedges.at(he_op_next).opposite;
            if (opp < 0) {
                break;
            }
            he_op_next = _halfedges.at(opp).next();
        }
    }
    return edge_points;
}

std::shared_ptr<tostf::Geometry> tostf::Halfedge_mesh::get_geometry() const {
    return _geometry;
}

tostf::Geometry_view tostf::Halfedge_mesh::compute_one_ring_neighborhood(const unsigned vertex_id) {
    const auto he_start = _vertices.at(vertex_id).halfedge_ref;
    auto he_a = he_start;
    std::vector<unsigned> indices;
    do {
        const auto a = static_cast<unsigned>(_halfedges.at(he_a).vertex_ref);
        const auto he_b = _halfedges.at(he_a).next();
        const auto b = static_cast<unsigned>(_halfedges.at(he_b).vertex_ref);
        const auto he_a_op = _halfedges.at(he_a).opposite;
        indices.insert(indices.end(), {vertex_id, b, a});
        if (he_a_op < 0) {
            break;
        }
        he_a = _halfedges.at(he_a_op).next();
    }
    while (he_a != he_start && he_a > 0);
    return {_geometry, std::move(indices)};
}

std::vector<unsigned> tostf::Halfedge_mesh::compute_one_ring_neighborhood_faces(const unsigned vertex_id) {
    const auto he_start = _vertices.at(vertex_id).halfedge_ref;
    auto he_a = he_start;
    std::vector<unsigned> indices{static_cast<unsigned>(he_a) / 3};
    do {
        const auto he_a_op = _halfedges.at(he_a).opposite;
        indices.push_back(static_cast<unsigned>(he_a_op) / 3);
        if (he_a_op < 0) {
            break;
        }
        he_a = _halfedges.at(he_a_op).next();
    }
    while (he_a != he_start && he_a > 0);
    return indices;
}

void tostf::Halfedge_mesh::find_boundaries() {
    _boundaries = std::vector<std::vector<unsigned>>();
    std::set<unsigned> boundary_candidates;
    unsigned boundary_id = 0;
    for (int i = 0; i < static_cast<int>(_halfedges.size()); ++i) {
        const auto he = _halfedges.at(i);
        if (he.opposite == -1) {
            const auto res = boundary_candidates.emplace(i);
            if (res.second) {
                _boundaries->push_back({he.vertex_ref});
                int next_he = i;
                while ((next_he = find_adjacent_boundary_candidate(_halfedges.at(next_he).vertex_ref)) != -1
                       && next_he != i) {
                    const auto res_candidate = boundary_candidates.emplace(next_he);
                    if (res_candidate.second) {
                        _boundaries->at(boundary_id).push_back(_halfedges.at(next_he).vertex_ref);
                    }
                }
                ++boundary_id;
            }
        }
    }
}

int tostf::Halfedge_mesh::find_adjacent_boundary_candidate(const unsigned next_vertex) {
    if (next_vertex >= _halfedge_map.size()) {
        return -1;
    }
    for (auto he : _halfedge_map.at(next_vertex)) {
        if (_halfedges.at(he).opposite == -1) {
            return he;
        }
    }
    return -1;
}

std::vector<std::vector<unsigned>> tostf::Halfedge_mesh::get_boundaries() {
    if (!_boundaries) {
        find_boundaries();
    }
    return _boundaries.value();
}
