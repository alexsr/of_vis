//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//
#define IMGUI_DEFINE_MATH_OPERATORS
#include "utility/logging.hpp"
#include "display/window.hpp"
#include "display/gui.hpp"
#include "gpu_interface/debug.hpp"
#include "core/shader_settings.hpp"
#include "foam_processing/foam_loader.hpp"
#include "file/file_handling.hpp"
#include "core/gpu_interface.hpp"
#include "geometry/primitives.hpp"
#include "utility/vector.hpp"
#include "geometry/mesh_handling.hpp"
#include "foam_processing/volume_grid.hpp"
#include "gpu_interface/storage_buffer.hpp"
#include "utility/str_conversion.hpp"
#include "iconfont/IconsMaterialDesignIcons.h"
#include "gpu_interface/fbo.hpp"

struct Colormap {
    std::unique_ptr<tostf::Texture2D> tex{};
    std::string name;
};

struct Colormap_handler {
    Colormap_handler(const std::filesystem::path& folder) {
        for (auto& p : std::filesystem::directory_iterator(folder)) {
            if (p.is_regular_file() && p.path().extension() == ".png") {
                maps.push_back({std::make_unique<tostf::Texture2D>(p.path()), p.path().filename().string()});
            }
        }
    }

    std::vector<Colormap> maps;
};


struct Player_step {
    std::string name;
    std::filesystem::path path;
};

struct Player {
    Player(const std::filesystem::path& main_folder, bool skip_first) {
        for (auto& p : std::filesystem::directory_iterator(main_folder)) {
            if (p.is_directory() && tostf::is_str_float(p.path().filename().string())) {
                if (!skip_first) {
                    steps.push_back({p.path().filename().string(), p.path()});
                }
                skip_first = false;
            }
        }
    }

    bool skip_next() {
        if (current_step + 1 < static_cast<int>(steps.size())) {
            ++current_step;
            return true;
        }
        return false;
    }

    bool skip_previous() {
        if (current_step - 1 >= 0) {
            --current_step;
            return true;
        }
        return false;
    }

    void check_advance() {
        if (accumulator > time_per_step) {
            accumulator = accumulator - time_per_step;
            if (!skip_next()) {
                playing = false;
                accumulator = 0.0f;
            }
        }
    }

    int current_step = 0;
    bool playing = false;
    float accumulator = 0.0f;
    float time_per_step = 0.125f;
    std::vector<Player_step> steps;
};

struct Scalar_field {
    float min;
    float max;
};

int main() {
    tostf::cmd::enable_color();
    const tostf::window_config win_conf{
        "Aneurysm mesh rendering", 1600, 900, 0, tostf::win_mgr::default_window_flags
    };
    const tostf::gl_settings gl_config{
        4, 6, true, tostf::gl_profile::core, 4,
        {1, 1, 1, 1}, {tostf::gl::cap::depth_test}
    };
    tostf::Window window(win_conf, gl_config);
    window.set_minimum_size({800, 800});
    tostf::Gui gui(window.get(), 22.0f);
    {
        auto& style = ImGui::GetStyle();
        style.WindowRounding = 0;
        style.ScrollbarRounding = 0.0f;
        style.ScrollbarSize = 20.0f;
        style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
        style.DisplayWindowPadding = ImVec2(0.0f, 0.0f);
        style.ChildBorderSize = 1.0f;
        ImGui::StyleColorsVS();
        // tostf::gl::set_clear_color(0.43f, 0.43f, 0.43f, 1);
    }
    tostf::gl::Debug_logger gl_debug_logger;
    gl_debug_logger.disable_severity(tostf::gl::dbg::severity::notification);
    auto path = std::filesystem::path(
        "C:/Users/alexa/Documents/projects/openfoam-env/z_deformed_test/reference_case/constant/polyMesh");
    auto case_path = std::filesystem::path(
        "C:/Users/alexa/Documents/projects/openfoam-env/z_deformed_test/reference_case/");
    Scalar_field p_field_minmax{};
    Player player(case_path, true);
    for (auto& step : player.steps) {
        auto temp_field = tostf::foam::load_scalar_field_from_file(step.path / "p");
        p_field_minmax.min = glm::min(p_field_minmax.min, temp_field.min);
        p_field_minmax.max = glm::max(p_field_minmax.max, temp_field.max);
    }
    Colormap_handler color_maps("../../resources/colormaps/");
    tostf::foam::Poly_mesh poly_mesh;
    poly_mesh.load_poly_mesh(path);
    auto points = poly_mesh.cell_centers;
    std::vector<float> radii(poly_mesh.cell_centers.size());
#pragma omp parallel for
    for (int p_id = 0; p_id < static_cast<int>(points.size()); ++p_id) {
        radii.at(p_id) = poly_mesh.cell_radii.at(p_id);
    }
    tostf::foam::Volume_grid test_grid(poly_mesh.mesh_bb, 0.0001f);
    // test_grid.populate_cells(poly_mesh, 0);
    const auto slice_vao = std::make_unique<tostf::VAO>();
    auto cam_radius = length(glm::vec3(poly_mesh.mesh_bb.max));
    tostf::Trackball_camera cam(cam_radius, 0.2f, 10.0f, true, glm::vec3(0.0f), 60.0f);
    tostf::V_F_shader visualization_shader("../../shader/rendering/screenfilling_quad.vert",
                                           "../../shader/visualization/volume_grid_slicing.frag");
    tostf::Texture3D test_tex({test_grid.cell_count.x, test_grid.cell_count.y, test_grid.cell_count.z},
                              {tostf::tex::intern::rg32f},
                              {tostf::tex::min_filter::linear, tostf::tex::mag_filter::linear});
    auto cam_mat_ortho = glm::ortho(poly_mesh.mesh_bb.min.x, poly_mesh.mesh_bb.max.x,
                                    poly_mesh.mesh_bb.min.y, poly_mesh.mesh_bb.max.y, test_grid.cell_size / 2.0f - 0.00001f,
                                    poly_mesh.mesh_bb.max.z - poly_mesh.mesh_bb.min.z + test_grid.cell_size * 1.5f + 0.00001f);
    tostf::FBO test_fbo(test_tex.get_res(), {test_tex.get_fbo_tex_def()});
    tostf::V_G_F_shader render_texture("../../shader/data_to_voxelgrid/points_to_texture_sliced.vert",
                                       "../../shader/data_to_voxelgrid/points_to_texture.geom",
                                       "../../shader/data_to_voxelgrid/points_to_texture.frag");
    tostf::Compute_shader divide_by_count("../../shader/data_to_voxelgrid/divide_by_count.comp");
    render_texture.update_uniform("proj", cam_mat_ortho);
    render_texture.update_uniform("bb_min", poly_mesh.mesh_bb.min);
    render_texture.update_uniform("cell_size", test_grid.cell_size);
    divide_by_count.update_uniform("data", test_tex.get_image_handle(tostf::gl::img_access::read_write, 0, 0, true));
    divide_by_count.update_uniform("default_value", -FLT_MAX);
    auto sphere_vao = std::make_unique<tostf::VAO>();
    const auto points_vbo = std::make_shared<tostf::VBO>(poly_mesh.cell_centers);
    const auto radii_vbo = std::make_shared<tostf::VBO>(poly_mesh.cell_radii, 1);
    sphere_vao->attach_main_vbo(points_vbo, 0);
    sphere_vao->attach_vbo(radii_vbo, 1);

    tostf::foam::Field<float> scalar_field;
    const auto load_field = [&]() {
        scalar_field = tostf::foam::load_vector_field_mag_field_from_file(
            player.steps.at(player.current_step).path / "U");
        const auto scalar_field_vbo = std::make_shared<tostf::VBO>(scalar_field.internal_data, 1);
        sphere_vao->attach_vbo(scalar_field_vbo, 2);
        tostf::gl::set_viewport(test_grid.cell_count.x, test_grid.cell_count.y);
        test_fbo.bind();
        tostf::gl::set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
        clear(tostf::gl::clear_bit::color | tostf::gl::clear_bit::depth);
        render_texture.use();
        disable(tostf::gl::cap::depth_test);
        disable(tostf::gl::cap::stencil_test);
        enable(tostf::gl::cap::blend);
        glBlendFunc(GL_ONE, GL_ONE);
        for (int layer = 0; layer < test_tex.get_res().z; ++layer) {
            auto cam_view = lookAt(glm::vec3(0, 0, poly_mesh.mesh_bb.min.z + test_grid.cell_size / 2.0f + layer * test_grid.cell_size),
                                   glm::vec3(0, 0, poly_mesh.mesh_bb.min.z - test_grid.cell_size / 2.0f),
                                   glm::vec3(0, 1, 0));
            render_texture.update_uniform("view", cam_view);
            render_texture.update_uniform("layer_in", layer);
            sphere_vao->draw(tostf::gl::draw::points);
        }
        disable(tostf::gl::cap::blend);
        enable(tostf::gl::cap::depth_test);
        tostf::FBO::unbind();
        divide_by_count.run(test_grid.cell_count.x, test_grid.cell_count.y, test_grid.cell_count.z);
        tostf::gl::set_viewport(window.get_resolution().x, window.get_resolution().y);
        tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1.0f);
        visualization_shader.update_uniform("data", test_tex.get_tex_handle());
    };
    load_field();
    float k = 0;
    glm::vec4 cam_origin(0.0f, 0.0f, 1.0f, 1.0f);
    glm::vec4 up(0.0f, 1.0f, 0.0f, 0.0f);
    auto cam_mat = lookAt(glm::vec3(cam_origin), glm::vec3(0.0f), glm::vec3(up));
    int active_colormap = 0;
    visualization_shader.update_uniform("k", k);
    visualization_shader.update_uniform("colormap", color_maps.maps.at(active_colormap).tex->get_tex_handle());
    const auto screenfilling_triangle = std::make_unique<tostf::VAO>();
    const float dt = 0.01f;
    while (!window.should_close()) {
        if (window.is_minimized()) {
            window.poll_events();
            continue;
        }
        tostf::gl::clear_all();
        gui.start_frame();
        if (ImGui::Begin("Test")) {
            if (ImGui::SliderFloat("z", &k, 0.0f, 1.0f, "%.5f")) {
                visualization_shader.update_uniform(
                    "k", k);
            }
        }
        ImGui::End();
        if (ImGui::Begin("Player")) {
            auto previous_step = player.current_step;
            ImGui::Text(player.steps.at(player.current_step).name.c_str());
            if (ImGui::Button(ICON_MDI_SKIP_PREVIOUS "##player")) {
                player.skip_previous();
            }
            ImGui::SameLine();
            if (player.playing) {
                player.accumulator += dt;
                player.check_advance();
                if (ImGui::Button(ICON_MDI_PAUSE "##player")) {
                    player.playing = false;
                    player.accumulator = 0.0f;
                }
            }
            else {
                if (ImGui::Button(ICON_MDI_PLAY "##player")) {
                    player.playing = true;
                    player.accumulator = 0.0f;
                    if (player.current_step == player.steps.size() - 1) {
                        player.current_step = 0;
                    }
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(ICON_MDI_SKIP_NEXT "##player")) {
                player.skip_next();
            }
            auto progress_player = static_cast<float>(player.current_step) /
                                   static_cast<float>(player.steps.size() - 1);
            ImGui::ProgressBar(progress_player, ImVec2(-1, 0), player.steps.at(player.current_step).name.c_str());
            auto progress_bar_width = ImGui::GetItemRectSize().x + 16.0f;
            ImGui::SameLine(-1);
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
            ImGui::PushItemWidth(progress_bar_width);
            ImGui::SliderInt("##player_progress_slider", &player.current_step, 0,
                             static_cast<int>(player.steps.size() - 1), "");
            ImGui::PopItemWidth();
            ImGui::PopStyleColor(3);
            if (previous_step != player.current_step) {
                load_field();
            }
        }
        ImGui::End();
        if (ImGui::Begin("Color maps")) {
            int i = 0;
            for (auto& m : color_maps.maps) {
                if (ImGui::RadioButton(m.name.c_str(), &active_colormap, i++)) {
                    visualization_shader.update_uniform(
                        "colormap", color_maps.maps.at(active_colormap).tex->get_tex_handle());
                }
                ImGui::SameLine();
                ImGui::Image(ImGui::ConvertGLuintToImTex(m.tex->get_name()), ImVec2(256.0f, 20.0f));
            }
        }
        ImGui::End();
        cam.update(window, dt);
        visualization_shader.use();
        visualization_shader.update_uniform("min_scalar", p_field_minmax.min);
        visualization_shader.update_uniform("max_scalar", p_field_minmax.max);
        screenfilling_triangle->draw(tostf::gl::draw::triangles, 1, 3);
        gui.render();
        window.swap_buffers();
        gl_debug_logger.retrieve_log();
        window.poll_events();
    }
    tostf::win_mgr::terminate();
    return 0;
}
