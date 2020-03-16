//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <vector>
#include <filesystem>
#include "math/glm_helper.hpp"
#include <variant>
#include "geometry/geometry.hpp"

namespace tostf
{
    namespace foam
    {
        enum class file_format {
            ascii,
            binary
        };

        struct Header {
            std::string object;
            file_format format;
        };

        struct Grid { };

        struct Foam_boundary {
            std::string name;
            int start_id;
            int size;
        };

        struct Foam_owner {
            int cell_count{};
            int internal_faces_count{};
            std::vector<int> owner;
            std::vector<int> neighbor;
            void load_cells_from_file(const std::filesystem::path& path);
        };

        struct Foam_face_it {
            std::vector<int>::iterator begin;
            std::vector<int>::iterator end;
        };

        struct Foam_face_handler {
            std::vector<int> point_refs;
            std::vector<Foam_face_it> face_it_refs;
            void load_faces_from_file(const std::filesystem::path& path);
        };

        struct Poly_mesh_boundary {
            std::string name;
            std::vector<glm::vec4> points;
            std::vector<float> radii;
            std::vector<int> cell_refs;
        };

        template <typename T>
        struct Field {
            float min{FLT_MAX};
            float max{-FLT_MAX};
            float avg{0};
            std::vector<T> internal_data{};
            std::vector<std::vector<T>> boundaries_data;
        };

        struct Poly_mesh {
            void load(std::filesystem::path p);
            std::filesystem::path case_path;
            Bounding_box mesh_bb;
            std::vector<glm::vec4> cell_centers;
            std::vector<float> cell_radii;
            std::vector<Poly_mesh_boundary> boundaries;
            std::string gen_datapoint_str();
            Field<float> load_scalar_field_from_file(const std::string& step_path, const std::string& field_name) const;
            Field<glm::vec4> load_vector_field_from_file(const std::string& step_path,
                                                         const std::string& field_name) const;
        };

        bool is_vector_field_file(const std::filesystem::path& file_path);
        std::vector<glm::vec4> load_points_from_file(const std::filesystem::path& path);
        std::vector<Foam_boundary> load_boundaries_from_file(const std::filesystem::path& path);
    }
}
