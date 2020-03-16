//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "geometry/geometry.hpp"
#include "data_processing/plane_fitting.hpp"
#include "mesh_processing/structured_mesh.hpp"

namespace tostf
{
    using truncation_plane = std::pair<Plane, Dataset_stats<glm::vec4>>;
    std::vector<truncation_plane> find_truncation_planes(const std::vector<glm::vec4>& edge_candidates, int k);

    enum class boundary_type : int {
        wall = 0,
        inlet = 1,
        outlet = 2
    };

    inline std::string boundary_type_to_str(const boundary_type b) {
        if (b == boundary_type::wall) {
            return "Wall";
        }
        if (b == boundary_type::inlet) {
            return "Inlet";
        }
        return "Outlet";
    }

    enum class boundary_condition {
        neumann,
        // zeroGradient
        dirichlet,
        // fixedValue
        empty
    };

    inline std::string boundary_condition_to_str(const boundary_condition bc) {
        switch (bc) {
            case boundary_condition::neumann:
                return "zeroGradient";
            case boundary_condition::dirichlet:
                return "fixedValue";
            case boundary_condition::empty:
            default:
                return "empty";
        }
    }

    struct Boundary {
        boundary_type type = boundary_type::outlet;
        boundary_condition U_condition = boundary_condition::neumann;
        glm::vec3 U_value{};
        boundary_condition p_condition = boundary_condition::dirichlet;
        float p_value = 0.0f;
    };

    struct Inlet {
        Inlet();
        Inlet(std::vector<std::array<unsigned, 3>>& in_indices, const glm::vec4& in_center, const glm::vec4& in_normal);
        void rename(const std::string& n);
        void set_boundary_type(boundary_type b_type);
        std::vector<std::array<unsigned, 3>> indices;
        glm::vec4 center{};
        glm::vec4 normal{};
        glm::vec4 tangent{};
        glm::vec4 bitangent{};
        float radius = 0.0f;
        std::string name;
        Boundary boundary{};
        bool renaming = false;
        glm::vec4 color{};
        float inflow_velocity = 0.5f;
    };

    std::vector<Inlet> detect_inlets_automatically(const Structured_mesh& mesh_data, int plane_count);

    struct Manual_inlet_selection {
        std::vector<float> highlighted;
        Inlet selection;
    };

    std::optional<Manual_inlet_selection> select_inlet_manually(const Structured_mesh& mesh_data,
                                                                unsigned selected_vertex);

    std::vector<Inlet> detect_inlets_from_holes(const Structured_mesh& mesh_data);
}
