//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "inlet_detection.hpp"
#include "data_processing/clustering.hpp"
#include "mesh_processing/tri_face_mesh.hpp"
#include <set>
#include "utility/random.hpp"

std::vector<tostf::truncation_plane> tostf::find_truncation_planes(const std::vector<glm::vec4>& edge_candidates,
                                                                   const int k) {
    std::vector<truncation_plane> res(k);
    auto clusters = k_means_pp(edge_candidates, k);
    bool cluster_empty = false;
    for (int i = 0; i < k; i++) {
        cluster_empty = cluster_empty || clusters.at(i).data_points.empty();
    }
    if (!cluster_empty) {
#pragma omp parallel for
        for (int i = 0; i < k; i++) {
            clusters.at(i).remove_outlier();
            res.at(i) = std::make_pair(find_plane(clusters.at(i).data_points), analyze_dataset(clusters.at(i).data_points));
        }
    }
    return res;
}

tostf::Inlet::Inlet() {
    color = tostf::gen_random_vec(glm::vec4(0.0f), glm::vec4(1.0f));
}

tostf::Inlet::Inlet(std::vector<std::array<unsigned, 3>>& in_indices, const glm::vec4& in_center,
                    const glm::vec4& in_normal)
    : indices(in_indices), center(in_center), normal(in_normal) {
    color = tostf::gen_random_vec(glm::vec4(0.0f), glm::vec4(1.0f));
}

void tostf::Inlet::rename(const std::string& n) {
    name = n;
}

void tostf::Inlet::set_boundary_type(const boundary_type b_type) {
    boundary.type = b_type;
}

std::vector<tostf::Inlet> tostf::detect_inlets_automatically(const Structured_mesh& mesh_data, const int plane_count) {
    std::vector<Inlet> inlets(plane_count);
    auto trunc_planes = find_truncation_planes(mesh_data.halfedge->find_hard_edge_candidates(), plane_count);
    for (int p = 0; p < plane_count; p++) {
        std::vector<std::array<unsigned, 3>> indices;
        auto plane = trunc_planes.at(p).first;
        const auto cluster_stats = trunc_planes.at(p).second;
        std::vector<glm::vec4> inlet_points;
        for (unsigned f = 0; f < mesh_data.tri_face_mesh->face_indices.size(); ++f) {
            if (plane.is_on_plane(mesh_data.tri_face_mesh->face_centers.at(f), 0.3f)
                && abs(dot(mesh_data.tri_face_mesh->face_normals.at(f), plane.normal)) >= 0.9
                && distance(mesh_data.tri_face_mesh->face_centers.at(f), plane.center)
                < 4.0f * cluster_stats.standard_deviation) {
                if (dot(mesh_data.tri_face_mesh->face_normals.at(f), plane.normal) < 0.0f) {
                    plane.normal = -plane.normal;
                }
                auto face_ids = mesh_data.tri_face_mesh->face_indices.at(f);
                inlet_points.push_back(mesh_data.mesh->vertices.at(face_ids.at(0)));
                inlet_points.push_back(mesh_data.mesh->vertices.at(face_ids.at(1)));
                inlet_points.push_back(mesh_data.mesh->vertices.at(face_ids.at(2)));
                indices.insert(indices.end(), mesh_data.tri_face_mesh->face_indices.at(f));
            }
        }
        if (indices.empty()) {
            return detect_inlets_automatically(mesh_data, plane_count);
        }
        auto new_plane = tostf::find_plane(inlet_points);
        if (dot(new_plane.normal, plane.normal) < 0.0f) {
            new_plane.normal = -new_plane.normal;
        }
        float dist_to_center = 0.0f;
        for (int inlet_p_id = 0; inlet_p_id < static_cast<int>(inlet_points.size()); ++inlet_p_id) {
            dist_to_center = glm::max(dist_to_center, distance(inlet_points.at(inlet_p_id), new_plane.center));
        }
        inlets.at(p).indices = indices;
        inlets.at(p).normal = plane.normal;
        inlets.at(p).center = new_plane.center;
        inlets.at(p).bitangent = new_plane.bitangent;
        inlets.at(p).tangent = new_plane.tangent;
        inlets.at(p).radius = dist_to_center;
        float radius = 0;
        for (const auto& face : indices) {
            radius += distance(inlets.at(p).center, mesh_data.mesh->vertices.at(face.at(0)));
            radius += distance(inlets.at(p).center, mesh_data.mesh->vertices.at(face.at(1)));
            radius += distance(inlets.at(p).center, mesh_data.mesh->vertices.at(face.at(2)));
        }
        inlets.at(p).name = "inlet" + std::to_string(p);
    }
    return inlets;
}

inline bool operator==(const std::array<unsigned, 3>& f1, const std::array<unsigned, 3>& f2) {
    return f1.at(0) == f2.at(0) && f1.at(1) == f2.at(1) && f1.at(2) == f2.at(2)
           || f1.at(0) == f2.at(1) && f1.at(1) == f2.at(2) && f1.at(2) == f2.at(0)
           || f1.at(0) == f2.at(2) && f1.at(1) == f2.at(0) && f1.at(2) == f2.at(1);
}

std::optional<tostf::Manual_inlet_selection> tostf::select_inlet_manually(const Structured_mesh& mesh_data,
                                                                          const unsigned selected_vertex) {
    std::optional<Manual_inlet_selection> result;
    Plane plane{mesh_data.mesh->vertices.at(selected_vertex), mesh_data.mesh->normals.at(selected_vertex)};
    std::vector<unsigned> still_to_check{selected_vertex};
    std::set<unsigned> found_plane_points{selected_vertex};
    std::vector<float> highlight_data(mesh_data.mesh->vertices.size(), 0);
    highlight_data.at(selected_vertex) = 1;
    std::set<std::array<unsigned, 3>> selected_faces;
    while (!still_to_check.empty()) {
        std::vector<unsigned> new_still_to_check;
        for (auto sel_v : still_to_check) {
            auto one_ring_view = mesh_data.halfedge->compute_one_ring_neighborhood_faces(sel_v);
            for (unsigned i = 0; i < one_ring_view.size(); i++) {
                const auto face_id = one_ring_view.at(i);
                const auto face_indices = mesh_data.original_tri_face_mesh->face_indices.at(face_id);
                const auto center = mesh_data.original_tri_face_mesh->face_centers.at(face_id);
                const auto normal = mesh_data.original_tri_face_mesh->face_normals.at(face_id);
                if (abs(dot(normal, plane.normal)) >= 0.9 && plane.is_on_plane(center, 0.3f)) {
                    const auto res_face = selected_faces.emplace(face_indices).second;
                    if (res_face) {
                        const auto a_id = face_indices.at(0);
                        const auto b_id = face_indices.at(1);
                        const auto c_id = face_indices.at(2);
                        const auto res_a = found_plane_points.emplace(a_id).second;
                        if (res_a) {
                            highlight_data.at(a_id) = 1;
                            new_still_to_check.push_back(a_id);
                            auto a = mesh_data.mesh->vertices.at(a_id);
                        }
                        const auto res_b = found_plane_points.emplace(b_id).second;
                        if (res_b) {
                            highlight_data.at(b_id) = 1;
                            new_still_to_check.push_back(b_id);
                            auto b = mesh_data.mesh->vertices.at(b_id);
                        }
                        const auto res_c = found_plane_points.emplace(c_id).second;
                        if (res_c) {
                            highlight_data.at(c_id) = 1;
                            new_still_to_check.push_back(c_id);
                            auto c = mesh_data.mesh->vertices.at(c_id);
                        }
                    }
                }
            }
        }
        still_to_check = std::move(new_still_to_check);
    }
    std::vector<glm::vec4> inlet_points(found_plane_points.size());
    int i = 0;
    for (auto& id : found_plane_points) {
        inlet_points.at(i) = mesh_data.mesh->vertices.at(id);
        i++;
    }
    auto new_plane = tostf::find_plane(inlet_points);
    if (dot(new_plane.normal, plane.normal) < 0.0f) {
        new_plane.normal = -new_plane.normal;
    }
    float dist_to_center = 0.0f;
    for (int inlet_p_id = 0; inlet_p_id < static_cast<int>(inlet_points.size()); ++inlet_p_id) {
        dist_to_center = glm::max(dist_to_center, distance(inlet_points.at(inlet_p_id), new_plane.center));
    }
    if (!selected_faces.empty()) {
        std::vector<std::array<unsigned, 3>> res_indices;
        std::copy(selected_faces.begin(), selected_faces.end(), std::back_inserter(res_indices));
        result = {highlight_data, {res_indices, new_plane.center, new_plane.normal}};
        result->selection.tangent = new_plane.tangent;
        result->selection.bitangent = new_plane.bitangent;
        result->selection.radius = dist_to_center;
    }
    return result;
}

std::vector<tostf::Inlet> tostf::detect_inlets_from_holes(const Structured_mesh& mesh_data) {
    std::vector<Inlet> inlets;
    auto boundaries = mesh_data.halfedge->get_boundaries();
    if (!boundaries.empty()) {
        const auto vertices_count = mesh_data.mesh->vertices.size();
        mesh_data.mesh->vertices.resize(vertices_count + boundaries.size());
        mesh_data.mesh->normals.resize(vertices_count + boundaries.size());
        inlets.resize(boundaries.size());
        for (int boundary_id = 0; boundary_id < static_cast<int>(boundaries.size()); ++boundary_id) {
            auto b = boundaries.at(boundary_id);
            inlets.at(boundary_id).indices.resize(b.size() * 3);
            std::vector<glm::vec4> inlet_points(b.size());
            std::vector<unsigned> indices(b.size() * 3);
            for (unsigned i = 0; i < b.size(); ++i) {
                const auto next_i = (i + 1) % b.size();
                inlet_points.at(i) = mesh_data.mesh->vertices.at(b.at(i));
                const std::array<unsigned, 3> index_tuple{b.at(next_i), b.at(i), vertices_count + boundary_id};
                inlets.at(boundary_id).indices.at(i) = index_tuple;
                indices.at(i * 3 + 0) = b.at(next_i);
                indices.at(i * 3 + 1) = b.at(i);
                indices.at(i * 3 + 2) = vertices_count + boundary_id;
            }
            auto plane = tostf::find_plane(inlet_points);
            if (dot(plane.normal, mesh_data.mesh->normals.at(b.at(0))) < 0.0f) {
                plane.normal = -plane.normal;
            }
            float dist_to_center = 0.0f;
            for (int inlet_p_id = 0; inlet_p_id < static_cast<int>(inlet_points.size()); ++inlet_p_id) {
                dist_to_center = glm::max(dist_to_center, distance(inlet_points.at(inlet_p_id), plane.center));
            }
            mesh_data.mesh->vertices.at(vertices_count + boundary_id) = plane.center;
            mesh_data.mesh->normals.at(vertices_count + boundary_id) = plane.normal;
            mesh_data.mesh->indices.insert(mesh_data.mesh->indices.end(), indices.begin(), indices.end());
            inlets.at(boundary_id).normal = plane.normal;
            inlets.at(boundary_id).center = plane.center;
            inlets.at(boundary_id).tangent = plane.tangent;
            inlets.at(boundary_id).bitangent = plane.tangent;
            inlets.at(boundary_id).radius = dist_to_center;
            inlets.at(boundary_id).name = "inlet" + std::to_string(boundary_id);
        }
    }
    return inlets;
}
