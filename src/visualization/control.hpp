//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <utility>
#include "utility/vector.hpp"
#include "math/interpolation.hpp"
#include "glm/gtx/associated_min_max.inl"
#include "glm/gtx/component_wise.hpp"
#include "vis_utilities.hpp"
#include "foam_processing/volume_grid.hpp"
#include "visualization.hpp"
#include "math/advanced_techniques.hpp"
#include "assimp/postprocess.h"

namespace tostf::vis
{
    struct Field_description {
        Field_description() = default;

        explicit Field_description(const bool vector_field)
            : is_vector_field(vector_field) { }

        bool is_vector_field{};
        float min_scalar = FLT_MAX;
        float max_scalar = -FLT_MAX;
        bool minmax_set{};
    };

    struct Render_offset {
        Render_offset(const int c, const int o) {
            count = c;
            offset = o;
        }

        int count{};
        int offset{};
    };

    struct Surface_mesh_handler {
        void init(const std::map<std::string, std::shared_ptr<Mesh>>& meshes) {
            if (!meshes.empty()) {
                vao = std::make_unique<VAO>();
                std::vector<glm::vec4> vertices;
                std::vector<glm::vec4> normals;
                int offset = 0;
                for (auto& m : meshes) {
                    const auto count = static_cast<int>(m.second->vertices.size());
                    counts.emplace_back(count, offset);
                    offset += count;
                    if (m.second->has_vertices() && m.second->has_normals()) {
                        vertices.insert(vertices.end(), m.second->vertices.begin(),
                                        m.second->vertices.end());
                        normals.insert(normals.end(), m.second->normals.begin(), m.second->normals.end());
                    }
                }
                if (!vertices.empty() && !normals.empty()) {
                    vbo_pos = std::make_shared<VBO>(vertices, 4);
                    vbo_normal = std::make_shared<VBO>(normals, 4);
                    vbo_scalar = std::make_shared<VBO>(1);
                    vao->attach_main_vbo(vbo_pos, 0);
                    vao->attach_vbo(vbo_normal, 1);
                    vbo_scalar->initialize_empty_storage(offset * sizeof(float));
                }
            }
        }

        std::vector<Render_offset> counts;
        std::unique_ptr<VAO> vao;
        std::shared_ptr<VBO> vbo_pos;
        std::shared_ptr<VBO> vbo_normal;
        std::shared_ptr<VBO> vbo_scalar;
    };

    struct Points_handler {
        void init(const foam::Poly_mesh& foam_mesh) {
            vao = std::make_unique<VAO>();
            auto points = foam_mesh.cell_centers;
            auto radii = foam_mesh.cell_radii;
            auto offset = static_cast<int>(radii.size());
            const auto internal_count = offset;
            counts.emplace_back(offset, 0);
            for (auto& b : foam_mesh.boundaries) {
                const auto count = static_cast<int>(b.points.size());
                counts.emplace_back(count, offset);
                offset += count;
                points.insert(points.end(), b.points.begin(), b.points.end());
                radii.insert(radii.end(), b.radii.begin(), b.radii.end());
            }
            vbo_pos = std::make_shared<VBO>(points, 4);
            vbo_radii = std::make_shared<VBO>(radii, 1);
            vbo_scalar = std::make_shared<VBO>(1);
            vbo_vector = std::make_shared<VBO>(4);
            vao->attach_main_vbo(vbo_pos, 0);
            vao->attach_vbo(vbo_radii, 1);
            vbo_scalar->initialize_empty_storage(offset * sizeof(float));
            vbo_vector->initialize_empty_storage(internal_count * sizeof(glm::vec4));
        }

        std::vector<Render_offset> counts;
        std::unique_ptr<VAO> vao;
        std::shared_ptr<VBO> vbo_pos;
        std::shared_ptr<VBO> vbo_radii;
        std::shared_ptr<VBO> vbo_scalar;
        std::shared_ptr<VBO> vbo_vector;
    };

    struct Point_to_mesh_interpolation {
        std::vector<std::vector<float>> weights;
        std::vector<std::vector<int>> point_ids;
    };

    struct Volume_handler {
        void init(const Bounding_box& mesh_bb) {
            const auto cell_size = compMax(glm::vec3(mesh_bb.max - mesh_bb.min)) / 250.0f;
            const auto bb_min = mesh_bb.min - glm::vec4(glm::vec3(cell_size * 1.5f), 0.0f);
            const auto bb_max = mesh_bb.max + glm::vec4(glm::vec3(cell_size * 1.5f), 0.0f);
            const foam::Volume_grid vol_grid({bb_min, bb_max}, cell_size);
            tex_res res{vol_grid.cell_count.x, vol_grid.cell_count.y, vol_grid.cell_count.z};
            Texture_definition tex_def{tex::intern::rgba32f};
            volume_tex = std::make_unique<Texture3D>(res, tex_def);
            volume_fbo = std::make_unique<FBO>(volume_tex->get_res(), volume_tex->get_fbo_tex_def());
            cam_view = lookAt(glm::vec3(0, 0, bb_max.z + vol_grid.cell_size / 2.0f),
                              glm::vec3(0, 0, bb_min.z - vol_grid.cell_size / 2.0f),
                              glm::vec3(0, 1, 0));
            cam_ortho = glm::ortho(bb_min.x, bb_max.x, bb_min.y, bb_max.y, vol_grid.cell_size / 2.0f,
                                   vol_grid.cell_size * 1.5f);
            grid_info = vol_grid.get_grid_info();
        }

        std::unique_ptr<Texture3D> volume_tex;
        std::unique_ptr<FBO> volume_fbo;
        glm::mat4 cam_view{};
        glm::mat4 cam_ortho{};
        foam::Grid_info grid_info{};
    };

    struct Streamline_settings {
        float seed_sphere_radius = 1.0f;
        glm::vec4 seed_sphere_pos{0.0f, 0.0f, 0.0f, 1.0f};
        int line_count = 100;
        int step_count = 500;
        float step_size_factor = 0.3f;
        float line_radius = 0.001f;
        int glyphs_per_line = 20;
        bool equal_length = false;
    };

    struct Pathline_settings {
		float step_size = 0.01f;
        int start_pathline_step = 0;
        int max_pathline_steps = 0;
        int glyph_step_count = 20;
        int get_pathline_count() const {
            return max_pathline_steps - start_pathline_step;
        }
    };

    struct Pathline_integrator {
        glm::vec4 pos{0.0};
        glm::vec4 k1{0.0};
        glm::vec4 k2{0.0};
        glm::vec4 k3{0.0};
    };

    struct Flow_handler {
        void init(const float mesh_scale, const int max_pathline_steps) {
            vao = std::make_unique<VAO>();
            init_line_buffer();
            init_pathlines_buffer();
            init_seeds();
            settings.seed_sphere_radius = mesh_scale;
            pathline_settings.max_pathline_steps = max_pathline_steps;
        }

        void init_line_buffer() {
            lines = std::make_shared<SSBO>(vis_ssbo_defines::streamlines);
            lines->set_data(std::vector<glm::vec4>(settings.line_count * settings.step_count, glm::vec4(0)));
        }

        void init_pathlines_buffer() {
            pathlines = std::make_shared<SSBO>(vis_ssbo_defines::streamlines);
            pathlines->set_data(std::vector<glm::vec4>(settings.line_count * pathline_settings.max_pathline_steps, glm::vec4(0)));
        }

        void init_seeds() {
            auto seed_points = generate_points_on_sphere(settings.line_count, settings.seed_sphere_radius,
                                                     glm::vec3(settings.seed_sphere_pos));
            seeds = std::make_unique<SSBO>(vis_ssbo_defines::streamline_seeds, seed_points);
        }

        void init_pathline_integrators() {
            auto seed_points = generate_points_on_sphere(settings.line_count, settings.seed_sphere_radius,
                                                     glm::vec3(settings.seed_sphere_pos));
            std::vector<Pathline_integrator> integrators(seed_points.size());
#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(seed_points.size()); ++i) {
                integrators.at(i).pos = seed_points.at(i);
            }
            pathline_integrator = std::make_unique<SSBO>(vis_ssbo_defines::pathline_integrator, integrators);
        }

        void draw_streamlines() const {
            lines->bind_base(vis_ssbo_defines::streamlines);
            vao->draw(gl::draw::line_strip_adj, settings.line_count, settings.step_count);
        }

        void draw_pathlines(const int curr_step) const {
            if (curr_step >= pathline_settings.start_pathline_step) {
                pathlines->bind_base(vis_ssbo_defines::streamlines);
                vao->draw(gl::draw::line_strip_adj, settings.line_count,
                          curr_step - pathline_settings.start_pathline_step);
            }
        }

        void draw_pathline_glyphs(const int curr_step) const {
            if (curr_step >= pathline_settings.start_pathline_step) {
                pathlines->bind_base(vis_ssbo_defines::streamlines);
                vao->draw(gl::draw::line_strip_adj, settings.line_count,
                          glm::max(pathline_settings.glyph_step_count,
                              curr_step - pathline_settings.start_pathline_step), glm::max(0, curr_step - pathline_settings.glyph_step_count / 2));
            }
        }

        std::unique_ptr<VAO> vao;
        std::shared_ptr<SSBO> lines;
        std::shared_ptr<SSBO> pathlines;
		std::shared_ptr<SSBO> seeds;
        std::shared_ptr<SSBO> pathline_integrator;
		Streamline_settings settings;
        Pathline_settings pathline_settings;
        bool seeds_set = false;
        bool updated = false;
        bool pathlines_generated = false;
    };

    struct Clipping_plane {
        glm::vec4 point_on_plane{};
        glm::vec4 plane_normal{1.0f, 0.0f, 0.0f, 0.0f};
    };

    struct Clipping_handler {
        void init() {
            clipping_ssbo = std::make_unique<SSBO>(vis_ssbo_defines::clipping_planes);
            clipping_ssbo->set_data(std::vector<Clipping_plane>(max_planes_count));
        }

        std::vector<Clipping_plane> clipping_planes;
        std::unique_ptr<SSBO> clipping_ssbo;
        int max_planes_count = 8;
    };

    struct Overlay_settings {
        float mouse_radius = 0.1f;
        bool pattern = false;
        bool active = false;
        float type = 2.0f;
        float scale = 5.0f;
    };

    struct Case {
        explicit Case(std::filesystem::path case_path, bool skip_first)
            : path(std::move(case_path)) {
            mesh.load(path);
            for (auto& p : std::filesystem::directory_iterator(path)) {
                if (p.is_directory() && is_str_float(p.path().filename().string())) {
                    if (!skip_first) {
                        player.steps.push_back(p.path().filename().string());
                    }
                    skip_first = false;
                }
            }
            std::sort(player.steps.begin(), player.steps.end(), [](const std::string& a, const std::string& b) {
				return std::stof(a) < std::stof(b);
			});
            if (!player.steps.empty()) {
                for (auto& f : std::filesystem::directory_iterator(path / player.steps.at(0))) {
                    if (f.is_regular_file()) {
                        Field_description desc{foam::is_vector_field_file(f.path())};
                        fields.insert_or_assign(f.path().filename().string(), desc);
                    }
                }
            }
            auto mesh_path = std::filesystem::path(path / "constant" / "triSurface");
            for (auto& f : std::filesystem::directory_iterator(mesh_path)) {
                if (is_regular_file(f)) {
                    if (f.path().extension().string() == ".obj") {
                        mesh_path = f.path();
                        break;
                    }
                }
            }
            surface_mesh = load_named_meshes(mesh_path, false, aiProcess_FixInfacingNormals);
            if (!surface_mesh.empty()) {
                const auto surface_mesh_bb = surface_mesh.begin()->second->get_bounding_box();
                const auto poly_mesh_bb = mesh.mesh_bb;
                const auto exp_poly_mesh = pow(10.0f, get_float_exp(length(poly_mesh_bb.max
                                                                           - poly_mesh_bb.min)) - 1.0f);
                mesh_scale = exp_poly_mesh / pow(10.0f, get_float_exp(length(surface_mesh_bb.max
                                                                             - surface_mesh_bb.min)) - 1.0f);
                for (auto& m : surface_mesh) {
#pragma omp parallel for
                    for (int v_id = 0; v_id < static_cast<int>(m.second->vertices.size()); ++v_id) {
                        m.second->vertices.at(v_id) *= mesh_scale;
                        m.second->vertices.at(v_id).w = 1.0f;
                    }
                    mesh_interpolations.emplace(m.first, Point_to_mesh_interpolation());
                }
                surface_mesh.begin()->second->recalc_bounding_box();
#pragma omp parallel for
                for (int surface_id = 0; surface_id < static_cast<int>(surface_mesh.size()); ++surface_id) {
                    auto mesh_begin_it = surface_mesh.begin();
                    std::advance(mesh_begin_it, surface_id);
                    foam::Volume_grid helper_grid(mesh_begin_it->second->get_bounding_box(), 0.1 * exp_poly_mesh);
                    const auto& curr_boundary_points = mesh.boundaries.at(surface_id).points;
                    const auto& curr_boundary_radii = mesh.boundaries.at(surface_id).radii;
                    helper_grid.populate_cells(curr_boundary_points, curr_boundary_radii);
                    const auto name = mesh_begin_it->first;
                    const auto surface_size = mesh_begin_it->second->vertices.size();
                    const auto& surface_verts = mesh_begin_it->second->vertices;
                    mesh_interpolations.at(name).point_ids.resize(surface_size);
                    mesh_interpolations.at(name).weights.resize(surface_size);
#pragma omp parallel for
                    for (int v_id = 0; v_id < static_cast<int>(surface_size); ++v_id) {
                        const auto vert = surface_verts.at(v_id);
                        const auto cell_id = helper_grid.calc_cell_index(vert);
                        if (cell_id >= 0 && cell_id < helper_grid.total_cell_count) {
                            const auto& cell_data = helper_grid.cell_data_points.at(cell_id);
                            mesh_interpolations.at(name).point_ids.at(v_id).reserve(cell_data.size());
                            mesh_interpolations.at(name).weights.at(v_id).reserve(cell_data.size());
                            for (int dp_id : cell_data) {
                                const auto dist_ab = distance(vert, curr_boundary_points.at(dp_id));
                                if (dist_ab <= 2.0f * helper_grid.cell_radius) {
                                    if (dist_ab > 0.0f) {
                                        mesh_interpolations.at(name).point_ids.at(v_id).push_back(dp_id);
                                        const auto w_sqrt = glm::max(0.0f, 2.0f * helper_grid.cell_radius - dist_ab)
                                                            / (2.0f * helper_grid.cell_radius * dist_ab);
                                        mesh_interpolations.at(name).weights.at(v_id).push_back(
                                            w_sqrt * w_sqrt);
                                    }
                                    else {
                                        mesh_interpolations.at(name).point_ids.at(v_id) = {dp_id};
                                        mesh_interpolations.at(name).weights.at(v_id) = {1.0f};
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        inline foam::Field<float> load_scalar_field(const std::string& field_name) {
            const auto field = mesh.load_scalar_field_from_file(player.steps.at(player.current_step), field_name);
            fields.at(field_name).min_scalar = glm::min(fields.at(field_name).min_scalar, field.min);
            fields.at(field_name).max_scalar = glm::max(fields.at(field_name).max_scalar, field.max);
            return field;
        }

        inline void calc_minmax(const std::string& field_name) {
            if (!fields.at(field_name).minmax_set) {
                for (int s = 0; s < static_cast<int>(player.steps.size()); ++s) {
                    const auto field = mesh.load_scalar_field_from_file(player.steps.at(s), field_name);
                    fields.at(field_name).min_scalar = glm::min(fields.at(field_name).min_scalar, field.min);
                    fields.at(field_name).max_scalar = glm::max(fields.at(field_name).max_scalar, field.max);
                }
                fields.at(field_name).minmax_set = true;
            }
        }

        inline void export_current_step() {
            const auto time_step = player.steps.at(player.current_step);
            std::optional<std::string> empty_internal_data;
            std::vector<std::optional<std::string>> empty_boundary_data_strs(mesh.boundaries.size());
            if (!datapoint_str) {
                datapoint_str = mesh.gen_datapoint_str();
                save_str_to_file(export_path / "datapoints.txt", datapoint_str.value(), std::ios::out);
            }
            else {
                save_str_to_file(export_path / "datapoints.txt", datapoint_str.value(), std::ios::out);
            }
            for (auto f : fields) {
                log() << f.first;
                if (f.first != "p" && f.first != "U" && f.first != "Q" && f.first != "Lambda2" && f.first != "wallShearStress") {
                    continue;
                }
                std::string scalar_data_str;
                auto scalar_data = mesh.load_scalar_field_from_file(player.steps.at(player.current_step), f.first);
                if (!scalar_data.internal_data.empty()) {
                    std::stringstream strstr;
                    for (auto v : scalar_data.internal_data) {
                        strstr << v << "\n";
                    }
                    scalar_data_str.append(strstr.str());
                }
                else {
                    if (empty_internal_data) {
                        scalar_data_str.append(empty_internal_data.value());
                    }
                }
                for (unsigned b_id = 0; b_id < mesh.boundaries.size(); ++b_id) {
                    if (!scalar_data.boundaries_data.at(b_id).empty()) {
                        std::stringstream strstr;
                        for (auto v : scalar_data.boundaries_data.at(b_id)) {
                            strstr << v << "\n";
                        }
                        scalar_data_str.append(strstr.str());
                    }
                    else {
                        if (empty_boundary_data_strs.at(b_id)) {
                            scalar_data_str.append(empty_boundary_data_strs.at(b_id).value());
                        }
                    }
                }
                save_str_to_file(export_path / (f.first + "_" + time_step + ".txt"), scalar_data_str, std::ios::out);
            }
        }

        inline void init_renderers() {
            surface_renderer.init(surface_mesh);
            points_renderer.init(mesh);
            volume_renderer.init(mesh.mesh_bb);
            flow_renderer.init(mesh_scale, player.get_max_step_id());
            grid_ssbo = std::make_unique<SSBO>(vis_ssbo_defines::volume_grid);
            grid_ssbo->set_data(volume_renderer.grid_info);
            clipping.init();
            renderers_ready = true;
        }

        std::filesystem::path path;
        Surface_mesh_handler surface_renderer;
        Points_handler points_renderer;
        Volume_handler volume_renderer;
        std::unique_ptr<SSBO> grid_ssbo;
        Flow_handler flow_renderer;
        Clipping_handler clipping;
        Overlay_settings overlay_settings;
        foam::Poly_mesh mesh;
        std::map<std::string, Point_to_mesh_interpolation> mesh_interpolations;
        std::map<std::string, Field_description> fields;
        Player player;
        std::map<std::string, std::shared_ptr<Mesh>> surface_mesh;
        float mesh_scale = 1.0f;
        bool renderers_ready = false;
        std::filesystem::path export_path;
        std::optional<std::string> datapoint_str;
    };

    struct Case_comparer {
        void set_reference(const std::filesystem::path& case_path, const bool skip_first) {
            reference = std::make_unique<Case>(case_path, skip_first);
            if (comparing) {
                fields.clear();
                fields = merge_maps(reference->fields, comparing->fields);
            }
            else {
                fields = reference->fields;
            }
        }

        void set_comparing_case(const std::filesystem::path& case_path, const bool skip_first) {
            comparing = std::make_unique<Case>(case_path, skip_first);
            if (reference) {
                fields.clear();
                fields = merge_maps(reference->fields, comparing->fields);
            }
            else {
                fields = comparing->fields;
            }
        }

        void remove_reference() {
            reference.reset();
        }

        void remove_comparing_case() {
            comparing.reset();
        }

        bool has_reference() const {
            return reference != nullptr;
        }

        bool has_comparing() const {
            return comparing != nullptr;
        }

        Field_description get_current_field_desc() {
            if (selected_field < 0 || selected_field + 1 > static_cast<int>(fields.size())) {
                return Field_description{};
            }
            return fields.at(get_field_name(selected_field));
        }

        std::string get_field_name(const int field_id) {
            if (field_id < 0 || field_id > static_cast<int>(fields.size())) {
                return "No field";
            }
            auto it = fields.begin();
            std::advance(it, field_id);
            return it->first;
        }

        void set_time_per_step(const float time_per_step) const {
            if (reference) {
                reference->player.time_per_step = time_per_step;
            }
            if (comparing) {
                comparing->player.time_per_step = time_per_step;
            }
        }

        void play() const {
            if (reference) {
                reference->player.play();
            }
            if (comparing) {
                comparing->player.play();
            }
        }

        void pause() const {
            if (reference) {
                reference->player.pause();
            }
            if (comparing) {
                comparing->player.pause();
            }
        }

        bool is_playing() const {
            auto res = reference || comparing;
            if (reference) {
                res = res && reference->player.is_playing();
            }
            if (comparing) {
                res = res && comparing->player.is_playing();
            }
            return res;
        }

        void stop() const {
            if (reference) {
                reference->player.stop();
            }
            if (comparing) {
                comparing->player.stop();
            }
        }

        bool is_repeating() const {
            auto res = reference || comparing;
            if (reference) {
                res = res && reference->player.is_repeating();
            }
            if (comparing) {
                res = res && comparing->player.is_repeating();
            }
            return res;
        }

        void toggle_repeat() const {
            if (is_repeating()) {
                if (reference) {
                    reference->player.set_repeat(false);
                }
                if (comparing) {
                    comparing->player.set_repeat(false);
                }
            }
            else {
                if (reference) {
                    reference->player.set_repeat(true);
                }
                if (comparing) {
                    comparing->player.set_repeat(true);
                }
            }
        }

        void skip_next() const {
            auto skipped = true;
            if (reference) {
                skipped = reference->player.skip_next() && skipped;
            }
            if (comparing) {
                skipped = comparing->player.skip_next() && skipped;
            }
        }

        void skip_previous() const {
            auto skipped = true;
            if (reference) {
                skipped = reference->player.skip_previous() && skipped;
            }
            if (comparing) {
                skipped = comparing->player.skip_previous() && skipped;
            }
        }

        void advance_accumulator(const float dt) const {
            if (reference) {
                reference->player.advance_accumulator(dt);
            }
            if (comparing) {
                comparing->player.advance_accumulator(dt);
            }
        }

        std::unique_ptr<Case> reference;
        std::unique_ptr<Case> comparing;
        std::map<std::string, Field_description> fields;
        int selected_colormap = 0;
        int selected_field = -1;
        Legend legend;

    private:
        void merge_fields() {
            fields.clear();
            auto it_ref = reference->fields.begin();
            auto it_comp = comparing->fields.begin();
            while (it_ref != reference->fields.end() && it_comp != comparing->fields.end()) {
                if (it_ref->first < it_comp->first) {
                    ++it_ref;
                }
                else if (it_comp->first < it_ref->first) {
                    ++it_comp;
                }
                else {
                    Field_description desc;
                    desc.is_vector_field = it_ref->second.is_vector_field && it_comp->second.is_vector_field;
                    desc.min_scalar = glm::min(it_ref->second.min_scalar, it_comp->second.min_scalar);
                    desc.max_scalar = glm::min(it_ref->second.max_scalar, it_comp->second.max_scalar);
                    fields.insert_or_assign(it_ref->first, desc);
                    ++it_ref;
                    ++it_comp;
                }
            }
        }
    };

    enum class vis_view : int {
        both,
        reference_case,
        comparing_case,
        count
    };

    inline std::string to_string(const vis_view v) {
        switch (v) {
            case vis_view::reference_case:
                return "Reference case view";
            case vis_view::comparing_case:
                return "Comparing case view";
            default:
                return "Comparison view";
        }
    }

    struct View_selector {
        vis_view current_view = vis_view::both;
    };
}
