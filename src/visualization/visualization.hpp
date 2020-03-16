//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <memory>
#include "gpu_interface/vao.hpp"
#include "gpu_interface/shader_program.hpp"
#include "geometry/primitives.hpp"
#include <execution>

namespace tostf
{
    enum class vis_type : int {
        points,
        points_boundary,
        volume,
        volume_to_surface,
        volume_to_surface_phong,
        surface,
        streamlines,
        glyphs,
        pathlines,
        pathline_glyphs,
        //ostium,
        count
    };

    inline std::string vis_type_to_str(const vis_type type) {
        switch (type) {
            case vis_type::points:
                return "Points";
            case vis_type::points_boundary:
                return "Boundary points";
            case vis_type::volume:
                return "Volume";
            case vis_type::volume_to_surface:
                return "Volume to surface";
            case vis_type::volume_to_surface_phong:
                return "Phong shaded volume to surface";
            case vis_type::surface:
                return "Surface";
            case vis_type::streamlines:
                return "Streamlines";
            case vis_type::glyphs:
                return "Glyphs";
            case vis_type::pathlines:
                return "Pathlines";
            case vis_type::pathline_glyphs:
                return "Pathline glyphs";
            //case vis_type::ostium:
                //return "2D aneurysm map";
            default:
                return "Surface";
        }
    }

    struct vis_ssbo_defines {
		enum ssbo_defines {
			diffuse = 1,
			camera = 2,
			min_max = 3,
			volume_grid = 4,
			clipping_planes = 10,
			streamline_seeds = 11,
			streamlines = 12,
			pathline_integrator = 13
        };
    };

    inline std::vector<Shader_define> shader_defines{
        {"DIFFUSE_LIGHTING", vis_ssbo_defines::diffuse},
        {"CAM_BUFFER_BINDING", vis_ssbo_defines::camera},
        {"MIN_MAX_BUFFER", vis_ssbo_defines::min_max},
        {"VOLUME_GRID_BUFFER", vis_ssbo_defines::volume_grid},
        {"SEEDS_SSBO_BINDING", vis_ssbo_defines::streamline_seeds},
        {"STREAMLINES_SSBO_BINDING", vis_ssbo_defines::streamlines},
        {"PATHLINE_SSBO_BINDING", vis_ssbo_defines::pathline_integrator},
		{"CLIPPING_PLANE_BUFFER", vis_ssbo_defines::clipping_planes}
    };

    struct Points_vis {
        Points_vis() {
            shader = std::make_unique<V_G_F_shader>("../../shader/visualization/points_colormapped.vert",
                                                    "../../shader/visualization/points_colormapped.geom",
                                                    "../../shader/visualization/points_colormapped.frag",
                                                    shader_defines);
        }

        std::unique_ptr<V_G_F_shader> shader;
    };

    struct Volume_vis {
        Volume_vis() {
            scalar_to_texture = std::make_unique<V_G_F_shader>("../../shader/data_to_voxelgrid/scalar_to_texture.vert",
                                                               "../../shader/data_to_voxelgrid/scalar_to_texture.geom",
                                                               "../../shader/data_to_voxelgrid/scalar_to_texture.frag",
                                                               shader_defines);
            vector_to_texture = std::make_unique<V_G_F_shader>("../../shader/data_to_voxelgrid/vector_to_texture.vert",
                                                               "../../shader/data_to_voxelgrid/vector_to_texture.geom",
                                                               "../../shader/data_to_voxelgrid/vector_to_texture.frag",
                                                               shader_defines);
            volume_to_surface = std::make_unique<V_F_shader>("../../shader/visualization/volume/volume_to_surface.vert",
                                                             "../../shader/visualization/volume/volume_to_surface.frag",
                                                             shader_defines);
            volume_to_plane = std::make_unique<V_F_shader>("../../shader/visualization/volume/slice.vert",
                                   "../../shader/visualization/volume/volume_to_plane.frag", shader_defines);
            volume_to_surface_phong = std::make_unique<V_F_shader>("../../shader/visualization/volume/volume_to_surface_phong.vert",
                                                             "../../shader/visualization/volume/volume_to_surface_phong.frag",
                                                             shader_defines);
            volume_to_plane_phong = std::make_unique<V_F_shader>("../../shader/visualization/volume/slice_phong.vert",
                                   "../../shader/visualization/volume/volume_to_plane_phong.frag", shader_defines);
            clipped_stencil = std::make_unique<V_F_shader>("../../shader/clipped_stencil.vert",
                                     "../../shader/rendering/white.frag", shader_defines);
            clipped_stencil_planes = std::make_unique<V_F_shader>("../../shader/visualization/volume/slice.vert",
                                     "../../shader/rendering/white.frag", shader_defines);
            volume_ray_casting = std::make_unique<V_F_shader>(
                "../../shader/visualization/volume/volume_ray_casting.vert",
                "../../shader/visualization/volume/volume_ray_casting.frag", shader_defines);
        }

        std::unique_ptr<V_G_F_shader> scalar_to_texture;
        std::unique_ptr<V_G_F_shader> vector_to_texture;
        std::unique_ptr<V_F_shader> volume_to_surface;
        std::unique_ptr<V_F_shader> volume_to_plane;
        std::unique_ptr<V_F_shader> volume_to_surface_phong;
        std::unique_ptr<V_F_shader> volume_to_plane_phong;
        std::unique_ptr<V_F_shader> clipped_stencil;
        std::unique_ptr<V_F_shader> clipped_stencil_planes;
        std::unique_ptr<V_F_shader> volume_ray_casting;
    };

    struct Surface_vis {
        Surface_vis() {
            shader = std::make_unique<V_F_shader>("../../shader/visualization/surface/surface_colormapped.vert",
                                                  "../../shader/visualization/surface/surface_colormapped.frag",
                                                  shader_defines);
        }

        std::unique_ptr<V_F_shader> shader;
    };

    struct Flow_vis {
        Flow_vis() {
            generate_streamlines = std::make_unique<Compute_shader>(
                "../../shader/visualization/flow/generate_streamlines.comp", shader_defines);

            gen_pathlines_start_step = std::make_unique<Compute_shader>(
                "../../shader/visualization/flow/pathlines/gen_pathlines_start_step.comp", shader_defines);
            gen_pathlines_half_step = std::make_unique<Compute_shader>(
                "../../shader/visualization/flow/pathlines/gen_pathlines_half_step.comp", shader_defines);
            gen_pathlines_integrate = std::make_unique<Compute_shader>(
                "../../shader/visualization/flow/pathlines/gen_pathlines_integrate.comp", shader_defines);

            render_streamlines = std::make_unique<V_G_F_shader>(
                "../../shader/visualization/flow/streamlines_colormapped.vert",
                "../../shader/visualization/flow/streamlines_colormapped.geom",
                "../../shader/visualization/flow/streamlines_colormapped.frag",
                shader_defines);

			render_streamlines = std::make_unique<V_G_F_shader>(
				"../../shader/visualization/flow/streamlines_colormapped.vert",
				"../../shader/visualization/flow/streamlines_colormapped.geom",
				"../../shader/visualization/flow/streamlines_colormapped.frag",
				shader_defines);
            render_glyphs = std::make_unique<V_G_F_shader>(
                "../../shader/visualization/flow/glyphs_colormapped.vert",
                "../../shader/visualization/flow/glyphs_colormapped.geom",
                "../../shader/visualization/flow/glyphs_colormapped.frag",
                shader_defines);
            render_surface_front = std::make_unique<V_F_shader>(
                "../../shader/visualization/flow/surface_streamlines.vert",
                "../../shader/visualization/flow/translucent_surface_front.frag", shader_defines);
            render_surface_back = std::make_unique<V_F_shader>(
                "../../shader/visualization/flow/surface_streamlines.vert",
                "../../shader/visualization/flow/translucent_surface_back.frag", shader_defines);
            render_sphere = std::make_unique<V_F_shader>(
                "../../shader/visualization/flow/sphere.vert",
                "../../shader/visualization/flow/sphere.frag", shader_defines);
            sphere_vao = std::make_unique<VAO>();
            const auto sphere = std::make_shared<Sphere>(1.0f);
            sphere_vbos = create_vbos_from_geometry(sphere);
            sphere_vao->attach_vbos(sphere_vbos, true);
        }

        std::unique_ptr<Compute_shader> generate_streamlines;
        std::unique_ptr<Compute_shader> gen_pathlines_start_step;
        std::unique_ptr<Compute_shader> gen_pathlines_half_step;
        std::unique_ptr<Compute_shader> gen_pathlines_integrate;
        std::unique_ptr<V_G_F_shader> render_streamlines;
        std::unique_ptr<V_G_F_shader> render_glyphs;
        std::unique_ptr<V_F_shader> render_surface_front;
        std::unique_ptr<V_F_shader> render_surface_back;
        std::unique_ptr<VAO> sphere_vao;
        std::vector<VAO_attachment> sphere_vbos;
        std::unique_ptr<V_F_shader> render_sphere;
    };

    struct Diffuse_vis_settings {
        glm::vec4 ambient_color{0.1f, 0.1f, 0.1f, 1.0f};
        glm::vec4 light_pos{0.0f, 0.0f, 10.0f, 1.0f};
        glm::vec4 light_color{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 object_color{1.0f, 1.0f, 1.0f, 1.0f};
        float shininess = 1.0f;
    };

    struct Cam_data {
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct Min_max_data {
        float min;
        float max;
    };

    struct Visualization_selector {
        Visualization_selector() {
            Cam_data cam_data{glm::mat4(1.0f), glm::mat4(1.0f)};
            diffuse_ssbo = std::make_unique<SSBO>(vis_ssbo_defines::diffuse, diffuse_settings);
            cam_ssbo = std::make_unique<SSBO>(vis_ssbo_defines::camera, cam_data);
            minmax_ssbo = std::make_unique<SSBO>(vis_ssbo_defines::min_max, Min_max_data{0.0f, 0.0f});
            divide_by_count = std::make_unique<Compute_shader>("../../shader/data_to_voxelgrid/divide_by_count.comp");
            helper_vao = std::make_unique<VAO>();
        }

        void update_camera(const glm::mat4& view, const glm::mat4& proj) const {
            cam_ssbo->set_data(Cam_data{view, proj});
        }

        void update_minmax(const float min, const float max) const {
            minmax_ssbo->set_data(Min_max_data{min, max});
        }

        void activate_vis(const vis_type t) {
            _current_vis = t;
        }

        vis_type get_current_vis_type() const {
            return _current_vis;
        }

        Points_vis points_vis;
        Surface_vis surface_vis;
        Volume_vis volume_vis;
        Flow_vis flow_vis;
        std::unique_ptr<Compute_shader> divide_by_count;
        Diffuse_vis_settings diffuse_settings;
        std::unique_ptr<SSBO> diffuse_ssbo;
        std::unique_ptr<SSBO> cam_ssbo;
        std::unique_ptr<SSBO> minmax_ssbo;
        std::unique_ptr<VAO> helper_vao;
        bool display_internal = true;
        bool display_boundary = false;

    private:
        vis_type _current_vis = vis_type::surface;
    };
}
