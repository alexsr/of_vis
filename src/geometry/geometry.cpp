//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "geometry.hpp"
#include "utility/utility.hpp"
#include <execution>
#include <utility>

tostf::Bounding_box tostf::calculate_bounding_box(const std::vector<glm::dvec4>& vertices) {
    // https://twitter.com/flx_schroeder/status/993097873322659840
    const auto min_max_func = make_overloaded_lambda(
        [](const glm::dmat2x4& b1, const glm::dmat2x4& b2) { return glm::dmat2x4(min(b1[0], b2[0]), max(b1[1], b2[1])); },
        [](const glm::dvec4& b1, const glm::dvec4& b2) { return glm::dmat2x4(min(b1, b2), max(b1, b2)); },
        [](const glm::dvec4& b1, const glm::dmat2x4& b2) { return glm::dmat2x4(min(b1, b2[0]), max(b1, b2[1])); },
        [](const glm::dmat2x4& b1, const glm::dvec4& b2) { return glm::dmat2x4(min(b1[0], b2), max(b1[1], b2)); }
    );
    auto bb = std::reduce(std::execution::par, vertices.begin(), vertices.end(),
        glm::dmat2x4(glm::vec4(std::numeric_limits<float>::max()),
            glm::dvec4(std::numeric_limits<float>::lowest())), min_max_func);
    return {bb[0], bb[1]};
}

tostf::Bounding_box tostf::calculate_bounding_box(const std::vector<glm::vec4>& vertices) {
    // https://twitter.com/flx_schroeder/status/993097873322659840
    const auto min_max_func = make_overloaded_lambda(
        [](const glm::mat2x4& b1, const glm::mat2x4& b2) { return glm::mat2x4(min(b1[0], b2[0]), max(b1[1], b2[1])); },
        [](const glm::vec4& b1, const glm::vec4& b2) { return glm::mat2x4(min(b1, b2), max(b1, b2)); },
        [](const glm::vec4& b1, const glm::mat2x4& b2) { return glm::mat2x4(min(b1, b2[0]), max(b1, b2[1])); },
        [](const glm::mat2x4& b1, const glm::vec4& b2) { return glm::mat2x4(min(b1[0], b2), max(b1[1], b2)); }
    );
    auto bb = std::reduce(std::execution::par, vertices.begin(), vertices.end(),
        glm::mat2x4(glm::vec4(std::numeric_limits<float>::max()),
            glm::vec4(std::numeric_limits<float>::lowest())), min_max_func);
    return {bb[0], bb[1]};
}

bool tostf::Geometry::has_vertices() const {
    return !vertices.empty();
}

bool tostf::Geometry::has_normals() const {
    return !normals.empty();
}

bool tostf::Geometry::has_uv_coords() const {
    return !uv_coords.empty();
}

bool tostf::Geometry::has_indices() const {
    return !indices.empty();
}

size_t tostf::Geometry::count() const {
    if (has_indices()) {
        return indices.size();
    }
    return vertices.size();
}

unsigned tostf::Geometry::get_attribute_count() const {
    return static_cast<unsigned>(has_vertices()) + static_cast<unsigned>(has_normals())
           + static_cast<unsigned>(has_uv_coords());
}

tostf::Bounding_box tostf::Geometry::get_bounding_box() {
    if (!_bounding_box) {
        _bounding_box = calculate_bounding_box(vertices);
    }
    return _bounding_box.value();
}

void tostf::Geometry::recalc_bounding_box() {
    _bounding_box = calculate_bounding_box(vertices);
}

void tostf::Geometry::move_to_center() {
    if (!_bounding_box) {
        _bounding_box = calculate_bounding_box(vertices);
    }
    const auto center = glm::vec4(glm::vec3(_bounding_box->min + _bounding_box->max) / 2.0f, 0.0f);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(vertices.size()); i++) {
        vertices.at(i) = vertices.at(i) - center;
    }
    _bounding_box->min -= center;
    _bounding_box->max -= center;
}

tostf::Faces_info tostf::Geometry::retrieve_faces_info() {
    if (!_faces_info) {
        _faces_info = Faces_info();
        _faces_info->face_normals.resize(count() / 3);
        _faces_info->face_centers.resize(count() / 3);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(count() / 3); i++) {
            const auto j = static_cast<unsigned>(i * 3);
            const auto a = vertices.at(indices.at(j));
            const auto b = vertices.at(indices.at(j + 1));
            const auto c = vertices.at(indices.at(j + 2));
            const auto ab = a - b;
            const auto ac = a - c;
            const auto face_normal = glm::vec4(normalize(cross(glm::vec3(ab), glm::vec3(ac))), 0.0f);
            const auto face_center = (a + b + c) / 3.0f;
            _faces_info->face_normals.at(i) = face_normal;
            _faces_info->face_centers.at(i) = face_center;
        }
    }
    return _faces_info.value();
}

tostf::Geometry_view::Geometry_view(std::shared_ptr<Geometry> geom, std::vector<unsigned> in_indices)
    : geometry(std::move(geom)), indices(std::move(in_indices)) {}
