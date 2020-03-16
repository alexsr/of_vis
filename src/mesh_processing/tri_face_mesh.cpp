//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "tri_face_mesh.hpp"
#include <utility>
#include "halfedge_mesh.hpp"
#include "math/vector_math.hpp"

tostf::Tri_face_mesh::Tri_face_mesh(std::shared_ptr<Geometry> geom) : _geometry(std::move(geom)) {
    face_indices.resize(_geometry->count() / 3);
    face_normals.resize(_geometry->count() / 3);
    face_centers.resize(_geometry->count() / 3);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(_geometry->count() / 3); i++) {
        const auto j = static_cast<unsigned>(i * 3);
        const auto a = _geometry->vertices.at(_geometry->indices.at(j));
        const auto b = _geometry->vertices.at(_geometry->indices.at(j + 1));
        const auto c = _geometry->vertices.at(_geometry->indices.at(j + 2));
        const auto ab = a - b;
        const auto ac = a - c;
        const auto face_normal = glm::vec4(normalize(cross(glm::vec3(ab), glm::vec3(ac))), 0.0f);
        const auto face_center = (a + b + c) / 3.0f;
        face_indices.at(i) = {
            _geometry->indices.at(j), _geometry->indices.at(j + 1), _geometry->indices.at(j + 2),
        };
        face_normals.at(i) = face_normal;
        face_centers.at(i) = face_center;
    }
}

std::shared_ptr<tostf::Mesh> tostf::Tri_face_mesh::to_mesh() {
    auto m = std::make_shared<Mesh>();
    m->indices.resize(face_indices.size() * 3);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(m->indices.size()); i++) {
        m->indices.at(i) = i;
    }
    m->vertices.resize(face_indices.size() * 3);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(face_indices.size()); i++) {
        m->vertices.at(i * 3) = _geometry->vertices.at(face_indices.at(i).at(0));
        m->vertices.at(i * 3 + 1) = _geometry->vertices.at(face_indices.at(i).at(1));
        m->vertices.at(i * 3 + 2) = _geometry->vertices.at(face_indices.at(i).at(2));
    }
    return m;
}

std::vector<unsigned> tostf::Tri_face_mesh::get_indices() {
    std::vector<unsigned> indices(face_indices.size() * 3);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(face_indices.size()); i++) {
        indices.at(i * 3) = face_indices.at(i).at(0);
        indices.at(i * 3 + 1) = face_indices.at(i).at(1);
        indices.at(i * 3 + 2) = face_indices.at(i).at(2);
    }
    return indices;
}

std::vector<std::array<unsigned, 3>> tostf::Tri_face_mesh::get_index_tuples() const {
    return face_indices;
}

tostf::Mesh_stats tostf::Tri_face_mesh::calculate_mesh_stats() {
    Mesh_stats stats{};
    const auto face_count = static_cast<int>(face_indices.size());
    for (int i = 0; i < face_count; i++) {
        const auto face = face_indices.at(i);
        const auto v_a = face.at(0);
        const auto v_b = face.at(1);
        const auto v_c = face.at(2);
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
    return stats;
}

inline bool operator==(const std::array<unsigned, 3>& f1, const std::array<unsigned, 3>& f2) {
    return f1.at(0) == f2.at(0) && f1.at(1) == f2.at(1) && f1.at(2) == f2.at(2)
           || f1.at(0) == f2.at(1) && f1.at(1) == f2.at(2) && f1.at(2) == f2.at(0)
           || f1.at(0) == f2.at(2) && f1.at(1) == f2.at(0) && f1.at(2) == f2.at(1);
}

bool tostf::Tri_face_mesh::add_face(const std::array<unsigned, 3>& face) {
    if (std::find(face_indices.begin(), face_indices.end(), face) == face_indices.end()) {
        face_indices.push_back(face);
        const auto face_normal = math::calc_normalized_normal(_geometry->vertices.at(face.at(0)),
                                                              _geometry->vertices.at(face.at(1)),
                                                              _geometry->vertices.at(face.at(2)));
        const auto face_center = math::calc_center({
            _geometry->vertices.at(face.at(0)),
            _geometry->vertices.at(face.at(1)),
            _geometry->vertices.at(face.at(2))
        });
        face_normals.emplace_back(face_normal, 0.0f);
        face_centers.push_back(face_center);
        return true;
    }
    return false;
}

void tostf::Tri_face_mesh::add_faces(const std::vector<std::array<unsigned, 3>>& faces) {
    std::vector<glm::vec4> normals(faces.size());
    std::vector<glm::vec4> centers(faces.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(faces.size()); i++) {
        auto a = _geometry->vertices.at(faces.at(i).at(0));
        auto b = _geometry->vertices.at(faces.at(i).at(1));
        auto c = _geometry->vertices.at(faces.at(i).at(2));
        const auto face_normal = math::calc_normalized_normal(a, b, c);
        const auto face_center = math::calc_center({a, b, c});
        normals.at(i) = glm::vec4(face_normal, 0.0f);
        centers.at(i) = face_center;
    }
    face_indices.insert(face_indices.end(), faces.begin(), faces.end());
    face_normals.insert(face_normals.end(), normals.begin(), normals.end());
    face_centers.insert(face_centers.end(), centers.begin(), centers.end());
}

void tostf::Tri_face_mesh::add_faces(const std::vector<unsigned>& faces) {
    std::vector<std::array<unsigned, 3>> indices(faces.size() / 3);
    std::vector<glm::vec4> normals(faces.size() / 3);
    std::vector<glm::vec4> centers(faces.size() / 3);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(faces.size() / 3); i++) {
        indices.at(i) = {faces.at(i * 3 + 0), faces.at(i * 3 + 1), faces.at(i * 3 + 2)};
        auto a = _geometry->vertices.at(faces.at(i * 3 + 0));
        auto b = _geometry->vertices.at(faces.at(i * 3 + 1));
        auto c = _geometry->vertices.at(faces.at(i * 3 + 2));
        const auto face_normal = math::calc_normalized_normal(a, b, c);
        const auto face_center = math::calc_center({a, b, c});
        normals.at(i) = glm::vec4(face_normal, 0.0f);
        centers.at(i) = face_center;
    }
    face_indices.insert(face_indices.end(), indices.begin(), indices.end());
    face_normals.insert(face_normals.end(), normals.begin(), normals.end());
    face_centers.insert(face_centers.end(), centers.begin(), centers.end());
}

void tostf::Tri_face_mesh::remove_face(const std::array<unsigned, 3>& index_triple) {
    const auto del_it = std::find(face_indices.begin(), face_indices.end(), index_triple);
    if (del_it != face_indices.end()) {
        const auto dist = std::distance(face_indices.begin(), del_it);
        face_indices.erase(del_it);
        face_normals.erase(face_normals.begin() + dist);
        face_centers.erase(face_centers.begin() + dist);
    }
}

void tostf::Tri_face_mesh::remove_faces(const std::vector<std::array<unsigned, 3>>& del_faces) {
    for (auto& f : del_faces) {
        remove_face(f);
    }
}

bool tostf::Tri_face_mesh::has_face(const std::array<unsigned, 3>& index_triple) {
    return std::find(face_indices.begin(), face_indices.end(), index_triple) != face_indices.end();
}

std::shared_ptr<tostf::Geometry> tostf::Tri_face_mesh::get_geometry() const {
    return _geometry;
}

void tostf::Tri_face_mesh::set_geometry(const std::shared_ptr<Geometry>& geom) {
    _geometry = geom;
}
