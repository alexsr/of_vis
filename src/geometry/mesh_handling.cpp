#include "mesh_handling.hpp"
//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "mesh_handling.hpp"
#include "utility/logging.hpp"
#include "file/file_handling.hpp"
#include <stdexcept>
#include <thread>
#include <sstream>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>

std::optional<std::shared_ptr<tostf::Mesh>> extract_mesh_from_scene(const aiScene* scene,
                                                                    const bool move_mesh_to_center,
                                                                    const unsigned id,
                                                                    const tostf::mesh_flag mesh_load_flags) {
    if (id >= scene->mNumMeshes) {
        tostf::log_warning() << "No meshes found.";
        return std::nullopt;
    }
    std::vector<glm::vec4> vertices;
    std::vector<glm::vec4> normals;
    std::vector<glm::vec2> uv_coords;
    std::vector<unsigned> indices;
    auto mesh = std::make_shared<tostf::Mesh>();
    const auto count = scene->mMeshes[id]->mNumVertices;
    const auto faces_count = scene->mMeshes[id]->mNumFaces;
    // Copying the data from assimp arrays to the Mesh struct vectors is done in parallel
    // to improve mesh loading times.
    if (mesh_load_flags & tostf::mesh_flag::vertices) {
        vertices.resize(count);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(count); i++) {
            vertices.at(i) = glm::vec4(scene->mMeshes[id]->mVertices[i].x, scene->mMeshes[id]->mVertices[i].y,
                                       scene->mMeshes[id]->mVertices[i].z, 1);
        }
        mesh->vertices = std::move(vertices);
    }
    if (scene->mMeshes[id]->HasNormals() && mesh_load_flags & tostf::mesh_flag::normals) {
        normals.resize(count);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(count); i++) {
            normals.at(i) = glm::vec4(scene->mMeshes[id]->mNormals[i].x, scene->mMeshes[id]->mNormals[i].y,
                                      scene->mMeshes[id]->mNormals[i].z, 0);
        }
        mesh->normals = std::move(normals);
    }
    else {
        tostf::log_warning() << "Mesh has no normals.";
    }
    if (scene->mMeshes[id]->HasTextureCoords(0) && mesh_load_flags & tostf::mesh_flag::uv_coords) {
        uv_coords.resize(count);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(count); i++) {
            uv_coords.at(i) = glm::vec2(scene->mMeshes[id]->mTextureCoords[0][i].x,
                                        scene->mMeshes[id]->mTextureCoords[0][i].y);
        }
        mesh->uv_coords = std::move(uv_coords);
    }
    else {
        tostf::log_warning() << "Mesh has no uv coords.";
    }
    if (mesh_load_flags & tostf::mesh_flag::indices) {
        const auto face_indices = scene->mMeshes[id]->mFaces[0].mNumIndices;
        indices.resize(faces_count * face_indices);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(faces_count); i++) {
            const auto face = scene->mMeshes[id]->mFaces[i];
            for (int j = 0; j < static_cast<int>(face.mNumIndices); j++) {
                indices.at(i * face.mNumIndices + j) = face.mIndices[j];
            }
        }
        mesh->indices = std::move(indices);
    }
    if (move_mesh_to_center) {
        mesh->move_to_center();
    }
    return mesh;
}

std::optional<std::shared_ptr<tostf::Mesh>> tostf::load_single_mesh(const std::filesystem::path& path,
                                                                    const unsigned id,
                                                                    const bool move_to_center, const unsigned ai_pflags,
                                                                    const mesh_flag mesh_load_flags) {
    check_file_validity(path);
    log_info() << "Loading mesh from file: " << path.string();
    Assimp::Importer imp;
    const auto scene = imp.ReadFile(path.string(), ai_pflags);
    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mNumMeshes <= 0) {
        return std::nullopt;
    }
    return extract_mesh_from_scene(scene, move_to_center, id, mesh_load_flags);
}

std::vector<std::shared_ptr<tostf::Mesh>> tostf::load_meshes(const std::filesystem::path& path,
                                                             const bool move_to_center, const unsigned ai_pflags,
                                                             const mesh_flag mesh_load_flags) {
    check_file_validity(path);
    log_info() << "Loading meshes from file: " << path.string();
    Assimp::Importer imp;
    const auto scene = imp.ReadFile(path.string(), ai_pflags);
    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mNumMeshes <= 0) {
        throw std::runtime_error{"Mesh file not working. Path: " + path.string() + "\n" + imp.GetErrorString()};
    }
    std::vector<std::shared_ptr<Mesh>> meshes(scene->mNumMeshes);
    for (unsigned i = 0; i < scene->mNumMeshes; i++) {
        auto mesh = extract_mesh_from_scene(scene, move_to_center, i, mesh_load_flags);
        if (mesh) {
            meshes.at(i) = mesh.value();
        }
    }
    return meshes;
}

std::map<std::string, std::shared_ptr<tostf::Mesh>> tostf::load_named_meshes(const std::filesystem::path& path,
                                                                             const bool move_to_center,
                                                                             const unsigned ai_pflags,
                                                                             const mesh_flag mesh_load_flags) {
    check_file_validity(path);
    log_info() << "Loading meshes from file: " << path.string();
    Assimp::Importer imp;
    const auto scene = imp.ReadFile(path.string(), ai_pflags);
    if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mNumMeshes <= 0) {
        throw std::runtime_error{"Mesh file not working. Path: " + path.string() + "\n" + imp.GetErrorString()};
    }
    std::map<std::string, std::shared_ptr<Mesh>> meshes;
    for (unsigned i = 0; i < scene->mNumMeshes; i++) {
        auto mesh = extract_mesh_from_scene(scene, move_to_center, i, mesh_load_flags);
        if (mesh) {
            meshes.emplace(std::make_pair<>(std::string(scene->mMeshes[i]->mName.C_Str()), mesh.value()));
        }
    }
    return meshes;
}

std::string tostf::gen_geom_indices_export_str(const std::shared_ptr<Geometry>& geometry, const unsigned index_offset) {
    std::stringstream face_str;
    if (geometry->has_uv_coords() && geometry->has_normals()) {
        for (int i = 0; i < static_cast<int>(geometry->indices.size()) / 3; i++) {
            const int index_a = index_offset + geometry->indices.at(i * 3 + 0) + 1;
            const int index_b = index_offset + geometry->indices.at(i * 3 + 1) + 1;
            const int index_c = index_offset + geometry->indices.at(i * 3 + 2) + 1;
            face_str << "f " << index_a << "/" << index_a << "/" << index_a
                << " " << index_b << "/" << index_b << "/" << index_b
                << " " << index_c << "/" << index_c << "/" << index_c << "\n";
        }
    }
    else if (geometry->has_uv_coords() && !geometry->has_normals()) {
        for (int i = 0; i < static_cast<int>(geometry->indices.size()) / 3; i++) {
            const int index_a = index_offset + geometry->indices.at(i * 3 + 0) + 1;
            const int index_b = index_offset + geometry->indices.at(i * 3 + 1) + 1;
            const int index_c = index_offset + geometry->indices.at(i * 3 + 2) + 1;
            face_str << "f " << index_a << "/" << index_a
                << " " << index_b << "/" << index_b
                << " " << index_c << "/" << index_c << "\n";
        }
    }
    else if (!geometry->has_uv_coords() && geometry->has_normals()) {
        for (int i = 0; i < static_cast<int>(geometry->indices.size()) / 3; i++) {
            const int index_a = index_offset + geometry->indices.at(i * 3 + 0) + 1;
            const int index_b = index_offset + geometry->indices.at(i * 3 + 1) + 1;
            const int index_c = index_offset + geometry->indices.at(i * 3 + 2) + 1;
            face_str << "f " << index_a << "/" << "/" << index_a
                << " " << index_b << "/" << "/" << index_b
                << " " << index_c << "/" << "/" << index_c << "\n";
        }
    }
    else {
        for (int i = 0; i < static_cast<int>(geometry->indices.size()) / 3; i++) {
            const int index_a = index_offset + geometry->indices.at(i * 3 + 0) + 1;
            const int index_b = index_offset + geometry->indices.at(i * 3 + 1) + 1;
            const int index_c = index_offset + geometry->indices.at(i * 3 + 2) + 1;
            face_str << "f " << index_a << " " << index_b << " " << index_c << "\n";
        }
    }
    return face_str.str();
}

std::string tostf::gen_geom_data_export_str(const std::shared_ptr<Geometry>& geometry, const int precision) {
    std::stringstream vert_str;
    std::stringstream normal_str;
    std::stringstream uv_str;
    vert_str.setf(std::ios::fixed, std::ios::floatfield);
    normal_str.setf(std::ios::fixed, std::ios::floatfield);
    uv_str.setf(std::ios::fixed, std::ios::floatfield);
    vert_str.precision(precision);
    normal_str.precision(precision);
    uv_str.precision(precision);
    std::thread vert_thread([&vert_str, geometry]() {
        for (const auto& v : geometry->vertices) {
            vert_str << "v " << v.x << " " << v.y << " " << v.z << "\n";
        }
    });
    std::thread normal_thread([&normal_str, geometry]() {
        if (geometry->has_normals()) {
            for (const auto& n : geometry->normals) {
                normal_str << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            }
        }
    });
    std::thread uv_thread([&uv_str, geometry]() {
        if (geometry->has_uv_coords()) {
            for (const auto& uv : geometry->uv_coords) {
                uv_str << "vt " << uv.x << " " << uv.y << "\n";
            }
        }
    });
    vert_thread.join();
    normal_thread.join();
    uv_thread.join();
    vert_str << normal_str.str() << uv_str.str();
    return vert_str.str();
}

void tostf::export_geom_as_obj(const std::shared_ptr<Geometry>& geometry, const std::filesystem::path& path,
                               const int precision) {
    save_str_to_file(path, gen_geom_data_export_str(geometry, precision));
    save_str_to_file(path, gen_geom_indices_export_str(geometry, 0), std::ios::out | std::ios::app);
}

std::string tostf::gen_geom_view_data_export_str(const Geometry_view& view, const int precision) {
    std::stringstream vert_str;
    std::stringstream normal_str;
    std::stringstream uv_str;
    vert_str.setf(std::ios::fixed, std::ios::floatfield);
    normal_str.setf(std::ios::fixed, std::ios::floatfield);
    uv_str.setf(std::ios::fixed, std::ios::floatfield);
    vert_str.precision(precision);
    normal_str.precision(precision);
    uv_str.precision(precision);
    std::thread vert_thread([&vert_str, &view]() {
        for (int i = 0; i < static_cast<int>(view.indices.size()); i++) {
            auto& v = view.geometry->vertices.at(view.indices.at(i));
            vert_str << "v " << v.x << " " << v.y << " " << v.z << "\n";
        }
    });
    std::thread normal_thread([&normal_str, &view]() {
        if (view.geometry->has_normals()) {
            for (int i = 0; i < static_cast<int>(view.indices.size()); i++) {
                auto& n = view.geometry->normals.at(view.indices.at(i));
                normal_str << "vn " << n.x << " " << n.y << " " << n.z << "\n";
            }
        }
    });
    std::thread uv_thread([&uv_str, &view]() {
        if (view.geometry->has_uv_coords()) {
            for (int i = 0; i < static_cast<int>(view.indices.size()); i++) {
                auto& uv = view.geometry->uv_coords.at(view.indices.at(i));
                uv_str << "vt " << uv.x << " " << uv.y << "\n";
            }
        }
    });
    vert_thread.join();
    normal_thread.join();
    uv_thread.join();
    vert_str << normal_str.str() << uv_str.str();
    return vert_str.str();
}

std::string tostf::gen_geom_view_indices_export_str(const Geometry_view& view,
                                                    const unsigned index_offset) {
    std::stringstream face_str;
    if (view.geometry->has_uv_coords() && view.geometry->has_normals()) {
        for (int i = 0; i < static_cast<int>(view.indices.size()) / 3; i++) {
            const int index_a = index_offset + i * 3 + 0 + 1;
            const int index_b = index_offset + i * 3 + 1 + 1;
            const int index_c = index_offset + i * 3 + 2 + 1;
            face_str << "f " << index_a << "/" << index_a << "/" << index_a
                << " " << index_b << "/" << index_b << "/" << index_b
                << " " << index_c << "/" << index_c << "/" << index_c << "\n";
        }
    }
    else if (view.geometry->has_uv_coords() && !view.geometry->has_normals()) {
        for (int i = 0; i < static_cast<int>(view.indices.size()) / 3; i++) {
            const int index_a = index_offset + i * 3 + 0 + 1;
            const int index_b = index_offset + i * 3 + 1 + 1;
            const int index_c = index_offset + i * 3 + 2 + 1;
            face_str << "f " << index_a << "/" << index_a
                << " " << index_b << "/" << index_b
                << " " << index_c << "/" << index_c << "\n";
        }
    }
    else if (!view.geometry->has_uv_coords() && view.geometry->has_normals()) {
        for (int i = 0; i < static_cast<int>(view.indices.size()) / 3; i++) {
            const int index_a = index_offset + i * 3 + 0 + 1;
            const int index_b = index_offset + i * 3 + 1 + 1;
            const int index_c = index_offset + i * 3 + 2 + 1;
            face_str << "f " << index_a << "/" << "/" << index_a
                << " " << index_b << "/" << "/" << index_b
                << " " << index_c << "/" << "/" << index_c << "\n";
        }
    }
    else {
        for (int i = 0; i < static_cast<int>(view.indices.size()) / 3; i++) {
            const int index_a = index_offset + i * 3 + 0 + 1;
            const int index_b = index_offset + i * 3 + 1 + 1;
            const int index_c = index_offset + i * 3 + 2 + 1;
            face_str << "f " << index_a << " " << index_b << " " << index_c << "\n";
        }
    }
    return face_str.str();
}

std::string tostf::gen_geom_view_export_str(const Geometry_view& view, const unsigned index_offset,
                                            const int precision) {
    std::stringstream view_exp_str;
    view_exp_str << gen_geom_view_data_export_str(view, precision);
    view_exp_str << gen_geom_view_indices_export_str(view, index_offset);
    return view_exp_str.str();
}

void tostf::export_geom_view_as_obj(const Geometry_view& view, const std::filesystem::path& path,
                                    const unsigned index_offset, const int precision) {
    save_str_to_file(path, gen_geom_view_export_str(view, index_offset, precision));
}
