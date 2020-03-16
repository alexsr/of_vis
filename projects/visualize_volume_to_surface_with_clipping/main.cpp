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
#include "display/tostf_imgui_widgets.hpp"
#include "math/interpolation.hpp"
#include "assimp/postprocess.h"
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

struct Scalar_field_legend {
    float min;
    float max;
};

struct Legend {
    Legend(const float min_v, const float max_v, const int step_count) {
        min_value = min_v;
        max_value = max_v;
        precision = glm::max(3, glm::max(tostf::get_float_exp(max_v),
                                               tostf::get_float_exp(min_v)));
        const auto exp_10 = glm::pow(10, precision - 1);
        steps.resize(step_count + 2);
        const auto offset = glm::floor(min_v);
        const auto min_moved = min_v - offset;
        const auto max_moved = max_v - offset;
        const auto step_size = (max_moved - min_moved) / static_cast<float>(step_count + 1);
        steps.at(0) = min_v;
        steps.at(steps.size() - 1) = max_v;
        for (int i = 1; i < step_count - 1; ++i) {
            const auto step_pos = step_size + step_size * i;
            float exp_step_pos = exp_10 * step_pos;
            if (exp_step_pos < glm::floor(exp_step_pos) + 0.5f) {
                exp_step_pos = glm::floor(exp_step_pos) + 0.5f;
            }
            if (exp_step_pos >= glm::floor(exp_step_pos) + 0.5f) {
                exp_step_pos = glm::floor(exp_step_pos) + 1.0f;
            }
            steps.at(step_count - 1 - i) = exp_step_pos / static_cast<float>(exp_10) + offset;
        }
    }

    float value_to_height(const float step) const {
        return (step - min_value) / (max_value - min_value) * height;
    }

    float height = 256.0f;
    float line_length = 40.0f;
    float min_value;
    float max_value;
    int precision;
    std::vector<float> steps;
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
    tostf::V_F_shader plane_shader("../../shader/visualization/slice.vert",
                                   "../../shader/visualization/scalar_from_mesh.frag");
    tostf::VAO plane_vao;
    tostf::V_F_shader stencil_shader("../../shader/clipped_stencil.vert",
                                     "../../shader/rendering/white.frag");
    tostf::V_F_shader visualization_shader("../../shader/visualization/scalar_from_mesh_clipped.vert",
                                           "../../shader/visualization/scalar_from_mesh.frag");
    auto path = std::filesystem::path(
        "C:/Users/alexa/Documents/projects/openfoam-env/z_deformed_test/reference_case/constant/polyMesh");
    auto case_path = std::filesystem::path(
        "C:/Users/alexa/Documents/projects/openfoam-env/z_deformed_test/reference_case/");
    Scalar_field_legend p_field_minmax{};
    Player player(case_path, true);
    tostf::foam::Poly_mesh poly_mesh;
    poly_mesh.load(case_path);
    for (auto& step : player.steps) {
        const auto temp_field = poly_mesh.load_scalar_field_from_file(step.name, "p");
        p_field_minmax.min = glm::min(p_field_minmax.min, temp_field.min);
        p_field_minmax.max = glm::max(p_field_minmax.max, temp_field.max);
    }
    Colormap_handler color_maps("../../resources/colormaps/");
    Legend legend(p_field_minmax.min, p_field_minmax.max, 3);
    auto aneurysm_mesh = tostf::load_meshes(
        "C:/Users/alexa/Documents/projects/openfoam-env/z_deformed_test/reference_case/constant/triSurface/aneurysm.obj",
        true);
    auto an_bb = aneurysm_mesh.at(0)->get_bounding_box();
    auto cam_radius = length(glm::vec3(an_bb.max));
    an_bb.min /= 1000.0f;
    an_bb.max /= 1000.0f;
    auto aneurysm_vao = std::make_unique<tostf::VAO>();
    const auto aneurysm_vbos = create_vbos_from_geometry(aneurysm_mesh.at(0));
    aneurysm_vao->attach_vbos(aneurysm_vbos, true);
    auto cell_size = 0.00005f;
    an_bb.min = min(an_bb.min, poly_mesh.mesh_bb.min) - glm::vec4(glm::vec3(cell_size * 1.5f), 0.0f);
    an_bb.max = max(an_bb.max, poly_mesh.mesh_bb.max) + glm::vec4(glm::vec3(cell_size * 1.5f), 0.0f);
    tostf::foam::Volume_grid test_grid(an_bb, cell_size);
    tostf::Trackball_camera cam(cam_radius, 0.2f, 10.0f, true, glm::vec3(0.0f), 60.0f);
    tostf::Texture3D test_tex({test_grid.cell_count.x, test_grid.cell_count.y, test_grid.cell_count.z},
                              {tostf::tex::intern::rg32f},
                              {tostf::tex::min_filter::linear, tostf::tex::mag_filter::linear});
    auto cam_view = lookAt(glm::vec3(0, 0, poly_mesh.mesh_bb.max.z + test_grid.cell_size / 2.0f),
                           glm::vec3(0, 0, poly_mesh.mesh_bb.min.z - test_grid.cell_size / 2.0f),
                           glm::vec3(0, 1, 0));
    auto cam_mat_ortho = glm::ortho(poly_mesh.mesh_bb.min.x, poly_mesh.mesh_bb.max.x,
                                    poly_mesh.mesh_bb.min.y, poly_mesh.mesh_bb.max.y, test_grid.cell_size / 2.0f,
                                    poly_mesh.mesh_bb.max.z - poly_mesh.mesh_bb.min.z + test_grid.cell_size * 1.5f);
    tostf::FBO test_fbo(test_tex.get_res(), {test_tex.get_fbo_tex_def()});
    tostf::V_G_F_shader render_texture("../../shader/data_to_voxelgrid/points_to_texture.vert",
                                       "../../shader/data_to_voxelgrid/points_to_texture.geom",
                                       "../../shader/data_to_voxelgrid/points_to_texture.frag");
    tostf::Compute_shader divide_by_count("../../shader/data_to_voxelgrid/divide_by_count.comp");
    render_texture.update_uniform("view", cam_view);
    render_texture.update_uniform("proj", cam_mat_ortho);
    render_texture.update_uniform("bb_min", an_bb.min);
    render_texture.update_uniform("cell_size", test_grid.cell_size);
    divide_by_count.update_uniform("data", test_tex.get_image_handle(tostf::gl::img_access::read_write, 0, 0, true));
    divide_by_count.update_uniform("default_value", -FLT_MAX);
    auto sphere_vao = std::make_unique<tostf::VAO>();
    tostf::foam::Field<float> scalar_field;
    const auto load_field = [&]() {
        scalar_field = poly_mesh.load_scalar_field_from_file(
            player.steps.at(player.current_step).name, "p");
        size_t datapoint_count = 0;
        if (scalar_field.internal_data) {
            datapoint_count += scalar_field.internal_data->size();
        }
        for (auto& b : scalar_field.boundaries_data) {
            if (b) {
                datapoint_count += b->size();
            }
        }
        std::vector<glm::vec4> points;
        points.reserve(datapoint_count);
        std::vector<float> radii;
        radii.reserve(datapoint_count);
        std::vector<float> scalars;
        scalars.reserve(datapoint_count);

        if (scalar_field.internal_data) {
            points.insert(points.end(), poly_mesh.cell_centers.begin(), poly_mesh.cell_centers.end());
            radii.insert(radii.end(), poly_mesh.cell_radii.begin(), poly_mesh.cell_radii.end());
            scalars.insert(scalars.end(), scalar_field.internal_data->begin(), scalar_field.internal_data->end());
        }
        for (size_t b_id = 0; b_id < scalar_field.boundaries_data.size(); ++b_id) {
            if (scalar_field.boundaries_data.at(b_id)) {
                points.insert(points.end(), poly_mesh.boundaries.at(b_id).points.begin(),
                              poly_mesh.boundaries.at(b_id).points.end());
                radii.insert(radii.end(), poly_mesh.boundaries.at(b_id).radii.begin(),
                             poly_mesh.boundaries.at(b_id).radii.end());
                scalars.insert(scalars.end(), scalar_field.boundaries_data.at(b_id)->begin(),
                               scalar_field.boundaries_data.at(b_id)->end());
            }
        }
        const auto points_vbo = std::make_shared<tostf::VBO>(points);
        const auto radii_vbo = std::make_shared<tostf::VBO>(radii, 1);
        const auto scalar_field_vbo = std::make_shared<tostf::VBO>(scalars, 1);
        sphere_vao->attach_main_vbo(points_vbo, 0);
        sphere_vao->attach_vbo(radii_vbo, 1);
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
        sphere_vao->draw(tostf::gl::draw::points, test_grid.cell_count.z);
        disable(tostf::gl::cap::blend);
        enable(tostf::gl::cap::depth_test);
        tostf::FBO::unbind();
        divide_by_count.run(test_grid.cell_count.x, test_grid.cell_count.y, test_grid.cell_count.z);
        tostf::gl::set_viewport(window.get_resolution().x, window.get_resolution().y);
        tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1.0f);
        visualization_shader.update_uniform("data", test_tex.get_tex_handle());
    };
    load_field();

    visualization_shader.update_uniform("min_scalar", p_field_minmax.min);
    visualization_shader.update_uniform("max_scalar", p_field_minmax.max);
    visualization_shader.update_uniform("data", test_tex.get_tex_handle());
    visualization_shader.update_uniform("cell_diag", test_grid.cell_radius);
    visualization_shader.update_uniform("cell_size", test_grid.cell_size);
    visualization_shader.update_uniform("bb_min", 1000.0f * test_grid.bb.min);
    visualization_shader.update_uniform("bb_max", 1000.0f * test_grid.bb.max);

    stencil_shader.update_uniform("bb_min", 1000.0f * test_grid.bb.min);
    stencil_shader.update_uniform("bb_max", 1000.0f * test_grid.bb.max);
    plane_shader.update_uniform("bb_min", 1000.0f * test_grid.bb.min);
    plane_shader.update_uniform("bb_max", 1000.0f * test_grid.bb.max);
    plane_shader.update_uniform("min_scalar", p_field_minmax.min);
    plane_shader.update_uniform("max_scalar", p_field_minmax.max);
    plane_shader.update_uniform("data", test_tex.get_tex_handle());
    plane_shader.update_uniform("cell_diag", test_grid.cell_radius);
    plane_shader.update_uniform("cell_size", test_grid.cell_size);

    int k = 0;
    int active_colormap = 0;
    visualization_shader.update_uniform("colormap", color_maps.maps.at(active_colormap).tex->get_tex_handle());
    plane_shader.update_uniform("colormap", color_maps.maps.at(active_colormap).tex->get_tex_handle());
    float dt = 0.01f;
    auto previous_step = player.current_step;
    auto current_time = std::chrono::high_resolution_clock::now();
    glEnable(GL_CLIP_DISTANCE0);
    while (!window.should_close()) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - current_time;
        current_time = now;
        if (window.is_minimized()) {
            window.poll_events();
            continue;
        }
        dt = elapsed.count();
        tostf::gl::clear_all();
        gui.start_frame();
        if (ImGui::Begin("Player")) {
            previous_step = player.current_step;
            ImGui::SliderInt("k", &k, 0, test_grid.cell_count.z - 1);
            visualization_shader.update_uniform(
                "k", static_cast<float>(k) / static_cast<float>(test_grid.cell_count.z - 1));
            stencil_shader.update_uniform("k", static_cast<float>(k) / static_cast<float>(test_grid.cell_count.z - 1));
            plane_shader.update_uniform("k", static_cast<float>(k) / static_cast<float>(test_grid.cell_count.z - 1));
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
                    plane_shader.update_uniform(
                        "colormap", color_maps.maps.at(active_colormap).tex->get_tex_handle());
                }
                ImGui::SameLine();
                ImGui::Image(ImGui::ConvertGLuintToImTex(m.tex->get_name()), ImVec2(256.0f, 20.0f));
            }
        }
        ImGui::End();
        const auto legend_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
                                         ImGuiWindowFlags_NoTitleBar |
                                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
        auto frame_padding = ImGui::GetStyle().FramePadding;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(35, 35, 35, 255));
        auto font_size = ImGui::GetFontSize();

        ImGui::SetNextWindowSize(ImVec2(150.0f, legend.height + font_size));
        if (ImGui::Begin("Legend", nullptr, legend_window_flags)) {
            auto legend_top_left = ImGui::GetCursorScreenPos();
            auto legend_x = legend_top_left.x;
            auto legend_y = legend_top_left.y;
            legend_y += font_size / 2.0f;
            ImGui::SetCursorScreenPos(ImVec2(legend_x, legend_y));
            ImGui::RotatedImage(ImGui::ConvertGLuintToImTex(color_maps.maps.at(active_colormap).tex->get_name()),
                                ImVec2(30.0f, legend.height));
            for (auto& s : legend.steps) {
                auto height_offset = legend.height - legend.value_to_height(s);
                ImGui::SetCursorScreenPos(ImVec2(legend_top_left.x + legend.line_length + 2.0f * frame_padding.x,
                                                 legend_y + height_offset - font_size / 2.0f - 2.0f));
                ImGui::Text(tostf::float_to_scientific_str(s, legend.precision).c_str());
                ImGui::GetWindowDrawList()->AddLine(ImVec2(legend_x, legend_y + height_offset),
                                                    ImVec2(legend_x + legend.line_length, legend_y + height_offset),
                                                    IM_COL32(35, 35, 35, 255), 2.0f);
            }
        }
        ImGui::End();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(5);
        cam.update(window, dt);

        // http://glbook.gamedev.net/GLBOOK/glbook.gamedev.net/moglgp/advclip.html
        glEnable(GL_CLIP_DISTANCE0);
        stencil_shader.use();
        stencil_shader.update_uniform("view", cam.get_view());
        stencil_shader.update_uniform("proj", cam.get_projection());
        enable(tostf::gl::cap::stencil_test);
        glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0);
        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
        disable(tostf::gl::cap::depth_test);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        aneurysm_vao->draw(tostf::gl::draw::triangles);
        glStencilFunc(GL_NOTEQUAL, 0, ~0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDisable(GL_CLIP_DISTANCE0);
        enable(tostf::gl::cap::depth_test);
        enable(tostf::gl::cap::stencil_test);
        plane_shader.use();
        plane_shader.update_uniform("view", cam.get_view());
        plane_shader.update_uniform("proj", cam.get_projection());
        plane_vao.draw(tostf::gl::draw::triangle_strip, 1, 4);
        disable(tostf::gl::cap::stencil_test);
        glEnable(GL_CLIP_DISTANCE0);
        visualization_shader.use();
        visualization_shader.update_uniform("view", cam.get_view());
        visualization_shader.update_uniform("proj", cam.get_projection());
        visualization_shader.update_uniform("min_scalar", p_field_minmax.min);
        visualization_shader.update_uniform("max_scalar", p_field_minmax.max);
        aneurysm_vao->draw(tostf::gl::draw::triangles);
        glDisable(GL_CLIP_DISTANCE0);
        gui.render();
        window.swap_buffers();
        gl_debug_logger.retrieve_log();
        window.poll_events();
    }
    tostf::win_mgr::terminate();
    return 0;
}
