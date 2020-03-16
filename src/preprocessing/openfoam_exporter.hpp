//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#include <string>
#include "math/glm_helper.hpp"
#include "geometry/geometry.hpp"
#include <filesystem>
#include "aneurysm/aneurysm_viewer.hpp"
#include "mesh_deformation.hpp"

namespace tostf
{
    namespace foam
    {
        constexpr const char* source_openfoam = "source /opt/OpenFOAM/OpenFOAM-v1806/etc/bashrc;";
        enum class write_format : int {
            binary = 0,
            human_readable = 1
        };

        inline std::string write_format_to_string(const write_format format) {
            if (format == write_format::binary) {
                return "binary";
            }
            return "ascii";
        }

        struct Algorithm_settings {
            std::string get_application_name() const;
			bool steady = true;
            int n_correctors = 3;
            int n_non_orthogonal_correctors = 2;
            int n_outer_correctors = 200;
            bool momentum_predictor = true;
            int ref_cell = 0;
            int ref_value = 0;
            float tolerance_p = 0.00001f;
            float tolerance_U = 0.00001f;
        };

        struct Control_dict {
            std::string to_string(bool enable_iter) const;
            float duration = 1.0f;
			int iterations = 1000;
			int iter_write_interval = 100;
            int cycles = 1;
            int cycles_to_overwrite = 0;
            float dt = 0.00001f;
            int time_precision = 7;
            int write_precision = 7;
            float write_interval = 0.001f;
            write_format format = write_format::binary;
            float max_co = 1; // cfl
            float max_dt = 0.001f;
        };

        struct Dimensions {
            std::string to_string() const;
            int kg = 0;
            int m = 0;
            int s = 0;
            int K = 0;
            int mol = 0;
            int A = 0;
            int cd = 0;
        };

        constexpr Dimensions pressure_dims() {
            return {0, 2, -2, 0, 0, 0, 0};
        }

        constexpr Dimensions velocity_dims() {
            return {0, 1, -1, 0, 0, 0, 0};
        }

        constexpr Dimensions nu_dims() {
            return {0, 2, -1, 0, 0, 0, 0};
        }

        constexpr Dimensions no_dims() {
            return {0, 0, 0, 0, 0, 0, 0};
        }

        struct Foam_field {
            Foam_field() = default;
            Foam_field(std::string in_name, const Dimensions& in_dims);
            std::string name{};
            Dimensions dims{};
        };

        struct Scalar_field : Foam_field {
            Scalar_field() = default;
            Scalar_field(const std::string& in_name, const Dimensions& in_dims, float in_internal_value);
            float internal_value = 0;
        };

        struct Vector_field : Foam_field {
            Vector_field() = default;
            Vector_field(const std::string& in_name, const Dimensions& in_dims, const glm::vec3& in_internal_value);
            glm::vec3 internal_value{};
        };

        enum class field_solver_types : int {
            pcg,
            pbicg,
            pbicg_stab,
            smooth_solver,
            gamg,
            diagonal
        };

        std::string solver_type_to_str(field_solver_types t);

        enum class field_preconditioner : int {
            dic,
            fdic,
            dilu,
            diagonal,
            gamg,
            none
        };

        std::string preconditioner_to_str(field_preconditioner p);

        enum class field_smoother : int {
            gauss_seidel,
            sym_gauss_seidel,
            dic_gauss_seidel,
            dic,
            dilu
        };

        std::string smoother_to_str(field_smoother s);

        struct Field_solver {
            std::string to_string() const;
            field_solver_types solver;
            field_preconditioner preconditioner;
            field_smoother smoother;
            float tolerance;
            float relative_tolerance;
            float relaxation;
        };

        struct Solvers {
            std::string to_string() const;
            Field_solver p{
                field_solver_types::gamg, field_preconditioner::dic, field_smoother::gauss_seidel,
                0.000001f, 0.001f, 0.3f
            };
            Field_solver U{
                field_solver_types::smooth_solver, field_preconditioner::dilu, field_smoother::gauss_seidel,
                0.00000001f, 0.0f, 0.7f
            };
        };

        struct Scheme {
            std::string to_string() const;
            std::string name;
            std::string setting;
        };

        struct Schemes_settings {
            std::string to_string() const;
            std::vector<Scheme> ddt;
            std::vector<Scheme> grad;
            std::vector<Scheme> div;
            std::vector<Scheme> laplacian;
            std::vector<Scheme> interpolation;
            std::vector<Scheme> sn_grad;
        };

        enum class numerical_schemes : int {
            commercial,
            accurate,
            stable
        };

        Schemes_settings schemes_commercial_setup();
        Schemes_settings schemes_accurate_setup();
        Schemes_settings schemes_stable_setup();

        enum class transport_model : int {
            newtonian,
            bird_carreau,
            cross_power_law,
            power_law,
            herschel_bulkley,
            casson
        };

        std::string transport_model_to_str(transport_model tm);

        std::vector<Scalar_field> fields_from_model(transport_model model);

        struct Blood_flow_properties {
            float dynamic_viscosity = 0.004f;
            float density = 1055.0f;
            Vector_field U{"U", velocity_dims(), glm::vec3(0.0f)};
            Scalar_field p{"p", pressure_dims(), 0.0f};
            transport_model model = transport_model::newtonian;
            std::vector<Scalar_field> transport_fields;
        };

        struct Block_mesh_controls {
            float bb_offset = 1.0f;
            float initial_cell_size = 1.0f;
            int min_cells = 100;
        };

        struct Decompose_par_dict {
            int max_sub_domains = 2;
        };

        struct Surface_feature_extract {
            std::string to_string() const;
            int included_angle = 180;
            bool non_manifold_edges = false;
            bool open_edges = true;
        };

        struct Castellated_mesh_controls {
            std::string to_string() const;
            int max_local_cells = 500000;
            int max_global_cells = 5000000;
            int min_refinement_cells = 0;
            int n_cells_between_levels = 3;
            int resolve_feature_angle = 30;
            bool allow_free_standing_zone_faces = true;
            int features_level_refinement = 1;
            int refinement_surfaces_min_level = 0;
            int refinement_surfaces_max_level = 1;
        };

        struct Snap_controls {
            std::string to_string() const;
            int n_smooth_patch = 3;
            float tolerance = 2.0;
            int n_solve_iter = 100;
            int n_relax_iter = 5;
            int n_feature_snap_iter = 10;
        };

        struct Add_layers_controls { };

        struct Mesh_quality_controls {
            int max_non_ortho = 65;
            int max_boundary_skewness = 20;
            int max_internal_skewness = 4;
            int max_concave = 80;
            float min_flatness = 0.5;
        };

        struct Snappy_hex_mesh_controls {
            Castellated_mesh_controls castellated{};
            Snap_controls snap{};
            std::optional<Add_layers_controls> add_layers;
            bool snap_active = true;
        };

        struct Post_processing {
            bool lambda2 = true;
            bool wall_shear_stress = true;
            bool q_criterion = true;
        };

        struct OpenFOAM_export_info {
            std::filesystem::path export_path;
            Algorithm_settings algorithm{};
            Control_dict control_dict{};
            numerical_schemes schemes = numerical_schemes::commercial;
            Blood_flow_properties blood_flow{};
            Solvers solvers{};
            Block_mesh_controls block_mesh{};
            Decompose_par_dict decompose_par_dict{};
            Surface_feature_extract feature_extract{};
            Snappy_hex_mesh_controls snappy_hex_mesh{};
            Mesh_quality_controls mesh_quality{};
            Post_processing post_processing{};
            bool execute_export = false;
            bool inlet_specified = false;
            bool parallel = true;
        };

        void export_openfoam_cases(const std::string& name, const OpenFOAM_export_info& info, float scale, const Bounding_box& bb,
                                   float point_offset, const std::shared_ptr<Mesh>& mesh,
                                   const std::vector<Deformed_mesh>& deformed_meshes,
                                   const Geometry_view& wall, const std::vector<Geometry_view>& inlet_views,
                                   const std::vector<Inlet>& inlets);
    }
}
