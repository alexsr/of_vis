//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "mesh.hpp"
#include <glm/gtx/hash.hpp>
#include <vector>
#include <random>
#include "math/vector_math.hpp"
#include <map>

void tostf::Mesh::join_vertices() {
    std::unordered_map<glm::vec4, unsigned> joined_vertex_map;
    for (int i = 0; i < static_cast<int>(vertices.size()); ++i) {
        joined_vertex_map.emplace(vertices.at(i), joined_vertex_map.size());
    }
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(indices.size()); i++) {
        indices.at(i) = joined_vertex_map.at(vertices.at(indices.at(i)));
    }
    std::vector<glm::vec4> new_vertices(joined_vertex_map.size());
    for (auto& v : joined_vertex_map) {
        new_vertices.at(v.second) = v.first;
    }
    vertices = new_vertices;
}

void tostf::Mesh::remove_indices(const std::vector<unsigned>& ids) {
    auto end = std::remove_if(indices.begin(), indices.end(), [ids](const unsigned id) {
        return std::find(ids.begin(), ids.end(), id) != ids.end();
    });
}

glm::vec4 tostf::Mesh::find_random_point_in_mesh(const float offset) {
    std::random_device rd;
    std::mt19937 rng(rd());
    const std::uniform_int_distribution<unsigned> uni_id(0, static_cast<unsigned>(vertices.size()));
    const auto id = uni_id(rng);
    const auto random_point = vertices.at(id);
    auto normal = glm::vec4(0);
    if (has_normals()) {
        normal = normals.at(id);
    }
    return random_point - normal * offset;
}

bool tostf::Mesh::is_inside(const glm::vec4 p) {
    std::random_device rd;
    std::mt19937 rng(rd());
    const std::uniform_int_distribution<unsigned> uni(0, static_cast<unsigned>(vertices.size() - 1));
    const glm::vec3 ray_direction(vertices.at(uni(rng)) - p);
    const auto eps = 0.0001f;
    int intersections = 0;
    for (int i = 0; i < static_cast<int>(indices.size()); i += 3) {
        auto v0 = glm::vec3(vertices.at(indices.at(i)));
        auto v1 = glm::vec3(vertices.at(indices.at(i + 1)));
        auto v2 = glm::vec3(vertices.at(indices.at(i + 2)));
        auto e0 = v1 - v0;
        auto e1 = v2 - v0;
        auto h = cross(ray_direction, e1);
        const auto a = dot(e0, h);
        if (abs(a) < eps) {
            continue;
        }
        const auto f = 1.0f / a;
        auto s = glm::vec3(p) - v0;
        const auto u = f * dot(s, h);
        if (u < 0.0f || u > 1.0f) {
            continue;
        }
        auto q = cross(s, e0);
        const auto v = f * dot(ray_direction, q);
        if (v < 0.0f || v > 1.0f) {
            continue;
        }
        const auto t = f * dot(e1, q);
        if (t > eps) {
            intersections++;
        }
    }
    return intersections % 2 == 1;
}

std::vector<float> tostf::Mesh::surface_distance(const std::shared_ptr<Mesh>& m) {
    std::vector<float> distances(indices.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(distances.size()); i++) {
        distances.at(i) = math::min_dist(vertices.at(indices.at(i)), m->vertices);
    }
    return distances;
}

std::vector<float> tostf::Mesh::vertex_wise_distance(const std::shared_ptr<Mesh>& m) {
    if (indices.size() != m->indices.size()) {
        throw std::runtime_error{"Meshes have to have the same size."};
    }
    std::vector<float> distances(indices.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(distances.size()); i++) {
        distances.at(i) = distance(vertices.at(indices.at(i)), m->vertices.at(m->indices.at(i)));
    }
    return distances;
}
