//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "demo_utils.hpp"

int main() {
    tostf::cmd::enable_color();
    const tostf::window_config win_conf{
        "Aneurysm preprocessing tool", 1600, 900, 1, tostf::win_mgr::default_window_flags
    };
    const tostf::gl_settings gl_config{
        4, 6, true, tostf::gl_profile::core, 4,
        {1, 1, 1, 1}, {tostf::gl::cap::depth_test}
    };
    tostf::Window window(win_conf, gl_config);

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
    }
    tostf::gl::Debug_logger gl_debug_logger;
    gl_debug_logger.disable_severity(tostf::gl::dbg::severity::notification);
    gl_debug_logger.disable_severity(tostf::gl::dbg::severity::low);
    tostf::Event_profiler<std::chrono::milliseconds> ms_profiler;
    struct Simulation_info {
        std::filesystem::path dir_path;
        std::future<bool> finished;
        bool started = false;
        int case_count = 0;
        int mesh_gen_progress = 0;
        int sim_progress = 0;
        bool postprocess = false;
        int postprocess_progress = 0;
        bool parallel = false;
        std::string case_name;
    } sim_info;
    int active_tool = 0;
    while (!window.should_close()) {
        const auto win_size = window.get_resolution();
        tostf::View main_view({ 0, 0 }, win_size);
        main_view.activate();
        auto cam = std::make_shared<tostf::Trackball_camera>(2.0f, 0.2f, 100.0f, true);
        if (active_tool == 0) {
            tostf::gl::set_clear_color(0.43f, 0.43f, 0.43f, 1);
            window.set_minimum_size({ 800, 800 });
            tostf::log_section("Loading rendering shaders");
            const tostf::V_F_shader id_rendering("../../shader/rendering/id.vert",
                                                 "../../shader/rendering/id.frag");
            const tostf::V_G_F_shader wireframe("../../shader/mesh_visualisation/solid_wireframe.vert",
                                                "../../shader/mesh_visualisation/solid_wireframe.geom",
                                                "../../shader/mesh_visualisation/solid_wireframe.frag");
            const tostf::V_G_F_shader selected_wireframe("../../shader/mesh_visualisation/selectable_wireframe.vert",
                                                         "../../shader/mesh_visualisation/selectable_wireframe.geom",
                                                         "../../shader/mesh_visualisation/selectable_wireframe.frag");
            const tostf::V_F_shader instanced_debug("../../shader/mvp_instanced.vert",
                                                    "../../shader/rendering/minimal_instanced.frag");
            const tostf::V_G_F_shader diffuse_vessel("../../shader/aneurysm/diffuse_vessel.vert",
                                                     "../../shader/aneurysm/diffuse_vessel.geom",
                                                     "../../shader/aneurysm/diffuse_vessel.frag",
                                                     {{"DIFFUSE_LIGHTING", 1}, {"COLOR_BUFFER", 2}});
            const tostf::V_F_shader dist_to_ref("../../shader/mesh_visualisation/dist_to_ref.vert",
                                                "../../shader/mesh_visualisation/dist_to_ref.frag",
                                                {{"DIFFUSE_LIGHTING", 1}, {"DIST_BUFFER", 3}});
            diffuse_vessel.update_uniform("model", glm::mat4(1.0f));
            diffuse_vessel.update_uniform("anuerysm_color", glm::vec4(1.0f));
            const tostf::Diffuse_settings diffuse_settings{
                {0.1f, 0.1f, 0.1f, 1.0f}, {10.0f, 1.0f, 10.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}
            };
            tostf::SSBO diffuse_ssbo(1, diffuse_settings);
            tostf::Structured_mesh aneurysm{};
            tostf::Aneurysm_renderer renderer;
            std::unique_ptr<tostf::SSBO> highlight_ssbo;
            std::unique_ptr<tostf::SSBO> color_ssbo;
            std::unique_ptr<tostf::SSBO> dist_ssbo;
            const tostf::Wireframe_settings wireframe_settings;
            Proprocessing_settings settings;
            Step_results results;
            tostf::foam::OpenFOAM_export_info export_info;
            Imgui_window_settings imgui_settings;
            tostf::Texture2D dist_colormap("../../resources/colormaps/bwr.png");
            dist_to_ref.update_uniform("colormap", dist_colormap.get_tex_handle());
            imgui_settings.sidebar_width = 350.0f;
            struct Inlet_selector {
                int plane_count = 2;
                std::optional<tostf::Manual_inlet_selection> current_selection;
            } inlet_selector;
            Deformation_settings deform_settings;
            id_rendering.update_uniform("model", glm::mat4(1.0f));
            selected_wireframe.update_uniform("model", glm::mat4(1.0f));
            selected_wireframe.update_uniform("line_width", wireframe_settings.line_width);
            selected_wireframe.update_uniform("wireframe_color", wireframe_settings.color);
            selected_wireframe.update_uniform("select_wireframe_color", wireframe_settings.color_selected);
            selected_wireframe.update_uniform("win_res", glm::vec2(window.get_resolution()));
            wireframe.update_uniform("model", glm::mat4(1.0f));
            wireframe.update_uniform("line_width", wireframe_settings.line_width);
            wireframe.update_uniform("wireframe_color", wireframe_settings.color);
            wireframe.update_uniform("win_res", glm::vec2(window.get_resolution()));
            std::unique_ptr<tostf::Aneurysm_viewer> aneurysm_viewer;
            const auto popup_display = [&settings, &results]() {
                if (settings.popup == popups::wrong_mesh_path) {
                    if (ImGui::BeginPopupModal("Error")) {
                        ImGui::Text("Path to mesh is wrong.");
                        if (ImGui::Button("OK")) {
                            ImGui::CloseCurrentPopup();
                            settings.popup = popups::none;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::OpenPopup("Error");
                }
                else if (settings.popup == popups::no_mesh) {
                    if (ImGui::BeginPopupModal("Error")) {
                        ImGui::Text("No mesh in file at path: %ls", results.mesh_info.path.c_str());
                        if (ImGui::Button("OK")) {
                            ImGui::CloseCurrentPopup();
                            settings.popup = popups::none;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::OpenPopup("Error");
                }
                else if (settings.popup == popups::export_path_issue) {
                    if (ImGui::BeginPopupModal("Error")) {
                        ImGui::Text("Missing or wrong export path.");
                        if (ImGui::Button("OK")) {
                            ImGui::CloseCurrentPopup();
                            settings.popup = popups::none;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::OpenPopup("Error");
                }
                else if (settings.popup == popups::no_inlet_selected) {
                    if (ImGui::BeginPopupModal("Error")) {
                        ImGui::Text("An inlet has to be selected.");
                        if (ImGui::Button("OK")) {
                            ImGui::CloseCurrentPopup();
                            settings.popup = popups::none;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::OpenPopup("Error");
                }
                else if (settings.popup == popups::export_success) {
                    if (ImGui::BeginPopupModal("Success")) {
                        ImGui::Text("OpenFOAM cases successfully exported.");
                        if (ImGui::Button("OK")) {
                            ImGui::CloseCurrentPopup();
                            settings.popup = popups::none;
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::OpenPopup("Success");
                }
            };

            // Step 1: Open mesh
            bool tested_holes = false;
            const auto open_mesh = [&settings, &results, &cam, &aneurysm, &renderer, &tested_holes,
                    &highlight_ssbo, &deform_settings, &aneurysm_viewer, &color_ssbo, &dist_ssbo]() {

                tested_holes = false;
                const auto mesh_path = tostf::show_open_file_dialog("Open Mesh", "../../resources/meshes/",
                                                                    {"*.obj", "*.off", "*.stl", "*.ply"});
                if (mesh_path) {
                    if (!exists(mesh_path.value())) {
                        settings.popup = popups::wrong_mesh_path;
                    }
                    else {
                        results.mesh_info.path = mesh_path.value();
                        tostf::log_section("Mesh loading");
                        tostf::Event_profiler<std::chrono::milliseconds> mesh_loading_profiler;
                        auto loaded_mesh = tostf::load_single_mesh(mesh_path.value(), 0, true,
                                                                   aiProcess_JoinIdenticalVertices | aiProcess_FindDegenerates |
                                                                   aiProcess_FixInfacingNormals | aiProcess_PreTransformVertices);
                        if (!loaded_mesh) {
                            settings.popup = popups::no_mesh;
                        }
                        else {
                            settings.reset();
                            results.split_mesh.reset();
                            results.deformed.clear();
                            deform_settings.was_run = false;
                            auto mesh = loaded_mesh.value();
                            log_event_timing("Loading mesh", mesh_loading_profiler.lap());
                            mesh->join_vertices();
                            log_event_timing("Joining vertices", mesh_loading_profiler.lap());
                            aneurysm.init(mesh);
                            log_event_timing("Initializing data structures", mesh_loading_profiler.lap());
                            aneurysm.mesh->normals = aneurysm.halfedge->calculate_normals();
                            log_event_timing("Calculating normals", mesh_loading_profiler.lap());
                            aneurysm_viewer = std::make_unique<tostf::Aneurysm_viewer>(aneurysm.mesh);
                            results.split_mesh.color_data = std::vector<glm::vec4>(
                                aneurysm.mesh->vertices.size(), glm::vec4(1.0f));
                            color_ssbo = std::make_unique<tostf::SSBO>(2);
                            color_ssbo->set_data(results.split_mesh.color_data);
                            mesh_loading_profiler.pause();
                            cam->set_radius(length(aneurysm.mesh->get_bounding_box().max) * 1.5f);
                            mesh_loading_profiler.resume();
                            results.mesh_info.stats = aneurysm.tri_face_mesh->calculate_mesh_stats();
                            results.mesh_info.bounding_box = aneurysm.mesh->get_bounding_box();
                            log_event_timing("Calculate mesh stats", mesh_loading_profiler.lap());
                            renderer.vao = std::make_unique<tostf::VAO>();
                            renderer.update_vbos(create_vbos_from_geometry(aneurysm.mesh));
                            renderer.update_ebo(create_ebo_from_geometry(aneurysm.mesh));
                            highlight_ssbo = std::make_unique<tostf::SSBO>(0);
                            const std::vector<float> highlight_data(aneurysm.mesh->vertices.size(), 0.0f);
                            highlight_ssbo->set_data(highlight_data);
                            dist_ssbo = std::make_unique<tostf::SSBO>(3);
                            dist_ssbo->initialize_empty_storage(aneurysm.mesh->vertices.size() * sizeof(float));
                            settings.finish_step();
                        }
                    }
                }
            };

            const auto step_display = [&export_info, &settings, &open_mesh, &window, &imgui_settings, &renderer, &
                    aneurysm_viewer, &cam, &active_tool,
                    &deform_settings, &results]() {
                const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse
                                          | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
                const auto style = ImGui::GetStyle();
                auto offset = imgui_settings.sidebar_width;
                if (settings.step == preprocessing_steps::export_case || settings.step == preprocessing_steps::start_simulation) {
                    offset = 0;
                }
                const auto status_window_size = (style.FramePadding.y * 4.0f + style.ItemSpacing.x * 4.0f) * settings
                                                                                                             .step_status.
                                                                                                             size();
                ImGui::SetNextWindowPos(ImVec2((window.get_resolution().x - offset) / 2.0f,
                                               window.get_resolution().y - imgui_settings.panel_height), 0, ImVec2(0.5f, 1.0f));
                ImGui::SetNextWindowSize(ImVec2(status_window_size, 40.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.75f);
                const auto deform_active = deform_settings.future.valid() && !tostf::is_future_ready(deform_settings.future);
                if (ImGui::Begin("##STEPRADIOBUTTONS", nullptr, window_flags
                                                                | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration)
                ) {
                    if (deform_active) {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }
                    for (unsigned s_i = 0; s_i < settings.step_status.size(); ++s_i) {
                        ImGui::SameLine();
                        std::string status_label("##stepstatus" + std::to_string(s_i));
                        const auto step_status = settings.step_status.at(s_i);
                        if (step_status) {
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                        }
                        else if (s_i > 0 && settings.step_status.at(s_i - 1) || s_i == 0) {
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
                        }
                        else {
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.95f, 0.12f, 0.12f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.6f, 0.17f, 0.17f, 1.0f));
                        }
                        const auto inactive_step = settings.step != static_cast<preprocessing_steps>(s_i);
                        if (inactive_step) {
                            const auto curr_col = ImGui::GetColorU32(ImGuiCol_FrameBg);
                            ImGui::PopStyleColor(2);
                            ImGui::PushStyleColor(ImGuiCol_CheckMark, curr_col);
                        }
                        if (ImGui::RadioButton(status_label.c_str(), inactive_step)) {
                            settings.go_to_step(s_i);
                            cam->set_center(glm::vec3(0.0f));
                            if (settings.step == preprocessing_steps::find_inlets) {
                                aneurysm_viewer->select_all();
                                aneurysm_viewer->update_renderer(renderer);
                            }
                        }
                        if (!inactive_step) {
                            ImGui::PopStyleColor();
                        }
                        ImGui::PopStyleColor();
                    }
                    if (deform_active) {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }
                }
                ImGui::PopStyleVar();
                ImGui::End();
                ImGui::SetNextWindowPos(ImVec2(0.0f, window.get_resolution().y), 0, ImVec2(0.0f, 1.0f));
                ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x, imgui_settings.panel_height));
                std::string title = "Preprocessing: Step " + std::to_string(static_cast<int>(settings.step) + 1)
                                    + " / " + std::to_string(static_cast<int>(preprocessing_steps::count));
                const auto current_step = settings.step;
                ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
                if (ImGui::Begin(title.c_str(), nullptr, window_flags)) {
                    ImGui::Columns(3, "##stepping", false);
                    ImGui::SetColumnWidth(0, window.get_resolution().x / 5.0f);
                    ImGui::SetColumnWidth(1, window.get_resolution().x / 5.0f * 3.0f);
                    ImGui::SetColumnWidth(2, window.get_resolution().x / 5.0f);
                    ImGui::Dummy(ImVec2(0.0f, 25.0f));
                    if (current_step == preprocessing_steps::open_mesh || deform_active) {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }
                    auto spacing_left = window.get_resolution().x / 10.0f - (100.0f + style.FramePadding.x * 2.0f) / 2.0f;
                    ImGui::Dummy(ImVec2(spacing_left, 0.0f));
                    ImGui::SameLine();
                    if (ImGui::Button("Back", ImVec2(100.0f, 100.0f))) {
                        settings.back();
                        cam->set_center(glm::vec3(0.0f));
                        if (settings.step == preprocessing_steps::find_inlets) {
                            aneurysm_viewer->select_all();
                            aneurysm_viewer->update_renderer(renderer);
                        }
                    }
                    if (current_step == preprocessing_steps::open_mesh || deform_active) {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }
                    ImGui::NextColumn();
                    if (current_step == preprocessing_steps::open_mesh) {
                        ImGui::TextWrapped(
                            "This tool is used to preprocess any 3D vessel mesh for a CFD robustness evaluation. "
                            "First you have to load the mesh you want to run the CFD simulation on. Then both inlets and outlets have to be selected. "
                            "Afterwards you can specify deformation settings and how many different deformed meshes you want to create. "
                            "Finally you can export runable OpenFOAM cases for every deformation.");
                        if (ImGui::Button("Please open a vessel mesh")) {
                            open_mesh();
                        }
                        ImGui::SameLine();
                        ImGui::Text(" or ");
                        ImGui::SameLine();
                        if (ImGui::Button("Go to visualization")) {
                            active_tool = 1;
                        }
                    }
                    else if (current_step == preprocessing_steps::find_inlets) {
                        ImGui::TextWrapped("This tool provides two ways to select inlets and outlets in the 3D vessel mesh. "
                            "Either you select the inlets and outlets manually by clicking on them, or you specify the number "
                            "of in inlets and outlets in the mesh and they will be detected automatically."
                            "Automatic detection might not work every time, so a rerun might be necessary.");
                        if (!settings.inlet_manual_mode && ImGui::Button("Activate manual mode")) {
                            settings.inlet_manual_mode = true;
                        }
                        if (settings.inlet_manual_mode && ImGui::Button("Activate automatic mode")) {
                            settings.inlet_manual_mode = false;
                        }
                    }
                    else if (current_step == preprocessing_steps::create_deformed) {
                        ImGui::TextWrapped("This tool enables you to create multiple deformations of the 3D vessel mesh. "
                            "The extends of the governing formulae of the deformation can be specified in the settings window. "
                            "These deformations are intended to be used for the evaluation of the robustness of the CFD simulation.");
                    }
                    else if (current_step == preprocessing_steps::select_aneurysm) {
                        ImGui::TextWrapped(
                            "Here you should select 3 points along the aneurysm ostium and 2 points on the dome of the aneurysm. "
                            "These enable a 2D parametrization of the aneurysm which is useful for the visualization of scalar values "
                            "on the surface of the anuerysm.");
                    }
                    ImGui::NextColumn();
                    spacing_left = window.get_resolution().x / 10.0f - (100.0f + style.FramePadding.x * 2.0f) / 2.0f;
                    ImGui::Dummy(ImVec2(0.0f, 25.0f));
                    ImGui::Dummy(ImVec2(spacing_left, 0.0f));
                    ImGui::SameLine();

                    bool skip_available = false;
                    std::string proceed_str("Proceed");
                    if (current_step == preprocessing_steps::create_deformed && results.deformed.empty()) {
                        proceed_str = "Skip";
                        skip_available = true;
                    }
                    if ((!settings.is_step_finished(current_step) || deform_active) && !skip_available) {
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                    }
                    if (current_step == preprocessing_steps::export_case) {
                        proceed_str = "Export case";
                        if (!settings.is_step_finished(current_step)) {
                            ImGui::PopStyleVar();
                            ImGui::PopItemFlag();
                        }
                    }
                    if (ImGui::Button(proceed_str.c_str(), ImVec2(100.0f, 100.0f))) {
                        if (current_step == preprocessing_steps::find_inlets) {
                            aneurysm_viewer->select_all();
                            aneurysm_viewer->update_renderer(renderer);
                        }
                        else if (current_step == preprocessing_steps::export_case) {
                            export_info.execute_export = true;
                        }
                        cam->set_center(glm::vec3(0.0f));
                        if (skip_available) {
                            settings.force_proceed();
                        }
                        else {
                            settings.proceed();
                        }
                    }
                    if (current_step != preprocessing_steps::export_case && !skip_available
                        && (!settings.is_step_finished(current_step) || deform_active)) {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleVar();
                    }
                }
                ImGui::End();
                ImGui::PopStyleVar();
            };

            tostf::Legend dist_legend;
            dist_legend.set_height(256.0f);

            const auto deform_points = [&aneurysm, &deform_settings, &results, &settings, &dist_legend]() {
                bool finished = false;
                while (!finished) {
                    if (deform_settings.was_run) {
                        break;
                    }
                    finished = true;
                    int offset = 0;
                    if (deform_settings.append_deforms) {
                        offset = glm::max(0, static_cast<int>(results.deformed.size() - deform_settings.deformation_count));
                    }
        #pragma omp parallel for
                    for (int c_id = 0; c_id < static_cast<int>(deform_settings.controls.size()); ++c_id) {
                        if (deform_settings.controls.at(c_id).current < deform_settings.controls.at(c_id).iterations) {
                            auto random_point = tostf::gen_random_vec(aneurysm.mesh->get_bounding_box().min * 1.5f,
                                                                            aneurysm.mesh->get_bounding_box().max * 1.5f);
					        auto random_sign = glm::sign(glm::sign(tostf::gen_random_float_value<float>(-1.0f, 1.0f) + 0.5f));
					        for (int id = 0; id < static_cast<int>(results.deformed.at(c_id + offset).vertices.size()); id++) {
						        const auto vert = results.deformed.at(c_id + offset).vertices.at(id);
						        float dist_to_inlet_weight = 1.0f;
						        if (deform_settings.preserve_inlets) {
							        for (const auto& inlet : results.split_mesh.inlets) {
								        dist_to_inlet_weight = glm::min(dist_to_inlet_weight,
									        glm::max(glm::max(
										        abs(length(vert - inlet.center)) -
										        inlet.radius, 0.0f) / inlet.radius - 0.5f, 0.0f));
							        }
						        }
						        const auto dist_vert = distance(random_point, vert);
						        const auto gauss_rayleigh =
							        deform_settings.controls.at(c_id).scale * tostf::math::calc_gauss_rayleigh(
								        dist_vert, deform_settings.controls.at(c_id).gauss_variance,
								        deform_settings.controls.at(c_id).rayleigh_variance,
								        deform_settings.controls.at(c_id).rayleigh_shift);
						        results.deformed.at(c_id + offset).vertices.at(id) = vert
							        + (vert - random_point) * random_sign *
							        gauss_rayleigh * dist_to_inlet_weight;
					        }
                            deform_settings.controls.at(c_id).current++;
                        }
                        finished = finished && deform_settings.controls.at(c_id).current == deform_settings
                                                                                            .controls.at(c_id).iterations;
                    }
                }
                for (auto& c : deform_settings.controls) {
                    c.current = 0;
                }
                for (auto& d : results.deformed) {
                    d.distances_to_reference.resize(aneurysm.mesh->vertices.size());
                }
                results.min_dist_to_ref = FLT_MAX;
                results.max_dist_to_ref = -FLT_MAX;
                for (int deform_id = 0; deform_id < static_cast<int>(results.deformed.size()); ++deform_id) {
                    auto& deformed_verts = results.deformed.at(deform_id).vertices;
        #pragma omp parallel for
                    for (int p_id = 0; p_id < static_cast<int>(deformed_verts.size()); ++p_id) {
                        const auto dir = results.deformed.at(deform_id).vertices.at(p_id) - aneurysm.mesh->vertices.at(p_id);
                        const auto dist = length(dir);
                        const auto sign = glm::sign(dot(dir / dist, aneurysm.mesh->normals.at(p_id)));
                        results.deformed.at(deform_id).distances_to_reference.at(p_id) = dist * sign;
                    }

                    const auto min_max = std::minmax_element(std::execution::par,
                                                             results.deformed.at(deform_id).distances_to_reference.begin(),
                                                             results.deformed.at(deform_id).distances_to_reference.end());
                    results.deformed.at(deform_id).min_dist_to_ref = *min_max.first;
                    results.deformed.at(deform_id).max_dist_to_ref = *min_max.second;
                    results.min_dist_to_ref = glm::min(results.min_dist_to_ref, *min_max.first);
                    results.max_dist_to_ref = glm::max(results.max_dist_to_ref, *min_max.second);
                    const auto avg_dist = std::reduce(std::execution::par,
                                                      results.deformed.at(deform_id).distances_to_reference.begin(),
                                                      results.deformed.at(deform_id).distances_to_reference.end(), 0.0f);
                    results.deformed.at(deform_id).avg_dist_to_ref =
                        avg_dist / static_cast<float>(results.deformed.at(deform_id).vertices.size());
                    results.deformed.at(deform_id).sd_dist_to_ref = glm::sqrt(tostf::math::variance_seq(
                        results.deformed.at(deform_id).distances_to_reference, results.deformed.at(deform_id).avg_dist_to_ref));

                }
                results.min_dist_to_ref = glm::min(results.min_dist_to_ref, -results.max_dist_to_ref);
                results.max_dist_to_ref = glm::max(-results.min_dist_to_ref, results.max_dist_to_ref);
                dist_legend.init(results.min_dist_to_ref, results.max_dist_to_ref);
                deform_settings.was_run = true;
                settings.finish_step(preprocessing_steps::create_deformed);
            };

            const auto display_legend = [&]() {
                const auto legend_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
                                                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                                 ImGuiWindowFlags_NoResize;
                const auto frame_padding = ImGui::GetStyle().FramePadding;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                const auto font_size = ImGui::GetFontSize();
                ImGui::SetNextWindowSize(ImVec2(150.0f, 256.0f + font_size));
                ImGui::SetNextWindowPos(ImVec2(50.0f, window.get_resolution().y - imgui_settings.panel_height - 300.0f));
                if (ImGui::Begin("Legend", nullptr, legend_window_flags)) {
                    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(35, 35, 35, 255));
                    const auto legend_top_left = ImGui::GetCursorScreenPos();
                    const auto legend_x = legend_top_left.x;
                    auto legend_y = legend_top_left.y;
                    legend_y += font_size / 2.0f;
                    ImGui::SetCursorScreenPos(ImVec2(legend_x, legend_y));
                    ImGui::RotatedImage(ImGui::ConvertGLuintToImTex(dist_colormap.get_name()),
                                        ImVec2(20.0f, 256.0f));
                    for (auto& s : dist_legend.steps) {
                        const auto height_offset = 256.0f - dist_legend.value_to_height(s);
                        ImGui::SetCursorScreenPos(ImVec2(
                            legend_top_left.x + 30.0f + 2.0f * frame_padding.x,
                            legend_y + height_offset - font_size / 2.0f - 2.0f));
                        ImGui::Text("%s", tostf::float_to_scientific_str(s).c_str());
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(legend_x, legend_y + height_offset),
                                                            ImVec2(legend_x + 30.0f,
                                                                   legend_y + height_offset),
                                                            IM_COL32(35, 35, 35, 255), 2.0f);
                    }
                    ImGui::PopStyleColor();
                }
                ImGui::End();
                ImGui::PopStyleVar(5);
            };


            const auto sidebar = [&settings, &results, &inlet_selector, &deform_settings, &imgui_settings, &ms_profiler,
                    &window, &aneurysm_viewer, &aneurysm, &highlight_ssbo, &color_ssbo, &cam, &deform_points, &renderer]() {
                const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse
                                          | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
                if (settings.step == preprocessing_steps::export_case || settings.step == preprocessing_steps::start_simulation) {
                    return;
                }
                const auto style = ImGui::GetStyle();
                ImGui::SetNextWindowPos(ImVec2(window.get_resolution().x, 0.0f), 0, ImVec2(1.0f, 0.0f));
                ImGui::SetNextWindowSize(ImVec2(imgui_settings.sidebar_width,
                                                window.get_resolution().y - imgui_settings.panel_height));
                if (ImGui::Begin(step_to_sidebar_title(settings.step).c_str(), nullptr, window_flags)) {
                    // Sidebar mesh_info
                    if (settings.step == preprocessing_steps::open_mesh && settings.is_current_step_finished()) {
                        ImGui::TextWrapped("Please change the scale according to mesh size.");
                        auto pos = ImGui::GetCursorPos();
                        pos.y += style.FramePadding.y;
                        ImGui::SetCursorPos(pos);
                        ImGui::Text("1 mesh unit equals ");
                        ImGui::SameLine();
                        pos = ImGui::GetCursorPos();
                        pos.y -= style.FramePadding.y;
                        ImGui::SetCursorPos(pos);
                        ImGui::PushItemWidth(style.FramePadding.x * 2.0f * 12.0f);
                        ImGui::Combo("##meshunitscale", reinterpret_cast<int*>(&results.mesh_info.scale),
                                     "km\0m\0dm\0cm\0mm\0\xC2\xB5m\0nm\0\0");
                        ImGui::PopItemWidth();
                        if (ImGui::Button("Flip normals")) {
                            for (auto& n : aneurysm.mesh->normals) {
                                n = -n;
                            }
                            for (size_t i = 0; i < aneurysm.mesh->indices.size() / 3; ++i) {
                                auto temp = aneurysm.mesh->indices.at(i * 3);
                                aneurysm.mesh->indices.at(i * 3) = aneurysm.mesh->indices.at(i * 3 + 2);
                                aneurysm.mesh->indices.at(i * 3 + 2) = temp;
                            }
                            aneurysm.halfedge = std::make_unique<tostf::Halfedge_mesh>(aneurysm.mesh);
                            renderer.vbos.at(1).vbo->set_data(aneurysm.mesh->normals);
                        }
                        ImGui::Separator();
                        ImGui::Text("Triangle area");
                        const auto mesh_scale = results.mesh_info.get_scale();
                        float scaled_mesh_info_value = mesh_scale * results.mesh_info.stats.area.avg;
                        ImGui::InputFloat("average##area", &scaled_mesh_info_value, 0, 0, "%.7fm\xC2\xB2",
                                          ImGuiInputTextFlags_ReadOnly);
                        scaled_mesh_info_value = mesh_scale * results.mesh_info.stats.area.min;
                        ImGui::InputFloat("min##area", &scaled_mesh_info_value, 0, 0, "%.7fm\xC2\xB2",
                                          ImGuiInputTextFlags_ReadOnly);
                        scaled_mesh_info_value = mesh_scale * results.mesh_info.stats.area.max;
                        ImGui::InputFloat("max##area", &scaled_mesh_info_value, 0, 0, "%.7fm\xC2\xB2",
                                          ImGuiInputTextFlags_ReadOnly);
                        ImGui::Separator();
                        ImGui::Text("Edge length");
                        scaled_mesh_info_value = mesh_scale * results.mesh_info.stats.edge_length.avg;
                        ImGui::InputFloat("average##edge", &scaled_mesh_info_value, 0, 0, "%.7fm",
                                          ImGuiInputTextFlags_ReadOnly);
                        scaled_mesh_info_value = mesh_scale * results.mesh_info.stats.edge_length.min;
                        ImGui::InputFloat("min##edge", &scaled_mesh_info_value, 0, 0, "%.7fm", ImGuiInputTextFlags_ReadOnly);
                        scaled_mesh_info_value = mesh_scale * results.mesh_info.stats.edge_length.max;
                        ImGui::InputFloat("max##edge", &scaled_mesh_info_value, 0, 0, "%.7fm", ImGuiInputTextFlags_ReadOnly);
                        ImGui::Separator();
                        ImGui::Text("Volume");
                        scaled_mesh_info_value = mesh_scale * mesh_scale * mesh_scale * results.mesh_info.stats.volume;
                        ImGui::InputFloat("##volume", &scaled_mesh_info_value, 0, 0, "%.7fm\xC2\xB3",
                                          ImGuiInputTextFlags_ReadOnly);
                        ImGui::Separator();
                        ImGui::Text("Bounding box");
                        ImGui::PushItemWidth(-50.0f);
                        auto scaled_bb = results.mesh_info.bounding_box.min * mesh_scale;
                        ImGui::InputFloat3("min##boundingbox", value_ptr(scaled_bb), "%.5fm");
                        scaled_bb = results.mesh_info.bounding_box.max * mesh_scale;
                        ImGui::InputFloat3("min##boundingbox", value_ptr(scaled_bb), "%.5fm");
                        ImGui::PopItemWidth();
                    }
                    // Sidebar find_inlets
                    if (settings.step == preprocessing_steps::find_inlets) {
                        if (!settings.inlet_manual_mode) {
                            ImGui::Text("Inlet / outlet count");
                            if (ImGui::InputInt("##Inlet / outlet count", &inlet_selector.plane_count)) {
                                inlet_selector.plane_count = glm::max(1, inlet_selector.plane_count);
                            }
                            if (ImGui::Button("Run automatic detection")) {
                                aneurysm.reset_tri_face_mesh();
                                aneurysm_viewer->reset_views();
                                ms_profiler.start();
                                std::fill(results.split_mesh.color_data.begin(), results.split_mesh.color_data.end(),
                                          results.split_mesh.aneurysm_color);
                                results.split_mesh.set_inlets(
                                    detect_inlets_automatically(aneurysm, inlet_selector.plane_count));
                                for (auto& v : results.split_mesh.inlets) {
                                    aneurysm_viewer->add_inlet(v);
                                    aneurysm.tri_face_mesh->remove_faces(v.indices);
                                }
                                color_ssbo->set_data(results.split_mesh.color_data);
                                aneurysm_viewer->update_aneurysm_indices(aneurysm.tri_face_mesh->get_indices());
                                log_event_timing("automatic detection", ms_profiler.lap());
                                settings.finish_step();
                            }
                        }
                        else {
                            ImGui::TextWrapped(
                                "To select an inlet / outlet please press X on any vertex on the inlet / outlet.");
                            if (inlet_selector.current_selection) {
                                if (ImGui::Button("Confirm current selection")) {
                                    inlet_selector.current_selection->selection.name =
                                        "inlet_" + std::to_string(aneurysm_viewer->inlet_views.size());
                                    aneurysm_viewer->add_inlet(inlet_selector.current_selection->selection);
                                    results.split_mesh.add_inlet(inlet_selector.current_selection->selection);
                                    color_ssbo->set_data(results.split_mesh.color_data);
                                    aneurysm.tri_face_mesh->remove_faces(inlet_selector.current_selection->selection.indices);
                                    aneurysm_viewer->update_aneurysm_indices(aneurysm.tri_face_mesh->get_indices());
                                    const std::vector<float> highlight_data(aneurysm.mesh->vertices.size(), 0.0f);
                                    highlight_ssbo->set_data(highlight_data);
                                }
                            }
                            if (ImGui::Button("Finish selection")) {
                                settings.inlet_manual_mode = false;
                                inlet_selector.current_selection.reset();
                                settings.finish_step();
                                const std::vector<float> highlight_data(aneurysm.mesh->vertices.size(), 0.0f);
                                highlight_ssbo->set_data(highlight_data);
                            }
                            if (ImGui::Button("Cancel manual mode")) {
                                inlet_selector.current_selection.reset();
                                results.split_mesh.reset();
                                aneurysm_viewer->reset_views();
                                const std::vector<float> highlight_data(aneurysm.mesh->vertices.size(), 0);
                                highlight_ssbo->set_data(highlight_data);

                            }
                        }
                    }
                    // Sidebar mesh viewer
                    if (settings.step == preprocessing_steps::find_inlets) {
                        if (!results.split_mesh.inlets.empty()) {
                            ImGui::Text("Select the parts you want to see.");
                            std::string full_eye_status(ICON_MDI_EYE_OFF);
                            if (aneurysm_viewer->full_mesh_visible()) {
                                full_eye_status = ICON_MDI_EYE;
                            }
                            std::string full_mesh_vis_label(full_eye_status + "##vis_full_mesh");
                            if (ImGui::Button(full_mesh_vis_label.c_str())) {
                                aneurysm_viewer->toggle_all();
                            }
                            ImGui::SameLine();
                            if (ImGui::CustomSelectable("Full mesh", results.split_mesh.active == -3)) {
                                results.split_mesh.active = -3;
                                cam->set_center(glm::vec3(0.0f));
                            }
                            std::string inlets_eye_status(ICON_MDI_EYE_OFF);
                            if (aneurysm_viewer->all_inlets_visible()) {
                                inlets_eye_status = ICON_MDI_EYE;
                            }
                            std::string inlets_vis_label(inlets_eye_status + "##vis_all_inlets");
                            if (ImGui::Button(inlets_vis_label.c_str())) {
                                aneurysm_viewer->toggle_all_inlets();
                            }
                            ImGui::SameLine();
                            if (ImGui::CustomSelectable("all inlets", results.split_mesh.active == -2)) {
                                results.split_mesh.active = -2;
                                cam->set_center(glm::vec3(0.0f));
                            }
                            std::string wall_eye_status(ICON_MDI_EYE_OFF);
                            if (aneurysm_viewer->wall_selected) {
                                wall_eye_status = ICON_MDI_EYE;
                            }
                            std::string wall_vis_label(wall_eye_status + "##vis_wall");
                            if (ImGui::Button(wall_vis_label.c_str())) {
                                aneurysm_viewer->toggle_wall();
                            }
                            ImGui::SameLine();
                            if (ImGui::CustomSelectable("wall", results.split_mesh.active == -1)) {
                                results.split_mesh.active = -1;
                                cam->set_center(glm::vec3(0.0f));
                            }
                            for (unsigned i = 0; i < aneurysm_viewer->get_view_count(); i++) {
                                std::string eye_status(ICON_MDI_EYE_OFF);
                                if (aneurysm_viewer->selected_inlets.at(i)) {
                                    eye_status = ICON_MDI_EYE;
                                }
                                std::string vis_label(eye_status + "##vis_inlet" + std::to_string(i));
                                if (ImGui::Button(vis_label.c_str())) {
                                    aneurysm_viewer->toggle_selection(i);
                                }
                                ImGui::SameLine();
                                if (!results.split_mesh.inlets.at(i).renaming
                                    && ImGui::CustomSelectable(results.split_mesh.inlets.at(i).name.data(),
                                                               results.split_mesh.active == static_cast<int>(i), 0,
                                                               ImVec2(ImGui::GetContentRegionAvailWidth() - 80.0f, 0))) {
                                    results.split_mesh.active = i;
                                    cam->rotate_to_vector(-results.split_mesh.inlets.at(i).normal);
                                }
                                if (results.split_mesh.inlets.at(i).renaming) {
                                    std::string input_label("##rename_inlet" + std::to_string(i));
                                    ImGui::PushItemWidth(ImGui::GetContentRegionAvailWidth() - 117.0f);
                                    if (ImGui::InputText(input_label.c_str(), &results.split_mesh.inlets.at(i).name,
                                                         ImGuiInputTextFlags_CharsNoBlank |
                                                         ImGuiInputTextFlags_EnterReturnsTrue)) {
                                        results.split_mesh.inlets.at(i).renaming = false;
                                    }
                                    ImGui::PopItemWidth();
                                    ImGui::SameLine();
                                    std::string color_label("##color" + results.split_mesh.inlets.at(i).name);
                                    if (ImGui::ColorEdit3(color_label.c_str(),
                                                          value_ptr(results.split_mesh.inlets.at(i).color),
                                                          ImGuiColorEditFlags_NoInputs)) {
                                        results.split_mesh.update_color(results.split_mesh.inlets.at(i));
                                        color_ssbo->set_data(results.split_mesh.color_data);
                                    }
                                    ImGui::SameLine();
                                    std::string edit_label(
                                        ICON_MDI_CHECK + std::string("##edit" + results.split_mesh.inlets.at(i).name));
                                    if (ImGui::Button(edit_label.c_str())) {
                                        results.split_mesh.inlets.at(i).renaming = false;
                                    }
                                    ImGui::SameLine();
                                    std::string label(
                                        ICON_MDI_DELETE + std::string("##delete" + results.split_mesh.inlets.at(i).name));
                                    if (ImGui::Button(label.c_str())) {
                                        aneurysm.tri_face_mesh->add_faces(results.split_mesh.inlets.at(i).indices);
                                        results.split_mesh.remove_inlet(i);
                                        aneurysm_viewer->aneurysm_view.indices = aneurysm.tri_face_mesh->get_indices();
                                        aneurysm_viewer->remove_inlet(i);
                                    }
                                }
                                else {
                                    ImGui::SameLine();
                                    std::string color_label("##color" + results.split_mesh.inlets.at(i).name);
                                    if (ImGui::ColorEdit3(color_label.c_str(),
                                                          value_ptr(results.split_mesh.inlets.at(i).color),
                                                          ImGuiColorEditFlags_NoInputs)) {
                                        results.split_mesh.update_color(results.split_mesh.inlets.at(i));
                                        color_ssbo->set_data(results.split_mesh.color_data);
                                    }
                                    ImGui::SameLine();
                                    std::string edit_label(ICON_MDI_PENCIL
                                                           + std::string("##delete" + results.split_mesh.inlets.at(i).name));
                                    if (ImGui::Button(edit_label.c_str())) {
                                        results.split_mesh.inlets.at(i).renaming = true;
                                    }
                                }
                            }
                        }
                    }
                    // Sidebar create_deformed
                    if (settings.step == preprocessing_steps::create_deformed) {
                        const auto deform_active = deform_settings.future.valid() && !tostf::is_future_ready(
                                                       deform_settings.future);
                        if (deform_active) {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                        }
                        bool curve_changed = false;
                        ImGui::PushItemWidth(175.0f);
                        ImGui::Text("General settings:");
                        bool iter_changed = ImGui::DragInt("Number of mesh\nvariations", &deform_settings.deformation_count, 1,
                                                           1, INT_MAX);
                        auto percentage = deform_settings.value_variance_in_percent * 100.0f;
                        iter_changed |= ImGui::DragFloat("Standard deviation\nin percent", &percentage, 1.0f, 0.0f, 100.0f,
                                                         "%.0f%%");
                        if (iter_changed) {
                            deform_settings.deformation_count = glm::max(1, deform_settings.deformation_count);
                            deform_settings.value_variance_in_percent = glm::clamp(percentage, 0.0f, 100.0f) / 100.0f;
                        }
                        ImGui::Checkbox("Preverse inlets", &deform_settings.preserve_inlets);
                        ImGui::Checkbox("Append deformations to list", &deform_settings.append_deforms);
                        ImGui::Separator();
                        ImGui::Text("Deformation settings:");
                        curve_changed |= ImGui::DragInt("Iterations per\nvariation",
                                                        &deform_settings.control_reference.iterations, 100, 1, INT_MAX);
                        if (curve_changed) {
                            deform_settings.control_reference.iterations = glm::max(
                                1, deform_settings.control_reference.iterations);
                        }
                        curve_changed |= ImGui::SliderFloat("Gaussian\nvariance",
                                                            &deform_settings.control_reference.gauss_variance, 0.01f, 50.0f,
                                                            "%.2f");
                        curve_changed |= ImGui::SliderFloat("Rayleigh\nvariance",
                                                            &deform_settings.control_reference.rayleigh_variance, 0.01f, 50.0f,
                                                            "%.2f");
                        curve_changed |= ImGui::DragFloat("Rayleigh shift", &deform_settings.control_reference.rayleigh_shift,
                                                          0.01f,
                                                          0.01f, 100.0f, "%.2f");
                        curve_changed |= ImGui::DragFloat("Scale##deform", &deform_settings.control_reference.scale, 0.01f,
                                                          0.01f, 100.0f, "%.2f");
                        if (curve_changed) {
                            deform_settings.update_curve();
                        }
                        if (iter_changed || curve_changed) {
                            deform_settings.reset();
                        }
                        ImGui::PopItemWidth();
                        ImGui::PlotLines("Gauss\nrayleigh\ndistribution", deform_settings.deform_curve.data(),
                                         static_cast<int>(deform_settings.deform_curve.size()), 0, nullptr, 0, FLT_MAX,
                                         ImVec2(0.0f, 50.0f));
                        if (ImGui::Button("Deform Mesh")) {
                            settings.reset_step();
                            const auto deform_count = static_cast<int>(results.deformed.size());
                            auto end_count = deform_settings.deformation_count;
                            if (deform_settings.append_deforms) {
                                end_count += deform_count;
                            }
                            results.deformed.resize(end_count);
                            //#pragma omp parallel for
                            for (auto& c : deform_settings.controls) {
                                c.current = 0;
                            }
                            for (int mesh_id = 0; mesh_id < end_count; ++mesh_id) {
                                results.deformed.at(mesh_id).name = std::string("deformation_" + std::to_string(mesh_id + 1));
                            }
                            for (int mesh_id = deform_count; mesh_id < end_count; ++mesh_id) {
                                results.deformed.at(mesh_id).vertices = aneurysm.mesh->vertices;
                                results.deformed.at(mesh_id).tex = std::make_unique<tostf::Texture2D>(
                                    tostf::tex_res(imgui_settings.deform_pic_size, imgui_settings.deform_pic_size, 1),
                                    tostf::Texture_definition(tostf::tex::intern::rgba32f));
                            }
                            deform_settings.was_run = false;
                            deform_settings.reset();
                            deform_settings.future = std::async(std::launch::async, deform_points);
                        }
                        if (!results.deformed.empty() && ImGui::Button("Reset deformations")) {
                            settings.reset_step();
                            results.deformed.clear();
                        }
                        if (deform_active) {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                            for (auto& c : deform_settings.controls) {
                                ImGui::ProgressBar(c.progress(), ImVec2(0.0f, 0.0f), c.progress_str().c_str());
                            }
                            if (ImGui::Button("Stop mesh deformation")) {
                                deform_settings.stop();
                                settings.finish_step();
                            }
                        }
                    }
                }
                ImGui::End();
            };

            const auto deform_result_window = [&window, &imgui_settings, &results, &deform_settings]() {
                const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse
                                          | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                                          ImGuiWindowFlags_AlwaysHorizontalScrollbar;
                if (deform_settings.was_run && !results.deformed.empty()) {
                    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), 0, ImVec2(0.0f, 0.0f));
                    ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x - imgui_settings.sidebar_width,
                                                    imgui_settings.deform_bar_height));
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                    if (ImGui::Begin("Deformation gallery", nullptr, window_flags)) {
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.75f);
                        if (deform_settings.active_view == -1) {
                            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            if (ImGui::ImageButton(
                                reinterpret_cast<ImTextureID>(static_cast<intptr_t>(results.reference_picture->get_name())),
                                ImVec2(results.reference_picture->get_res().x + 15.0f, results.reference_picture->get_res().y),
                                ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f), -1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f))) {
                                deform_settings.active_view = -1;
                            }
                            ImGui::PopStyleColor(2);
                        }
                        else {
                            if (ImGui::ImageButton(
                                reinterpret_cast<ImTextureID>(static_cast<intptr_t>(results.reference_picture->get_name())),
                                ImVec2(results.reference_picture->get_res().x + 15.0f, results.reference_picture->get_res().y),
                                ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f))) {
                                deform_settings.active_view = -1;
                            }
                        }
                        int tex_id = 0;
                        int delete_deform_id = -1;
                        for (auto& d : results.deformed) {
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            if (d.tex) {
                                if (deform_settings.active_view == tex_id) {
                                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                                    if (ImGui::ImageButton(
                                        reinterpret_cast<ImTextureID>(static_cast<intptr_t>(d.tex->get_name())),
                                        ImVec2(d.tex->get_res().x + 15.0f, d.tex->get_res().y), ImVec2(0.0f, 1.0f),
                                        ImVec2(1.0f, 0.0f),
                                        -1, ImVec4(1.0f, 1.0f, 1.0f, 1.0f))) {
                                        deform_settings.active_view = tex_id;
                                    }
                                    ImGui::PopStyleColor(2);
                                }
                                else {
                                    if (ImGui::ImageButton(
                                        reinterpret_cast<ImTextureID>(static_cast<intptr_t>(d.tex->get_name())),
                                        ImVec2(d.tex->get_res().x + 15.0f, d.tex->get_res().y), ImVec2(0.0f, 1.0f),
                                        ImVec2(1.0f, 0.0f))) {
                                        deform_settings.active_view = tex_id;
                                    }
                                }
                            }
                            std::string deform_name_edit("##deformname" + std::to_string(tex_id));
                            ImGui::PushItemWidth(imgui_settings.deform_pic_size * 0.8f);
                            ImGui::InputText(deform_name_edit.c_str(), &d.name);
                            ImGui::PopItemWidth();
                            ImGui::SameLine();
                            std::string delete_deform(ICON_MDI_DELETE + std::string("##deformdel" + std::to_string(tex_id)));
                            if (ImGui::Button(delete_deform.c_str())) {
                                delete_deform_id = tex_id;
                                if (deform_settings.active_view == tex_id) {
                                    deform_settings.active_view--;
                                }
                            }
                            ImGui::EndGroup();
                            tex_id++;
                        }
                        if (delete_deform_id > -1) {
                            results.deformed.erase(results.deformed.begin() + delete_deform_id);
                        }
                        ImGui::PopStyleVar();
                    }
                    ImGui::End();
                    ImGui::PopStyleVar();
                }
            };

            const auto openfoam_export_window = [&sim_info, &settings, &results, &export_info, &window, &imgui_settings, &aneurysm,
                    &aneurysm_viewer]() {
                const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus
                                          | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoResize;
                const auto style = ImGui::GetStyle();
                ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
                ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x,
                                                window.get_resolution().y - imgui_settings.panel_height));
                ImGuiIO& io = ImGui::GetIO();
                ImGui::PushFont(io.Fonts->Fonts[1]);
                if (ImGui::Begin("Export OpenFOAM case", nullptr, window_flags)) {
                    const auto win_size = ImGui::GetWindowSize();
                    std::string display_path;
                    if (!export_info.export_path.empty()) {
                        display_path = export_info.export_path.string();
                    }
                    ImGui::InputText("##exportstr", &display_path);
                    const auto input_clicked = ImGui::IsItemClicked();
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MDI_FOLDER_OPEN) || input_clicked && display_path.empty()) {
                        auto folder_path = std::filesystem::current_path();
                        if (!display_path.empty() && std::filesystem::exists(display_path)) {
                            folder_path = display_path;
                        }
                        auto path = tostf::show_select_folder_dialog("Select case folder", folder_path);
                        if (path) {
                            export_info.export_path = path.value();
                        }
                    }
                    ImGui::BeginChild("##exportSettings", ImVec2(0.0f, win_size.y - 120.0f), true);
                    ImGui::Columns(2, "columnalgo", false);
                    if (ImGui::BeginCollapsingSection("Blood flow properties", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                        /*ImGui::Combo("##newtoniancombo", &export_info.algorithm.newtonian, "non-newtonian\0newtonian\0\0");
                        if (export_info.algorithm.newtonian == 0) {
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
                            export_info.algorithm.laminar = 1;
                            export_info.algorithm.steady = 0;
                        }
                        ImGui::Combo("##laminarcombo", &export_info.algorithm.laminar, "turbulent\0laminar\0\0");
                        ImGui::Combo("##steadycombo", &export_info.algorithm.steady, "transient\0steady\0\0");
                        if (export_info.algorithm.newtonian == 0) {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleVar();
                        }*/
                        ImGui::DragFloat("Dynamic viscosity", &export_info.blood_flow.dynamic_viscosity, 0.0001f, 0.0f, FLT_MAX,
                                         "%.5f kg / (m * s)");
                        ImGui::DragFloat("Density", &export_info.blood_flow.density, 1.0f, 0.0f, FLT_MAX,
                                         "%.2f kg / m\xC2\xB3");
                        if (export_info.blood_flow.model == tostf::foam::transport_model::newtonian) {
                            if (export_info.blood_flow.transport_fields.empty()) {
                                export_info.blood_flow.transport_fields = fields_from_model(export_info.blood_flow.model);
                            }
                            export_info.blood_flow.transport_fields.at(0).internal_value =
                                export_info.blood_flow.dynamic_viscosity / export_info.blood_flow.density;
                            ImGui::InputFloat("Kinematic viscosity",
                                              &export_info.blood_flow.transport_fields.at(0).internal_value, 0.0f, 0.0f,
                                              "%.7f m\xC2\xB2 / s",
                                              ImGuiInputTextFlags_ReadOnly);
                        }
				        //ImGui::Checkbox("Steady simulation", &export_info.algorithm.steady);
                        ImGui::EndCollapsingSection();
                    }
                    if (ImGui::BeginCollapsingSection("Control settings", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
				        if (!export_info.algorithm.steady) {
					        ImGui::DragFloat("Initial delta time", &export_info.control_dict.dt, 0.000000001f, 0.000000001f,
						        100.0f, "%.9f s");
					        if (export_info.algorithm.get_application_name() == "pimpleFoam") {
						        ImGui::DragFloat("Maximum delta time", &export_info.control_dict.max_dt, 0.000000001f,
							        export_info.control_dict.dt,
							        100.0f, "%.9f s");
						        ImGui::DragFloat("Maximum Courant number", &export_info.control_dict.max_co, 0.1f, 0.1f,
							        300.0f, "%.1f");
					        }
					        ImGui::DragFloat("Duration", &export_info.control_dict.duration, 0.001f, 0.001f, 10.0f, "%.3f s");
				        }
				        else {
					        ImGui::DragInt("Iterations", &export_info.control_dict.iterations, 1, 1, INT_MAX);
				        }
				        ImGui::Checkbox("Enable parallel execution", &export_info.parallel);
				        if (ImGui::BeginCollapsingSection("Extended settings")) {
					        if (!export_info.algorithm.steady) {
						        ImGui::SliderInt("Time precision", &export_info.control_dict.time_precision, 1, 15);
						        ImGui::DragInt("Cycles", &export_info.control_dict.cycles, 1, 1, INT_MAX);
						        ImGui::DragInt("Overwrite first x cycles", &export_info.control_dict.cycles_to_overwrite, 1, 0,
							        glm::max(export_info.control_dict.cycles, 2));
						        ImGui::DragFloat("Write interval", &export_info.control_dict.write_interval, 0.000000001f,
							        0.000000001f, 100.0f, "%.9f s");
					        }
					        else {
						        ImGui::DragInt("Write interval", &export_info.control_dict.iter_write_interval, 1, 1, export_info.control_dict.iterations);
					        }
					        ImGui::SliderInt("Write precision", &export_info.control_dict.write_precision, 1, 15);
					        ImGui::Combo("Write format", reinterpret_cast<int*>(&export_info.control_dict.format),
						        "binary\0ascii\0\0");
					        ImGui::EndCollapsingSection();
				        }
                        ImGui::EndCollapsingSection();
                    }
                    if (ImGui::BeginCollapsingSection("Postprocessing", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                        ImGui::Checkbox("Lambda2", &export_info.post_processing.lambda2);
                        ImGui::Checkbox("Q criterion", &export_info.post_processing.q_criterion);
                        ImGui::Checkbox("Wall Shear Stress", &export_info.post_processing.wall_shear_stress);
                        ImGui::EndCollapsingSection();
                    }
                    if (ImGui::BeginCollapsingSection("Solver settings")) {
                        if (export_info.algorithm.get_application_name() != "simpleFoam") {
                            ImGui::DragInt("Corrector steps", &export_info.algorithm.n_correctors, 1, 0, INT_MAX);
                            ImGui::DragInt("Outer corrector steps", &export_info.algorithm.n_outer_correctors, 1, 1, INT_MAX);
                        }
                        ImGui::SliderInt("Non ortho corrector steps", &export_info.algorithm.n_non_orthogonal_correctors, 0, 100);
                        if (ImGui::BeginCollapsingSection("Velocity solver settings", nullptr)) {
                            ImGui::InputFloat("Tolerance##velocity", &export_info.solvers.U.tolerance, 0.0f, 1.0f, "%.9f");
                            ImGui::InputFloat("Relative tolerance##velocity", &export_info.solvers.U.relative_tolerance, 0.0f, 1.0f, "%.9f");
                            ImGui::SliderFloat("Relaxation factor##velocity", &export_info.solvers.U.relaxation, 0.0f, 1.0f, "%.2f");
                            ImGui::EndCollapsingSection();
                        }
                        if (ImGui::BeginCollapsingSection("Pressure solver settings", nullptr)) {
                            ImGui::InputFloat("Tolerance##pressure", &export_info.solvers.p.tolerance, 0.0f, 1.0f, "%.9f");
                            ImGui::InputFloat("Relative tolerance##pressure", &export_info.solvers.p.relative_tolerance, 0.0f, 1.0f, "%.9f");
                            ImGui::SliderFloat("Relaxation factor##pressure", &export_info.solvers.p.relaxation, 0.0f, 1.0f, "%.2f");
                            ImGui::EndCollapsingSection();
                        }
                        if (ImGui::BeginCollapsingSection("Residual controls", nullptr)) {
                            ImGui::InputFloat("Residual tolerance for pressure", &export_info.algorithm.tolerance_p, 0.0f, 1.0f, "%.9f");
                            ImGui::InputFloat("Residual tolerance for velocity", &export_info.algorithm.tolerance_U, 0.0f, 1.0f, "%.9f");
                            ImGui::EndCollapsingSection();
                        }
                        ImGui::Checkbox("Momentum predictor", &export_info.algorithm.momentum_predictor);
                        ImGui::Combo("Numerical schemes preset", reinterpret_cast<int*>(&export_info.schemes),
                                     "commercial\0accurate\0stable\0\0");
                        ImGui::EndCollapsingSection();
                    }
                    ImGui::NextColumn();
                    if (ImGui::BeginCollapsingSection("Boundaries", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                        const auto section_size = ImGui::GetItemRectMax().x;
                        if (results.split_mesh.inlets.empty()) {
                            ImGui::Text("You might want to select some inlets first before exporting the case.");
                        }
                        for (unsigned i = 0; i < results.split_mesh.inlets.size(); i++) {
                            ImGui::PushItemWidth(section_size / 5.0f);
                            std::string label("##inlet" + std::to_string(i));
                            ImGui::InputText(label.c_str(), &results.split_mesh.inlets.at(i).name,
                                             ImGuiInputTextFlags_CharsNoBlank);
                            ImGui::PopItemWidth();
                            ImGui::SameLine();
                            ImGui::PushItemWidth(section_size / 5.0f);
                            std::string combo_label(label + "combo");
                            const auto boundary = results.split_mesh.inlets.at(i).boundary;
                            if (ImGui::BeginCombo(combo_label.c_str(), boundary_type_to_str(boundary.type).c_str())) {
                                if (ImGui::Selectable("Wall", boundary.type == tostf::boundary_type::wall)) {
                                    if (boundary.type == tostf::boundary_type::inlet) {
                                        export_info.inlet_specified = false;
                                    }
                                    results.split_mesh.inlets.at(i).set_boundary_type(tostf::boundary_type::wall);
                                }
                                if (!export_info.inlet_specified || boundary.type == tostf::boundary_type::inlet) {
                                    if (ImGui::Selectable("Inlet", boundary.type == tostf::boundary_type::inlet)) {
                                        results.split_mesh.inlets.at(i).set_boundary_type(tostf::boundary_type::inlet);
                                        export_info.inlet_specified = true;
                                    }
                                }
                                if (ImGui::Selectable("Outlet", boundary.type == tostf::boundary_type::outlet)) {
                                    if (boundary.type == tostf::boundary_type::inlet) {
                                        export_info.inlet_specified = false;
                                    }
                                    results.split_mesh.inlets.at(i).set_boundary_type(tostf::boundary_type::outlet);
                                }
                                ImGui::EndCombo();
                            }
                            ImGui::PopItemWidth();
                            if (results.split_mesh.inlets.at(i).boundary.type == tostf::boundary_type::inlet) {
                                std::string inflow_label(results.split_mesh.inlets.at(i).name + " inflow velocity");
                                ImGui::DragFloat(inflow_label.c_str(), &results.split_mesh.inlets.at(i).inflow_velocity,
                                                 0.001, 0, FLT_MAX, "%.3fm/s");
                            }
                        }
                        ImGui::EndCollapsingSection();
                    }
                    if (ImGui::BeginCollapsingSection("Mesh generation", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                        auto pos = ImGui::GetCursorPos();
                        pos.y += style.FramePadding.y;
                        ImGui::SetCursorPos(pos);
                        ImGui::Text("1 mesh unit equals ");
                        ImGui::SameLine();
                        pos = ImGui::GetCursorPos();
                        pos.y -= style.FramePadding.y;
                        ImGui::SetCursorPos(pos);
                        ImGui::PushItemWidth(style.FramePadding.x * 2.0f * 12.0f);
                        ImGui::Combo("##meshunitscale", reinterpret_cast<int*>(&results.mesh_info.scale),
                                     "km\0m\0dm\0cm\0mm\0\xC2\xB5m\0nm\0\0");
                        ImGui::PopItemWidth();
                        ImGui::DragInt("Minimal cell count", &export_info.block_mesh.min_cells, 1, 1, INT_MAX);
                        const auto scaled_bb_min = (results.mesh_info.bounding_box.min - export_info.block_mesh.bb_offset)
                                                   * results.mesh_info.get_scale();
                        const auto scaled_bb_max = (results.mesh_info.bounding_box.max + export_info.block_mesh.bb_offset)
                                                   * results.mesh_info.get_scale();
                        float min_grid_side_length = glm::compMin(glm::vec3(scaled_bb_max - scaled_bb_min));
                        export_info.block_mesh.initial_cell_size = abs(min_grid_side_length / export_info.block_mesh.min_cells);
                        glm::ivec3 grid_size(ceil((scaled_bb_max - scaled_bb_min) / export_info.block_mesh.initial_cell_size));
                        ImGui::InputInt3("Initial grid size", value_ptr(grid_size), ImGuiInputTextFlags_ReadOnly);
                        if (export_info.parallel) {
                            ImGui::DragInt("Maximal subdomain count", &export_info.decompose_par_dict.max_sub_domains, 1, 1, INT_MAX);
                            auto bb_diff = glm::vec3(results.mesh_info.bounding_box.max - results.mesh_info.bounding_box.min);
                            float max_grid_side_length = glm::compMax(bb_diff);
                            auto max_side_length = abs(max_grid_side_length / export_info.decompose_par_dict.max_sub_domains);
                            glm::ivec3 subdomain_sizes(ceil(bb_diff / max_side_length));
                            ImGui::InputInt3("Subdomains per direction", value_ptr(subdomain_sizes), ImGuiInputTextFlags_ReadOnly);
                        }
                        if (ImGui::BeginCollapsingSection("snappyHexMesh controls", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                            if (ImGui::BeginCollapsingSection("castellatedMesh controls")) {
                                ImGui::DragInt("Max local cells",
                                               &export_info.snappy_hex_mesh.castellated.max_local_cells,
                                               100, 0, INT_MAX);
                                ImGui::DragInt("Max global cells",
                                               &export_info.snappy_hex_mesh.castellated.max_global_cells,
                                               100, 0, INT_MAX);
                                ImGui::DragInt("Resolve feature angle",
                                               &export_info.snappy_hex_mesh.castellated.resolve_feature_angle,
                                               1, 0, 180);
                                ImGui::DragInt("Feature-based refinement level",
                                               &export_info.snappy_hex_mesh.castellated.features_level_refinement,
                                               1, 0, INT_MAX);
                                ImGui::DragInt("Minimum surface-based\nrefinement level",
                                               &export_info.snappy_hex_mesh.castellated.refinement_surfaces_min_level,
                                               1, 0, export_info.snappy_hex_mesh.castellated.refinement_surfaces_max_level);
                                ImGui::DragInt("Maximum surface-based\nrefinement level",
                                               &export_info.snappy_hex_mesh.castellated.refinement_surfaces_max_level,
                                               1, export_info.snappy_hex_mesh.castellated.refinement_surfaces_min_level,
                                               INT_MAX);
                                ImGui::EndCollapsingSection();
                            }
                            ImGui::Checkbox("Activate snapping", &export_info.snappy_hex_mesh.snap_active);
                            if (export_info.snappy_hex_mesh.snap_active) {
                                if (ImGui::BeginCollapsingSection("Snap controls")) {
                                    ImGui::DragInt("Smooth patches", &export_info.snappy_hex_mesh.snap.n_smooth_patch,
                                        1, 0, INT_MAX);
                                    ImGui::SliderFloat("Tolerance", &export_info.snappy_hex_mesh.snap.tolerance, 1.0f, 6.0f);
                                    ImGui::DragInt("Solve iterations", &export_info.snappy_hex_mesh.snap.n_solve_iter,
                                        1, 0, INT_MAX);
                                    ImGui::SliderInt("Relax iterations", &export_info.snappy_hex_mesh.snap.n_relax_iter, 1, 10);
                                    ImGui::DragInt("Feature snap iterations",
                                        &export_info.snappy_hex_mesh.snap.n_feature_snap_iter,
                                        1, 0, INT_MAX);
                                    ImGui::EndCollapsingSection();
                                }
                            }
                            ImGui::EndCollapsingSection();
                        }
                        if (ImGui::BeginCollapsingSection("Surface feature extract controls")) {
                            ImGui::DragInt("Included angle", &export_info.feature_extract.included_angle,
                                           1, 0, 180);
                            ImGui::Checkbox("Non manifold edges", &export_info.feature_extract.non_manifold_edges);
                            ImGui::Checkbox("Open edges", &export_info.feature_extract.open_edges);
                            ImGui::EndCollapsingSection();
                        }
                        ImGui::EndCollapsingSection();
                    }
                    ImGui::Columns(1);
                    ImGui::EndChild();
                    if (ImGui::Button("Export case")) {
                        export_info.execute_export = true;
                    }
                    if (export_info.execute_export) {
                        export_info.execute_export = false;
                        if (!export_info.export_path.empty() && exists(export_info.export_path)
                            && export_info.inlet_specified) {

                            settings.finish_step();
                            settings.force_proceed();
                            export_openfoam_cases(results.mesh_info.path.filename().stem().string(),
                                export_info, results.mesh_info.get_scale(),
                                results.mesh_info.bounding_box,
                                results.mesh_info.stats.edge_length.avg,
                                aneurysm.mesh, results.deformed,
                                aneurysm_viewer->aneurysm_view, aneurysm_viewer->inlet_views,
                                results.split_mesh.inlets);
                            settings.popup = popups::export_success;
                            sim_info.dir_path = export_info.export_path;
                            sim_info.case_count = results.deformed.size() + 1;
                            sim_info.postprocess = export_info.post_processing.lambda2 | export_info.post_processing.wall_shear_stress
                                | export_info.post_processing.q_criterion;
                            sim_info.parallel = export_info.parallel;
                        }
                        else {
                            if (export_info.export_path.empty() || !exists(export_info.export_path)) {
                                settings.popup = popups::export_path_issue;
                            }
                            else if (!export_info.inlet_specified) {
                                settings.popup = popups::no_inlet_selected;
                            }
                        }
                    }
                }
                ImGui::End();
                ImGui::PopFont();
            };

            auto simulation_window = [&]() {
                const auto window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar
                    | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoResize;
                const auto style = ImGui::GetStyle();
                ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
                ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x,
                    window.get_resolution().y - imgui_settings.panel_height));
                ImGuiIO& io = ImGui::GetIO();
                ImGui::PushFont(io.Fonts->Fonts[1]);
                if (ImGui::Begin("Simulation", nullptr, window_flags)) {
                    const auto button_size = glm::vec2(200.0f, 100.0f);
                    const auto button_pos = (glm::vec2(window.get_resolution()) - button_size) / 2.0f;
                    ImGui::SetCursorPos(ImVec2(button_pos.x, button_pos.y));
                    if (ImGui::Button("Start simulation", ImVec2(button_size.x, button_size.y))) {
                        results = {};
                        highlight_ssbo.reset();
                        deform_settings = {};
                        aneurysm_viewer.reset();
                        color_ssbo.reset();
                        dist_ssbo.reset();
                        sim_info.started = true;
                        active_tool = 1;
                        sim_info.dir_path = relative(sim_info.dir_path);
                        sim_info.finished = std::async(std::launch::async, [&]() {
                            for (auto& d : std::filesystem::directory_iterator(sim_info.dir_path)) {
                                if (std::filesystem::is_directory(d)) {
                                    std::string cmd = "cd " + d.path().string() + " && bash ./mesh_gen.sh";
                                    tostf::log() << cmd;
                                    tostf::log() << exec(cmd.c_str());
                                    sim_info.mesh_gen_progress++;
                                }
                            }
                            if (sim_info.parallel) {
                                for (auto& d : std::filesystem::directory_iterator(sim_info.dir_path)) {
                                    if (std::filesystem::is_directory(d)) {
                                        std::string cmd = "cd " + d.path().string() + " && bash ./run_simulation_parallel.sh";
                                        tostf::log() << exec(cmd.c_str());
                                        sim_info.sim_progress++;
                                    }
                                }
                            }
                            else {
                                for (auto& d : std::filesystem::directory_iterator(sim_info.dir_path)) {
                                    if (std::filesystem::is_directory(d)) {
                                        std::string cmd = "cd " + d.path().string() + " && bash ./run_simulation.sh";
                                        tostf::log() << exec(cmd.c_str());
                                        sim_info.sim_progress++;
                                    }
                                }
                            }
                            if (sim_info.postprocess) {
                                std::string cmd = "cd " + sim_info.dir_path.string() + " && bash ./post_process.sh";
                            }
                            return true;
                        });
                    }
                }
                ImGui::End();
                ImGui::PopFont();
            };

            tostf::View aneurysm_view({0, imgui_settings.panel_height},
                                      win_size - glm::ivec2(imgui_settings.sidebar_width, imgui_settings.panel_height));
            tostf::Texture2D id_tex({aneurysm_view.get_size(), 1}, {tostf::tex::intern::r32i});
            tostf::FBO click_fbo({aneurysm_view.get_size(), 1}, {id_tex.get_fbo_tex_def()},
                                 {{tostf::tex::intern::depth32f, false, 0}});
            tostf::View deform_view({0, 0}, {imgui_settings.deform_pic_size, imgui_settings.deform_pic_size});
            tostf::FBO deform_fbo({deform_view.get_size(), 1}, {}, {{tostf::tex::intern::depth32f, false, 0}});
            results.reference_picture = std::make_unique<tostf::Texture2D>(
                tostf::tex_res(imgui_settings.deform_pic_size, imgui_settings.deform_pic_size, 1));

            const auto resize_views = [&main_view, &aneurysm_view, &window, &imgui_settings](
                tostf::win_mgr::raw_win_ptr, int, int) {
                const auto win_size = window.get_resolution();
                main_view.resize(win_size);
                aneurysm_view.resize(win_size - glm::ivec2(imgui_settings.sidebar_width, imgui_settings.panel_height));
            };
            window.add_resize_fun("resize_views", resize_views);

            // Sphere rendering
            tostf::V_F_shader sphere_renderer("../../shader/mvp_instanced.vert",
                                              "../../shader/rendering/minimal_instanced.frag");
            const float dt = 0.01f;
            while (!window.should_close() && !sim_info.started && active_tool == 0) {
                if (window.is_minimized()) {
                    window.wait_events();
                    continue;
                }
                gui.start_frame();
                sidebar();
                popup_display();
                if (settings.step == preprocessing_steps::create_deformed) {
                    deform_result_window();
                }
                else if (settings.step == preprocessing_steps::find_inlets) {
                    aneurysm_viewer->update_renderer(renderer);
                    if (!tested_holes) {
                        tested_holes = true;
                        const auto detected_inlets = detect_inlets_from_holes(aneurysm);
                        results.split_mesh.color_data = std::vector<glm::vec4>(
                            aneurysm.mesh->vertices.size(), glm::vec4(1.0f));
                        results.split_mesh.set_inlets(detected_inlets);
                        if (!results.split_mesh.inlets.empty()) {
                            aneurysm_viewer->aneurysm_view.indices = aneurysm.tri_face_mesh->get_indices();
                            for (auto& v : results.split_mesh.inlets) {
                                aneurysm_viewer->add_inlet(v);
                            }
                            aneurysm.halfedge = std::make_unique<tostf::Halfedge_mesh>(aneurysm.mesh);
                            aneurysm.mesh->normals = aneurysm.halfedge->calculate_normals();
                            settings.finish_step(preprocessing_steps::find_inlets);
                        }
                        color_ssbo = std::make_unique<tostf::SSBO>(2);
                        color_ssbo->set_data(results.split_mesh.color_data);
                        cam->set_radius(length(aneurysm.mesh->get_bounding_box().max)* 1.5f);
                        results.mesh_info.stats = aneurysm.tri_face_mesh->calculate_mesh_stats();
                        results.mesh_info.bounding_box = aneurysm.mesh->get_bounding_box();
                        renderer.vao = std::make_unique<tostf::VAO>();
                        renderer.update_vbos(create_vbos_from_geometry(aneurysm.mesh));
                        renderer.update_ebo(create_ebo_from_geometry(aneurysm.mesh));
                        highlight_ssbo = std::make_unique<tostf::SSBO>(0);
                        const std::vector<float> highlight_data(aneurysm.mesh->vertices.size(), 0.0f);
                        highlight_ssbo->set_data(highlight_data);
                        dist_ssbo = std::make_unique<tostf::SSBO>(3);
                        dist_ssbo->initialize_empty_storage(aneurysm.mesh->vertices.size() * sizeof(float));
                        settings.finish_step();
                    }
                }
                else if (settings.step == preprocessing_steps::export_case) {
                    openfoam_export_window();
                }
                else if (settings.step == preprocessing_steps::start_simulation) {
                    simulation_window();
                }
                step_display();
                tostf::gl::clear_all();
                aneurysm_view.activate();
                cam->update(window, dt);
                const auto view = cam->get_view();
                const auto proj = cam->get_projection();
                if (settings.inlet_manual_mode && settings.step == preprocessing_steps::find_inlets) {
                    disable(tostf::gl::cap::dither);
                    tostf::gl::set_viewport(0, 0, aneurysm_view.get_size().x, aneurysm_view.get_size().y);
                    click_fbo.bind();
                    tostf::gl::clear_all();
                    id_rendering.update_uniform("view", view);
                    id_rendering.update_uniform("proj", proj);
                    id_rendering.use();
                    renderer.vao->draw_elements(tostf::gl::draw::triangles);
                    tostf::FBO::unbind();
                    enable(tostf::gl::cap::dither);
                    // manual selection
                    if (window.check_key_action(tostf::win_mgr::key::x, tostf::win_mgr::key_action::press)) {
                        auto cursorpos = window.get_cursor_pos();
                        cursorpos.x = cursorpos.x
                                      / static_cast<double>(window.get_resolution().x - imgui_settings.sidebar_width)
                                      * aneurysm_view.get_size().x;
                        cursorpos.y = cursorpos.y
                                      / static_cast<double>(window.get_resolution().y - imgui_settings.panel_height)
                                      * aneurysm_view.get_size().y;
                        cursorpos.y = static_cast<double>(window.get_resolution().y - imgui_settings.panel_height)
                                      - cursorpos.y;
                        if (cursorpos.x > 0 && cursorpos.y > 0) {
                            const auto selected_vertex = static_cast<unsigned>(
                                id_tex.get_pixel_data<int>(static_cast<int>(cursorpos.x), static_cast<int>(cursorpos.y)));
                            if (selected_vertex < aneurysm.mesh->vertices.size()) {
                                inlet_selector.current_selection = select_inlet_manually(aneurysm, selected_vertex);
                                if (inlet_selector.current_selection) {
                                    highlight_ssbo->set_data(inlet_selector.current_selection->highlighted);
                                }
                            }
                        }
                    }
                    aneurysm_view.activate();
                    selected_wireframe.update_uniform("view", view);
                    selected_wireframe.update_uniform("proj", proj);
                    selected_wireframe.update_uniform("win_res", glm::vec2(window.get_resolution()));
                    selected_wireframe.use();
                }
                else {
                    if (settings.step == preprocessing_steps::create_deformed) {
                        std::vector<glm::vec4> midpoints;
                        for (auto& inlet : results.split_mesh.inlets) {
                            midpoints.push_back(inlet.center);
                        }

                        if (deform_settings.was_run) {
                            display_legend();
                            deform_view.activate();
                            cam->resize();
                            const auto proj_d = cam->get_projection();
                            dist_to_ref.use();
                            dist_to_ref.update_uniform("view", view);
                            dist_to_ref.update_uniform("proj", proj_d);
                            dist_to_ref.update_uniform("min_dist", results.min_dist_to_ref);
                            dist_to_ref.update_uniform("max_dist", results.max_dist_to_ref);
                            for (auto& d : results.deformed) {
                                dist_ssbo->set_data(d.distances_to_reference);
                                renderer.vbos.at(0).vbo->set_data(d.vertices);
                                deform_fbo.attach_color_texture(d.tex->get_fbo_tex_def(), 0);
                                deform_fbo.bind();
                                tostf::gl::clear_all();
                                renderer.vao->draw_elements(tostf::gl::draw::triangles);
                                tostf::FBO::unbind();
                            }
                            renderer.vbos.at(0).vbo->set_data(aneurysm.mesh->vertices);
                            diffuse_vessel.update_uniform("view", view);
                            diffuse_vessel.update_uniform("proj", proj_d);
                            diffuse_vessel.use();
                            deform_fbo.attach_color_texture(results.reference_picture->get_fbo_tex_def(), 0);
                            deform_fbo.bind();
                            tostf::gl::clear_all();
                            renderer.vao->draw_elements(tostf::gl::draw::triangles);
                            tostf::FBO::unbind();
                            aneurysm_view.activate();
                            if (deform_settings.active_view != -1
                                && deform_settings.active_view < static_cast<int>(results.deformed.size())) {
                                dist_ssbo->set_data(results.deformed.at(deform_settings.active_view).distances_to_reference);
                                renderer.vbos.at(0).vbo->set_data(results.deformed.at(deform_settings.active_view).vertices);
                            }
                        }
                        if (deform_settings.active_view == -1) {
                            diffuse_vessel.update_uniform("view", view);
                            diffuse_vessel.update_uniform("proj", proj);
                            diffuse_vessel.use();
                        }
                        else {
                            dist_to_ref.update_uniform("view", view);
                            dist_to_ref.update_uniform("proj", proj);
                            dist_to_ref.use();
                        }
                    }
                    else if (static_cast<int>(settings.step) > static_cast<int>(preprocessing_steps::open_mesh)
                             && settings.step_status.at(static_cast<int>(preprocessing_steps::find_inlets))) {
                        diffuse_vessel.update_uniform("view", view);
                        diffuse_vessel.update_uniform("proj", proj);
                        diffuse_vessel.use();
                    }
                    else {
                        wireframe.update_uniform("view", view);
                        wireframe.update_uniform("proj", proj);
                        wireframe.update_uniform("win_res", glm::vec2(window.get_resolution()));
                        wireframe.use();
                    }
                }
                renderer.vao->draw_elements(tostf::gl::draw::triangles);
                main_view.activate();
                gui.render();
                window.swap_buffers();
                gl_debug_logger.retrieve_log();
                window.poll_events();
            }
            deform_settings.was_run = true;
            if (deform_settings.future.valid()) {
                deform_settings.future.wait();
            }
        }
        else {
            window.set_minimum_size({ 1200, 800 });
            {
                auto& style = ImGui::GetStyle();
                style.WindowRounding = 0;
                style.ScrollbarRounding = 0.0f;
                style.ScrollbarSize = 20.0f;
                style.DisplaySafeAreaPadding = ImVec2(0.0f, 0.0f);
                style.DisplayWindowPadding = ImVec2(0.0f, 0.0f);
                style.ChildBorderSize = 1.0f;
                tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
            }
            //tostf::gl::Debug_logger gl_debug_logger;
            //gl_debug_logger.disable_severity(tostf::gl::dbg::severity::notification);
            Imgui_settings vis_imgui_settings;
            Case_selection_settings case_select_settings;
            Case_imgui_windows case_windows;
            auto& imgui_style = ImGui::GetStyle();
            imgui_style.WindowBorderSize = 0.0f;
            const float cam_near = 0.001f;
            auto checkerboard_shader = std::make_unique<tostf::V_F_shader>("../../shader/rendering/screenfilling_quad.vert",
                "../../shader/rendering/checkerboard_texture.frag");
            auto compositing_shader = std::make_unique<tostf::V_F_shader>("../../shader/rendering/screenfilling_quad.vert",
                "../../shader/rendering/texture_minimal.frag");
            const auto view_size = tostf::ui_size(window.get_resolution().x / 2, window.get_resolution().y
                - 2 * vis_imgui_settings.bottom_panel_height
                - vis_imgui_settings.top_control_height);
            tostf::View reference_view(tostf::ui_pos(0, 2 * vis_imgui_settings.bottom_panel_height), view_size);
            auto reference_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{ reference_view.get_size(), 1.0f });
            auto reference_fbo = std::make_unique<tostf::FBO>(reference_texture->get_res(),
                std::vector<tostf::FBO_texture_def>{
                reference_texture->get_fbo_tex_def()
            },
                std::vector<tostf::FBO_renderbuffer_def>{
                    tostf::FBO_renderbuffer_def{
                        tostf::tex::intern::depth32f_stencil8, false, 0
                    }
                });
            tostf::View comparing_view(tostf::ui_pos(view_size.x, 2 * vis_imgui_settings.bottom_panel_height), view_size);
            auto comparing_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{ comparing_view.get_size(), 1.0f });
            auto comparing_fbo = std::make_unique<tostf::FBO>(comparing_texture->get_res(),
                std::vector<tostf::FBO_texture_def>{
                comparing_texture->get_fbo_tex_def()
            },
                std::vector<tostf::FBO_renderbuffer_def>{
                    tostf::FBO_renderbuffer_def{
                        tostf::tex::intern::depth32f_stencil8, false, 0
                    }
                });
            main_view.resize(window.get_resolution());
            auto resize_window = [&](tostf::win_mgr::raw_win_ptr, const int x, const int y) {
                    main_view.resize({ x, y });
                    const auto new_view_size = tostf::ui_size(x / 2, y - 2 * vis_imgui_settings.bottom_panel_height
                        - vis_imgui_settings.top_control_height);
                    reference_view.resize(new_view_size);
                    comparing_view.resize(new_view_size);
                    reference_view.set_pos({ 0, 2 * vis_imgui_settings.bottom_panel_height });
                    comparing_view.set_pos({ new_view_size.x, 2 * vis_imgui_settings.bottom_panel_height });
                    if (x != 0 && y != 0) {
                        reference_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{ reference_view.get_size(), 1.0f });
                        reference_fbo = std::make_unique<tostf::FBO>(reference_texture->get_res(),
                            std::vector<tostf::FBO_texture_def>{
                            reference_texture->get_fbo_tex_def()
                        },
                            std::vector<tostf::FBO_renderbuffer_def>{
                                tostf::FBO_renderbuffer_def{
                                    tostf::tex::intern::depth32f_stencil8, false, 0
                                }
                            });
                        comparing_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{ comparing_view.get_size(), 1.0f });
                        comparing_fbo = std::make_unique<tostf::FBO>(comparing_texture->get_res(),
                            std::vector<tostf::FBO_texture_def>{
                            comparing_texture->get_fbo_tex_def()
                        },
                            std::vector<tostf::FBO_renderbuffer_def>{
                                tostf::FBO_renderbuffer_def{
                                    tostf::tex::intern::depth32f_stencil8, false, 0
                                }
                            });
                    }

            };
            window.add_resize_fun("window", resize_window);
            const auto resize_window_ref = [&](
                tostf::win_mgr::raw_win_ptr, const int x, const int y) {
                    main_view.resize({ x, y });
                    reference_view.resize({ x, y - vis_imgui_settings.bottom_panel_height - vis_imgui_settings.top_control_height });
                    reference_view.set_pos({ 0, vis_imgui_settings.bottom_panel_height });
                    if (x != 0 && y != 0) {
                        reference_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{ reference_view.get_size(), 1.0f });
                        reference_fbo = std::make_unique<tostf::FBO>(reference_texture->get_res(),
                            std::vector<tostf::FBO_texture_def>{
                            reference_texture->get_fbo_tex_def()
                        },
                            std::vector<tostf::FBO_renderbuffer_def>{
                                tostf::FBO_renderbuffer_def{
                                    tostf::tex::intern::depth32f_stencil8, false, 0
                                }
                            });
                    }
            };
            const auto resize_window_comp = [&](
                tostf::win_mgr::raw_win_ptr, const int x, const int y) {
                    main_view.resize({ x, y });
                    comparing_view.resize({ x, y - vis_imgui_settings.bottom_panel_height - vis_imgui_settings.top_control_height });
                    comparing_view.set_pos({ 0, vis_imgui_settings.bottom_panel_height });
                    if (x != 0 && y != 0) {
                        comparing_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{ comparing_view.get_size(), 1.0f });
                        comparing_fbo = std::make_unique<tostf::FBO>(comparing_texture->get_res(),
                            std::vector<tostf::FBO_texture_def>{
                            comparing_texture->get_fbo_tex_def()
                        },
                            std::vector<tostf::FBO_renderbuffer_def>{
                                tostf::FBO_renderbuffer_def{
                                    tostf::tex::intern::depth32f_stencil8, false, 0
                                }
                            });
                    }
            };
            tostf::vis::View_selector view_selector;
            tostf::Visualization_selector visualization_selector;
            tostf::vis::Case_comparer comparer;
            tostf::Colormap_handler color_maps("../../resources/colormaps/");
            enable(tostf::gl::cap::depth_test);
            visualization_selector.divide_by_count->update_uniform("default_value", -FLT_MAX);
            const auto init_colormap = [&]() {
                visualization_selector.surface_vis.shader->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.points_vis.shader->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.volume_vis.volume_to_surface->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.volume_vis.volume_to_plane->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.volume_vis.volume_to_surface_phong->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.volume_vis.volume_to_plane_phong->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.volume_vis.volume_ray_casting->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.flow_vis.render_streamlines->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
                visualization_selector.flow_vis.render_glyphs->update_uniform(
                    "colormap", color_maps.curr_map_it->second.tex->get_tex_handle());
            };
            init_colormap();

            const auto set_clipping_count = [&](const int count) {
                visualization_selector.surface_vis.shader->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.points_vis.shader->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.flow_vis.render_surface_front->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.flow_vis.render_surface_back->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.volume_ray_casting->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.volume_to_surface->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.volume_to_surface_phong->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.flow_vis.render_streamlines->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.flow_vis.render_glyphs->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.clipped_stencil->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.clipped_stencil_planes->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.volume_to_plane->update_uniform(
                    "clip_plane_count", count);
                visualization_selector.volume_vis.volume_to_plane_phong->update_uniform(
                    "clip_plane_count", count);
            };

            const auto enable_clipping = [&](const int count) {
                for (int i = 0; i < count; ++i) {
                    glEnable(GL_CLIP_DISTANCE0 + i);
                }
            };

            const auto disable_clipping = [&](const int count) {
                for (int i = 0; i < count; ++i) {
                    glDisable(GL_CLIP_DISTANCE0 + i);
                }
            };

            const auto render_overlay = [&](const std::unique_ptr<tostf::Texture2D> & texture,
                const tostf::vis::Overlay_settings & settings) {
                    auto mouse_pos = glm::vec2(window.get_cursor_pos());
                    const auto window_size = window.get_resolution();
                    mouse_pos.y = window_size.y - mouse_pos.y;
                    const auto curr_vp = tostf::gl::get_viewport();
                    enable(tostf::gl::cap::blend);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    checkerboard_shader->use();
                    checkerboard_shader->update_uniform("pattern", settings.pattern);
                    checkerboard_shader->update_uniform("scale", settings.scale);
                    checkerboard_shader->update_uniform("type", settings.type);
                    checkerboard_shader->update_uniform("aspect_ratio", curr_vp.width / static_cast<float>(curr_vp.height));
                    checkerboard_shader->update_uniform(
                        "mouse_uv",
                        glm::vec2(mouse_pos.x - curr_vp.x, mouse_pos.y - curr_vp.y) / glm::vec2(curr_vp.width, curr_vp.height));
                    checkerboard_shader->update_uniform("mouse_radius", settings.mouse_radius);
                    checkerboard_shader->update_uniform("tex", texture->get_tex_handle());
                    visualization_selector.helper_vao->draw(tostf::gl::draw::triangle_strip, 1, 4);
                    disable(tostf::gl::cap::blend);
            };

            const auto render_visualization = [&](const std::unique_ptr<tostf::vis::Case>& curr_case) {
                enable(tostf::gl::cap::depth_test);
                disable(tostf::gl::cap::blend);
                disable(tostf::gl::cap::stencil_test);
                if (curr_case->renderers_ready) {
                    curr_case->clipping.clipping_ssbo->bind_base(tostf::vis_ssbo_defines::clipping_planes);
                    set_clipping_count(static_cast<int>(curr_case->clipping.clipping_planes.size()));
                    enable_clipping(static_cast<int>(curr_case->clipping.clipping_planes.size()));
                    const auto curr_vis = visualization_selector.get_current_vis_type();
                    if (curr_vis == tostf::vis_type::surface) {
                        visualization_selector.surface_vis.shader->use();
                        curr_case->surface_renderer.vao->draw(tostf::gl::draw::triangles);
                    }
                    else if (curr_vis == tostf::vis_type::points) {
                        visualization_selector.points_vis.shader->use();
                        curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                            curr_case->points_renderer.counts.at(0).count);
                    }
                    else if (curr_vis == tostf::vis_type::points_boundary) {
                        visualization_selector.points_vis.shader->use();
                        for (unsigned c = 1; c < curr_case->points_renderer.counts.size(); ++c) {
                            curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                                curr_case->points_renderer.counts.at(c).count,
                                curr_case->points_renderer.counts.at(c).offset);
                        }
                    }
                    else if (curr_vis == tostf::vis_type::volume_to_surface
                        || curr_vis == tostf::vis_type::volume_to_surface_phong) {
                        visualization_selector.volume_vis.clipped_stencil->use();
                        enable(tostf::gl::cap::stencil_test);
                        glStencilFuncSeparate(GL_FRONT_AND_BACK, GL_ALWAYS, 0, 0);
                        glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_KEEP, GL_DECR_WRAP);
                        glStencilOpSeparate(GL_BACK, GL_KEEP, GL_KEEP, GL_INCR_WRAP);
                        disable(tostf::gl::cap::depth_test);
                        glDepthMask(GL_FALSE);
                        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                        curr_case->surface_renderer.vao->draw(tostf::gl::draw::triangles);
                        glStencilFunc(GL_NOTEQUAL, 0, ~0);
                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        glDepthMask(GL_TRUE);
                        disable(tostf::gl::cap::stencil_test);
                        enable(tostf::gl::cap::depth_test);
                        if (curr_vis == tostf::vis_type::volume_to_surface_phong) {
                            visualization_selector.volume_vis.volume_to_surface_phong->use();
                            visualization_selector.volume_vis.volume_to_surface_phong->update_uniform(
                                "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                        }
                        else {
                            visualization_selector.volume_vis.volume_to_surface->use();
                            visualization_selector.volume_vis.volume_to_surface->update_uniform(
                                "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                        }
                        curr_case->grid_ssbo->bind_base(tostf::vis_ssbo_defines::volume_grid);
                        curr_case->surface_renderer.vao->draw(tostf::gl::draw::triangles);
                        enable(tostf::gl::cap::stencil_test);
                        glStencilFunc(GL_NOTEQUAL, 0, ~0);
                        if (curr_vis == tostf::vis_type::volume_to_surface_phong) {
                            visualization_selector.volume_vis.volume_to_plane_phong->use();
                            visualization_selector.volume_vis.volume_to_plane_phong->update_uniform(
                                "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                        }
                        else {
                            visualization_selector.volume_vis.volume_to_plane->use();
                            visualization_selector.volume_vis.volume_to_plane->update_uniform(
                                "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                        }
                        visualization_selector.helper_vao->draw(tostf::gl::draw::triangle_strip,
                            static_cast<int>(curr_case->clipping.clipping_planes.size()),
                            4);
                        disable(tostf::gl::cap::stencil_test);
                    }
                    else if (curr_vis == tostf::vis_type::volume) {
                        glDepthMask(GL_FALSE);
                        visualization_selector.volume_vis.volume_ray_casting->use();
                        curr_case->grid_ssbo->bind_base(tostf::vis_ssbo_defines::volume_grid);
                        enable(tostf::gl::cap::blend);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        visualization_selector.volume_vis.volume_ray_casting->update_uniform(
                            "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                        visualization_selector.helper_vao->draw(tostf::gl::draw::triangles, 1, 4);
                        disable(tostf::gl::cap::blend);
                        glDepthMask(GL_TRUE);
                    }
                    else if (curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs
                        || curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs) {
                        enable(tostf::gl::cap::cull_face);
                        glCullFace(GL_FRONT);
                        visualization_selector.flow_vis.render_surface_back->use();
                        curr_case->surface_renderer.vao->draw(tostf::gl::draw::triangles);
                        disable(tostf::gl::cap::cull_face);
                        if (curr_case->flow_renderer.seeds_set
                            && (curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs)
                            || curr_case->flow_renderer.pathlines_generated
                            && (curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs)) {
                            curr_case->grid_ssbo->bind_base(tostf::vis_ssbo_defines::volume_grid);
                            if (curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::pathlines) {
                                visualization_selector.flow_vis.render_streamlines->use();
                                visualization_selector.flow_vis.render_streamlines->update_uniform(
                                    "step_count", curr_case->flow_renderer.settings.step_count);
                                visualization_selector.flow_vis.render_streamlines->update_uniform(
                                    "radius", curr_case->flow_renderer.settings.line_radius);
                                visualization_selector.flow_vis.render_streamlines->update_uniform(
                                    "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                            }
                            else {
                                visualization_selector.flow_vis.render_glyphs->use();
                                visualization_selector.flow_vis.render_glyphs->update_uniform(
                                    "step_count", curr_case->flow_renderer.settings.step_count);
                                visualization_selector.flow_vis.render_glyphs->update_uniform(
                                    "radius", curr_case->flow_renderer.settings.line_radius);
                                visualization_selector.flow_vis.render_glyphs->update_uniform(
                                    "steps_per_glyph",
                                    curr_case->flow_renderer.settings.step_count
                                    / curr_case->flow_renderer.settings.glyphs_per_line);
                                visualization_selector.flow_vis.render_glyphs->update_uniform(
                                    "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                            }
                            if (curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs) {
                                visualization_selector.flow_vis.render_streamlines->update_uniform(
                                    "step_count",
                                    curr_case->flow_renderer.pathline_settings.max_pathline_steps);
                                visualization_selector.flow_vis.render_glyphs->update_uniform(
                                    "step_count",
                                    curr_case->flow_renderer.pathline_settings.max_pathline_steps);
                                curr_case->flow_renderer.draw_pathlines(curr_case->player.current_step);
                            }
                            else {
                                curr_case->flow_renderer.draw_streamlines();
                            }
                        }
                        else {
                            visualization_selector.flow_vis.render_sphere->use();
                            visualization_selector.flow_vis.render_sphere->update_uniform(
                                "seed_sphere_pos", curr_case->flow_renderer.settings.seed_sphere_pos);
                            visualization_selector.flow_vis.render_sphere->update_uniform(
                                "seed_radius", curr_case->flow_renderer.settings.seed_sphere_radius);
                            visualization_selector.flow_vis.sphere_vao->draw(tostf::gl::draw::triangles);
                        }
                        enable(tostf::gl::cap::blend);
                        enable(tostf::gl::cap::cull_face);
                        glCullFace(GL_BACK);
                        glBlendFunc(GL_DST_ALPHA, GL_SRC_ALPHA);
                        visualization_selector.flow_vis.render_surface_front->use();
                        curr_case->surface_renderer.vao->draw(tostf::gl::draw::triangles);
                        disable(tostf::gl::cap::cull_face);
                        disable(tostf::gl::cap::blend);
                    }
                    disable_clipping(static_cast<int>(curr_case->clipping.clipping_planes.size()));
                }
            };

            const auto integrate_pathlines = [&](const std::unique_ptr<tostf::vis::Case> & curr_case, const int curr_step) {
                const auto curr_vis = visualization_selector.get_current_vis_type();
                float step_size = curr_case->flow_renderer.pathline_settings.step_size;
                if (curr_case && (curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs)
                    && curr_case->flow_renderer.pathlines_generated && curr_case->fields.find("U") != curr_case->fields.end()) {
                    auto step_path = curr_case->player.steps.at(curr_case->player.current_step);
                    auto field = curr_case->mesh.load_vector_field_from_file(step_path, "U");
                    step_path = curr_case->player.steps.at(curr_case->player.current_step + 1);
                    auto next_field = curr_case->mesh.load_vector_field_from_file(step_path, "U");
                    if (!field.internal_data.empty()) {
                        curr_case->points_renderer.vbo_vector->set_data(field.internal_data);
                    }
                    curr_case->points_renderer.vao->attach_vbo(curr_case->points_renderer.vbo_vector, 2);

                    disable(tostf::gl::cap::depth_test);
                    enable(tostf::gl::cap::blend);
                    glBlendFunc(GL_ONE, GL_ONE);

                    curr_case->grid_ssbo->bind_base(tostf::vis_ssbo_defines::volume_grid);
                    const auto tex_res = curr_case->volume_renderer.volume_tex->get_res();
                    visualization_selector.volume_vis.vector_to_texture->update_uniform(
                        "view", curr_case->volume_renderer.cam_view);
                    visualization_selector.volume_vis.vector_to_texture->update_uniform(
                        "proj", curr_case->volume_renderer.cam_ortho);
                    visualization_selector.volume_vis.vector_to_texture->update_uniform(
                        "max_layers", static_cast<int>(tex_res.z));
                    tostf::gl::set_viewport(tex_res.x, tex_res.y);
                    tostf::gl::set_clear_color(0.0f, 0.0f, 0.0f, 0.0f);

                    curr_case->volume_renderer.volume_fbo->bind();
                    clear(tostf::gl::clear_bit::color | tostf::gl::clear_bit::depth);

                    visualization_selector.volume_vis.vector_to_texture->use();
                    curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                        curr_case->points_renderer.counts.at(0).count);
                    glFinish();
                    visualization_selector.divide_by_count->update_uniform(
                        "data", curr_case->volume_renderer.volume_tex->get_image_handle(
                            tostf::gl::img_access::read_write, 0, 0, true));
                    visualization_selector.divide_by_count->update_uniform("default_value", 0.0f);
                    visualization_selector.divide_by_count->run(tex_res.x, tex_res.y, tex_res.z);

                    visualization_selector.flow_vis.gen_pathlines_start_step->update_uniform(
                        "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                    visualization_selector.flow_vis.gen_pathlines_start_step->update_uniform(
                        "line_count", curr_case->flow_renderer.settings.line_count);
                    visualization_selector.flow_vis.gen_pathlines_start_step->update_uniform(
                        "step_size", step_size);
                    visualization_selector.flow_vis.gen_pathlines_start_step->run(
                        curr_case->flow_renderer.settings.line_count);

                    curr_case->volume_renderer.volume_fbo->bind();
                    clear(tostf::gl::clear_bit::color | tostf::gl::clear_bit::depth);

                    visualization_selector.volume_vis.vector_to_texture->use();
                    curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                        curr_case->points_renderer.counts.at(0).count);

                    if (!next_field.internal_data.empty()) {
                        curr_case->points_renderer.vbo_vector->set_data(next_field.internal_data);
                    }
                    curr_case->points_renderer.vao->attach_vbo(curr_case->points_renderer.vbo_vector, 2);

                    visualization_selector.volume_vis.vector_to_texture->use();
                    curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                        curr_case->points_renderer.counts.at(0).count);

                    visualization_selector.divide_by_count->update_uniform(
                        "data", curr_case->volume_renderer.volume_tex->get_image_handle(
                            tostf::gl::img_access::read_write, 0, 0, true));
                    visualization_selector.divide_by_count->update_uniform("default_value", 0.0f);
                    visualization_selector.divide_by_count->run(tex_res.x, tex_res.y, tex_res.z);

                    visualization_selector.flow_vis.gen_pathlines_half_step->update_uniform(
                        "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                    visualization_selector.flow_vis.gen_pathlines_half_step->update_uniform("step_size", step_size);
                    visualization_selector.flow_vis.gen_pathlines_half_step->update_uniform(
                        "line_count", curr_case->flow_renderer.settings.line_count);
                    visualization_selector.flow_vis.gen_pathlines_half_step->run(
                        curr_case->flow_renderer.settings.line_count);

                    clear(tostf::gl::clear_bit::color | tostf::gl::clear_bit::depth);

                    visualization_selector.volume_vis.vector_to_texture->use();
                    curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                        curr_case->points_renderer.counts.at(0).count);

                    visualization_selector.divide_by_count->update_uniform(
                        "data", curr_case->volume_renderer.volume_tex->get_image_handle(
                            tostf::gl::img_access::read_write, 0, 0, true));
                    visualization_selector.divide_by_count->update_uniform("default_value", 0.0f);
                    visualization_selector.divide_by_count->run(tex_res.x, tex_res.y, tex_res.z);

                    visualization_selector.flow_vis.gen_pathlines_integrate->update_uniform(
                        "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                    visualization_selector.flow_vis.gen_pathlines_integrate->update_uniform("step_size", step_size);
                    visualization_selector.flow_vis.gen_pathlines_integrate->update_uniform(
                        "step_count", curr_case->flow_renderer.pathline_settings.max_pathline_steps);
                    visualization_selector.flow_vis.gen_pathlines_integrate->update_uniform(
                        "curr_step", curr_step);
                    visualization_selector.flow_vis.gen_pathlines_integrate->update_uniform(
                        "line_count", curr_case->flow_renderer.settings.line_count);
                    visualization_selector.flow_vis.gen_pathlines_integrate->run(
                        curr_case->flow_renderer.settings.line_count);

                    disable(tostf::gl::cap::blend);
                    enable(tostf::gl::cap::depth_test);
                    tostf::FBO::unbind();
                    tostf::gl::set_viewport(window.get_resolution().x, window.get_resolution().y);
                    if (vis_imgui_settings.dark_mode) {
                        tostf::gl::set_clear_color(0.3f, 0.3f, 0.3f, 1);
                    }
                    else {
                        tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
                    }
                }
            };

            const auto init_pathlines = [&](const std::unique_ptr<tostf::vis::Case> & curr_case) {
                const auto curr_vis = visualization_selector.get_current_vis_type();
                if (curr_case && (curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs)
                    && curr_case->fields.find("U") != curr_case->fields.end()) {
                    curr_case->calc_minmax("U");
                    const auto max_value = curr_case->fields.at("U").max_scalar;
                    const auto step_size = curr_case->volume_renderer.grid_info.cell_size
                        * curr_case->flow_renderer.settings.step_size_factor
                        / max_value;
                    curr_case->flow_renderer.init_pathlines_buffer();
                    curr_case->flow_renderer.init_pathline_integrators();
                    curr_case->flow_renderer.pathline_settings.start_pathline_step = curr_case->player.current_step;
                    if (curr_case->flow_renderer.pathline_settings.start_pathline_step < curr_case->player.get_max_step_id()) {
                        curr_case->flow_renderer.pathlines->bind_base(tostf::vis_ssbo_defines::streamlines);
                        for (int i = 0; i < curr_case->flow_renderer.pathline_settings.max_pathline_steps
                            - curr_case->player.current_step; ++i) {
                            integrate_pathlines(curr_case, i);
                        }
                    }
                }
            };

            const auto init_streamlines = [&](const std::unique_ptr<tostf::vis::Case> & curr_case) {
                if (curr_case && curr_case->flow_renderer.seeds_set) {
                    const auto curr_vis = visualization_selector.get_current_vis_type();
                    if ((curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs)
                        && curr_case->fields.find("U") != curr_case->fields.end()) {
                        const auto step_path = curr_case->player.steps.at(curr_case->player.current_step);
                        auto field = curr_case->mesh.load_vector_field_from_file(step_path, "U");
                        if (!field.internal_data.empty()) {
                            curr_case->points_renderer.vbo_vector->set_data(field.internal_data);
                        }
                        curr_case->points_renderer.vao->attach_vbo(curr_case->points_renderer.vbo_vector, 2);
                        curr_case->grid_ssbo->bind_base(tostf::vis_ssbo_defines::volume_grid);
                        const auto tex_res = curr_case->volume_renderer.volume_tex->get_res();
                        tostf::gl::set_viewport(tex_res.x, tex_res.y);
                        curr_case->volume_renderer.volume_fbo->bind();
                        tostf::gl::set_clear_color(0.0f, 0.0f, 0.0f, 0.0f);
                        clear(tostf::gl::clear_bit::color | tostf::gl::clear_bit::depth);
                        visualization_selector.volume_vis.vector_to_texture->use();
                        visualization_selector.volume_vis.vector_to_texture->update_uniform(
                            "view", curr_case->volume_renderer.cam_view);
                        visualization_selector.volume_vis.vector_to_texture->update_uniform(
                            "proj", curr_case->volume_renderer.cam_ortho);
                        visualization_selector.volume_vis.vector_to_texture->update_uniform(
                            "max_layers", static_cast<int>(tex_res.z));
                        disable(tostf::gl::cap::depth_test);
                        enable(tostf::gl::cap::blend);
                        glBlendFunc(GL_ONE, GL_ONE);
                        curr_case->points_renderer.vao->draw(tostf::gl::draw::points, 1,
                            curr_case->points_renderer.counts.at(0).count);
                        glFinish();
                        disable(tostf::gl::cap::blend);
                        enable(tostf::gl::cap::depth_test);
                        tostf::FBO::unbind();
                        tostf::gl::set_viewport(window.get_resolution().x, window.get_resolution().y);
                        if (vis_imgui_settings.dark_mode) {
                            tostf::gl::set_clear_color(0.3f, 0.3f, 0.3f, 1);
                        }
                        else {
                            tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
                        }
                        visualization_selector.divide_by_count->update_uniform(
                            "data", curr_case->volume_renderer.volume_tex->get_image_handle(
                                tostf::gl::img_access::read_write, 0, 0, true));
                        visualization_selector.divide_by_count->update_uniform("default_value", 0.0f);
                        visualization_selector.divide_by_count->run(tex_res.x, tex_res.y, tex_res.z);
                        const auto step_size = curr_case->volume_renderer.grid_info.cell_size
                            * curr_case->flow_renderer.settings.step_size_factor;
                        curr_case->flow_renderer.init_line_buffer();
                        curr_case->flow_renderer.init_seeds();
                        glFinish();
                        curr_case->flow_renderer.lines->bind_base(tostf::vis_ssbo_defines::streamlines);
                        visualization_selector.flow_vis.generate_streamlines->update_uniform(
                            "data", curr_case->volume_renderer.volume_tex->get_tex_handle());
                        visualization_selector.flow_vis.generate_streamlines->update_uniform(
                            "line_count", curr_case->flow_renderer.settings.line_count);
                        visualization_selector.flow_vis.generate_streamlines->update_uniform(
                            "step_count", curr_case->flow_renderer.settings.step_count);
                        visualization_selector.flow_vis.generate_streamlines->update_uniform("step_size", step_size);
                        visualization_selector.flow_vis.generate_streamlines->update_uniform(
                            "max_flow", field.max);
                        visualization_selector.flow_vis.generate_streamlines->update_uniform(
                            "equal_length", curr_case->flow_renderer.settings.equal_length);
                        visualization_selector.flow_vis.generate_streamlines->run(
                            curr_case->flow_renderer.settings.line_count);
                        glFinish();
                    }
                }
            };

            const auto load_field = [&](const std::unique_ptr<tostf::vis::Case> & curr_case, const std::string & field_name) {
                const auto curr_vis = visualization_selector.get_current_vis_type();
                float min_v = FLT_MAX;
                float max_v = -FLT_MAX;
                if (curr_case) {
                    auto field = curr_case->load_scalar_field(field_name);
                    min_v = glm::min(min_v, curr_case->fields.at(field_name).min_scalar);
                    max_v = glm::max(max_v, curr_case->fields.at(field_name).max_scalar);
                    if (curr_vis == tostf::vis_type::points || curr_vis == tostf::vis_type::points_boundary
                        || curr_vis == tostf::vis_type::volume_to_surface || curr_vis == tostf::vis_type::volume
                        || curr_vis == tostf::vis_type::volume_to_surface_phong
                        || curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs
                        || curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs) {
                        if (!field.internal_data.empty()) {
                            curr_case->points_renderer.vbo_scalar->set_data(field.internal_data, 0);
                        }
                        for (unsigned b_id = 0; b_id < field.boundaries_data.size(); ++b_id) {
                            if (!field.boundaries_data.at(b_id).empty()) {
                                auto render_counts_it = curr_case->points_renderer.counts.begin();
                                std::advance(render_counts_it, b_id + 1);
                                curr_case->points_renderer.vbo_scalar->set_data(field.boundaries_data.at(b_id),
                                    render_counts_it->offset * sizeof(float));
                            }
                        }
                        curr_case->points_renderer.vao->attach_vbo(curr_case->points_renderer.vbo_scalar, 2);
                    }
                    if (curr_vis == tostf::vis_type::surface) {
                        for (unsigned b_id = 0; b_id < field.boundaries_data.size(); ++b_id) {
                            if (!field.boundaries_data.at(b_id).empty()) {
                                const auto& field_data = field.boundaries_data.at(b_id);
                                auto mesh_interpolation_it = curr_case->mesh_interpolations.begin();
                                std::advance(mesh_interpolation_it, b_id);
                                const auto surface_size = mesh_interpolation_it->second.point_ids.size();
                                std::vector<float> scalar_data(surface_size);
        #pragma omp parallel for
                                for (int p_id = 0; p_id < static_cast<int>(surface_size); ++p_id) {
                                    const auto& data_point_refs = mesh_interpolation_it->second.point_ids.at(p_id);
                                    const auto& weights = mesh_interpolation_it->second.weights.at(p_id);
                                    float point_data = 0.0f;
                                    float weight_sum = 0.0f;
                                    for (int dp_id = 0; dp_id < static_cast<int>(weights.size()); ++dp_id) {
                                        point_data += field_data.at(data_point_refs.at(dp_id)) * weights.at(dp_id);
                                        weight_sum += weights.at(dp_id);
                                    }
                                    if (weight_sum > 0.0f) {
                                        scalar_data.at(p_id) = point_data / weight_sum;
                                    }
                                }
                                auto render_counts_it = curr_case->surface_renderer.counts.begin();
                                std::advance(render_counts_it, b_id);
                                curr_case->surface_renderer.vbo_scalar->set_data(
                                    scalar_data, render_counts_it->offset * sizeof(float));
                            }
                            curr_case->surface_renderer.vao->attach_vbo(curr_case->surface_renderer.vbo_scalar, 2);
                        }
                    }
                    if (curr_vis == tostf::vis_type::volume_to_surface || curr_vis == tostf::vis_type::volume
                        || curr_vis == tostf::vis_type::volume_to_surface_phong
                        || curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs
                        || curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs) {
                        const auto tex_res = curr_case->volume_renderer.volume_tex->get_res();
                        tostf::gl::set_viewport(tex_res.x, tex_res.y);
                        curr_case->volume_renderer.volume_fbo->bind();
                        tostf::gl::set_clear_color(0.0f, 0.0f, 0.0f, 0.0f);
                        clear(tostf::gl::clear_bit::color | tostf::gl::clear_bit::depth);
                        curr_case->grid_ssbo->bind_base(tostf::vis_ssbo_defines::volume_grid);
                        visualization_selector.volume_vis.scalar_to_texture->use();
                        visualization_selector.volume_vis.scalar_to_texture->update_uniform(
                            "view", curr_case->volume_renderer.cam_view);
                        visualization_selector.volume_vis.scalar_to_texture->update_uniform(
                            "proj", curr_case->volume_renderer.cam_ortho);
                        visualization_selector.volume_vis.scalar_to_texture->update_uniform(
                            "max_layers", static_cast<int>(tex_res.z));
                        disable(tostf::gl::cap::depth_test);
                        enable(tostf::gl::cap::blend);
                        glBlendFunc(GL_ONE, GL_ONE);
                        curr_case->points_renderer.vao->draw(tostf::gl::draw::points);
                        glFinish();
                        disable(tostf::gl::cap::blend);
                        enable(tostf::gl::cap::depth_test);
                        tostf::FBO::unbind();
                        tostf::gl::set_viewport(window.get_resolution().x, window.get_resolution().y);
                        if (vis_imgui_settings.dark_mode) {
                            tostf::gl::set_clear_color(0.3f, 0.3f, 0.3f, 1);
                        }
                        else {
                            tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
                        }
                        visualization_selector.divide_by_count->update_uniform(
                            "data", curr_case->volume_renderer.volume_tex->get_image_handle(
                                tostf::gl::img_access::read_write, 0, 0, true));
                        visualization_selector.divide_by_count->update_uniform("default_value", -FLT_MAX);
                        visualization_selector.divide_by_count->run(tex_res.x, tex_res.y, tex_res.z);
                        glFinish();
                    }
                }
                return std::make_pair<>(min_v, max_v);
            };

            const auto finish_case_loading = [&](const std::unique_ptr<tostf::vis::Case> & curr_case) {
                const auto current_cam_radius = length(glm::vec3(curr_case->mesh.mesh_bb.max
                    - curr_case->mesh.mesh_bb.min)) / 2.0f;
                cam->set_radius(current_cam_radius);
                cam->set_near(curr_case->mesh_scale * 0.1f);
                cam->set_move_speed(current_cam_radius);
                curr_case->init_renderers();
            };

            const auto vis_display_legend = [&](const ImVec2 & pos, const std::string & name) {
                const auto legend_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                const auto frame_padding = ImGui::GetStyle().FramePadding;
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                const auto font_size = ImGui::GetFontSize();
                ImGui::SetNextWindowSize(ImVec2(150.0f, comparer.legend.height + font_size));
                ImGui::SetNextWindowPos(pos);
                if (ImGui::Begin(("Legend##" + name).c_str(), nullptr, legend_window_flags)) {
                    if (!vis_imgui_settings.dark_mode) {
                        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(35, 35, 35, 255));
                    }
                    const auto legend_top_left = ImGui::GetCursorScreenPos();
                    const auto legend_x = legend_top_left.x;
                    auto legend_y = legend_top_left.y;
                    legend_y += font_size / 2.0f;
                    ImGui::SetCursorScreenPos(ImVec2(legend_x, legend_y));
                    ImGui::RotatedImage(ImGui::ConvertGLuintToImTex(color_maps.curr_map_it->second.tex->get_name()),
                        ImVec2(20.0f, comparer.legend.height));
                    for (auto& s : comparer.legend.steps) {
                        const auto height_offset = comparer.legend.height - comparer.legend.value_to_height(s);
                        ImGui::SetCursorScreenPos(ImVec2(
                            legend_top_left.x + comparer.legend.line_length + 2.0f * frame_padding.x,
                            legend_y + height_offset - font_size / 2.0f - 2.0f));
                        ImGui::Text("%s", tostf::float_to_scientific_str(s).c_str());
                        ImGui::GetWindowDrawList()->AddLine(ImVec2(legend_x, legend_y + height_offset),
                            ImVec2(legend_x + comparer.legend.line_length,
                                legend_y + height_offset),
                            ImGui::GetColorU32(ImGuiCol_Text), 2.0f);
                    }
                    if (!vis_imgui_settings.dark_mode) {
                        ImGui::PopStyleColor();
                    }
                }
                ImGui::End();
                ImGui::PopStyleVar(5);
            };

            const auto display_top_panel = [&]() {
                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x, vis_imgui_settings.top_control_height));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 5.0f));
                const auto eye_symbol_size = ImGui::CalcTextSize(ICON_MDI_EYE);
                if (ImGui::Begin("##toppanel", nullptr, vis_imgui_settings.control_window_flags)) {
                    ImGui::Columns(2, nullptr, false);
                    ImGui::SetColumnWidth(0, window.get_resolution().x - vis_imgui_settings.view_box_width
                        - imgui_style.FramePadding.x * 5.0f - eye_symbol_size.x);
                    ImGui::SetColumnWidth(
                        1, vis_imgui_settings.view_box_width + imgui_style.FramePadding.x * 5.0f + eye_symbol_size.x);
                    if (ImGui::Button("New preprocessing")) {
                        active_tool = 0;
                        sim_info = {};
                    }
                    ImGui::SameLine();
                    ImGui::PushItemWidth(vis_imgui_settings.view_box_width);
                    const auto curr_vis_type = visualization_selector.get_current_vis_type();
                    const auto curr_vis_type_int = to_underlying_type(curr_vis_type);
                    if (ImGui::BeginCombo("##comboselectvistype", vis_type_to_str(curr_vis_type).c_str())) {
                        for (int i = 0; i < static_cast<int>(tostf::vis_type::count); ++i) {
                            const auto vis_type_i = static_cast<tostf::vis_type>(i);
                            const bool is_selected = curr_vis_type_int == i;
                            if (ImGui::Selectable(vis_type_to_str(vis_type_i).c_str(), is_selected)) {
                                if (!is_selected) {
                                    visualization_selector.activate_vis(vis_type_i);
                                }
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    ImGui::PushItemWidth(vis_imgui_settings.field_box_width);
                    if (ImGui::BeginCombo("##comboselectfield", comparer.get_field_name(comparer.selected_field).c_str())) {
                        for (int i = -1; i < static_cast<int>(comparer.fields.size()); ++i) {
                            const bool is_selected = comparer.selected_field == i;
                            if (ImGui::Selectable(comparer.get_field_name(i).c_str(), is_selected)) {
                                if (!is_selected) {
                                    comparer.selected_field = i;
                                    if (i != -1) {
                                        auto field_it = comparer.fields.begin();
                                        std::advance(field_it, i);
                                        const auto minmax_ref = load_field(comparer.reference, field_it->first);
                                        const auto minmax_comp = load_field(comparer.comparing, field_it->first);
                                        const auto min_v = glm::min(minmax_ref.first, minmax_comp.first);
                                        const auto max_v = glm::max(minmax_ref.second, minmax_comp.second);
                                        comparer.fields.at(comparer.get_field_name(i)).max_scalar = max_v;
                                        comparer.fields.at(comparer.get_field_name(i)).min_scalar = min_v;
                                        comparer.fields.at(comparer.get_field_name(i)).minmax_set = true;
                                        visualization_selector.update_minmax(min_v, max_v);
                                        if (comparer.reference || comparer.comparing) {
                                            comparer.legend.init_precision(min_v, max_v);
                                            comparer.legend.init(min_v, max_v);
                                        }
                                    }
                                }
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                    ImGui::SameLine();
                    const ImVec2 combo_pos = ImGui::GetCursorPos();
                    const float image_height = ImGui::GetTextLineHeightWithSpacing() - imgui_style.FramePadding.y;
                    const auto font = ImGui::GetFont();
                    const auto font_width = font->FallbackAdvanceX;
                    ImGui::PushItemWidth(vis_imgui_settings.colormap_box_width);
                    if (ImGui::BeginCombo("##combocolormap", "")) {
                        for (auto m_it = color_maps.maps.begin(); m_it != color_maps.maps.end(); ++m_it) {
                            const bool is_selected = m_it == color_maps.curr_map_it;
                            if (ImGui::Selectable(m_it->first.c_str(), is_selected)) {
                                color_maps.curr_map_it = m_it;
                                init_colormap();
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                            ImGui::SameLine(vis_imgui_settings.colormap_box_width + font_width * color_maps.max_name_length
                                - vis_imgui_settings.colormap_image_width);
                            ImGui::Image(ImGui::ConvertGLuintToImTex(m_it->second.tex->get_name()),
                                ImVec2(vis_imgui_settings.colormap_image_width, image_height));
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::SetCursorPos(ImVec2(combo_pos.x + imgui_style.FramePadding.x,
                        combo_pos.y + imgui_style.FramePadding.y));
                    ImGui::Image(ImGui::ConvertGLuintToImTex(color_maps.curr_map_it->second.tex->get_name()),
                        ImVec2(vis_imgui_settings.colormap_image_width, image_height));
                    ImGui::PopItemWidth();
                    ImGui::SetCursorPos(ImVec2(combo_pos.x + vis_imgui_settings.colormap_box_width, combo_pos.y));
                    ImGui::SameLine(0, vis_imgui_settings.colormap_box_width
                        - vis_imgui_settings.colormap_image_width + imgui_style.ItemSpacing.x);
                    const auto curr_pos = ImGui::GetCursorPos();
                    ImGui::SetCursorPos(ImVec2(curr_pos.x, curr_pos.y - imgui_style.FramePadding.y));
                    if (ImGui::Button(ICON_MDI_PLUS "##addcolormap")) {
                        auto colormap_folder = tostf::show_select_folder_dialog("Select colormap folder",
                            std::filesystem::current_path());
                        if (colormap_folder) {
                            color_maps.add_colormap_folder(colormap_folder.value());
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MDI_WRENCH "##editlegendscale")) {
                        ImGui::OpenPopup("edit_legend_scale_popup");
                    }
                    if (ImGui::BeginPopup("edit_legend_scale_popup")) {
                        ImGui::Text("Edit scale");
                        ImGui::Separator();
                        auto legend_min_v = comparer.legend.min_value;
                        auto legend_max_v = comparer.legend.max_value;
                        const auto step_size = pow(10.0f, comparer.legend.precision - 3);
                        const auto scale_format = "%." + std::to_string(glm::max(-comparer.legend.precision + 3, 3)) + "f";
                        auto changed_scale = ImGui::DragFloat("min##legend_scale", &legend_min_v, step_size, -FLT_MAX,
                            legend_max_v, scale_format.c_str());
                        changed_scale = ImGui::DragFloat("max##legend_scale", &legend_max_v, step_size, legend_min_v, FLT_MAX,
                            scale_format.c_str())
                            || changed_scale;
                        if (changed_scale) {
                            comparer.legend.init(legend_min_v, legend_max_v);
                            visualization_selector.update_minmax(legend_min_v, legend_max_v);
                        }
                        if (ImGui::Button("Reset scale##legend_scale")) {
                            const auto curr_field_desc = comparer.get_current_field_desc();
                            comparer.legend.init(curr_field_desc.min_scalar, curr_field_desc.max_scalar);
                            visualization_selector.update_minmax(comparer.legend.min_value, comparer.legend.max_value);
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::SameLine();
                    std::string dark_mode_str("Activate dark mode");
                    if (vis_imgui_settings.dark_mode) {
                        dark_mode_str = "Deactivate dark mode";
                    }
                    if (ImGui::Button(dark_mode_str.c_str())) {
                        vis_imgui_settings.dark_mode = !vis_imgui_settings.dark_mode;
                        if (vis_imgui_settings.dark_mode) {
                            tostf::gl::set_clear_color(0.3f, 0.3f, 0.3f, 1);
                        }
                        else {
                            tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
                        }
                    }
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(vis_imgui_settings.view_box_width);
                    if (ImGui::BeginCombo(ICON_MDI_EYE "##comboselectview", to_string(view_selector.current_view).c_str())) {
                        for (int i = 0; i < tostf::to_underlying_type(tostf::vis::vis_view::count); ++i) {
                            const bool is_selected = tostf::to_underlying_type(view_selector.current_view) == i;
                            if (ImGui::Selectable(to_string(static_cast<tostf::vis::vis_view>(i)).c_str(), is_selected)) {
                                view_selector.current_view = static_cast<tostf::vis::vis_view>(i);
                                if (view_selector.current_view == tostf::vis::vis_view::comparing_case) {
                                    window.add_resize_fun("window", resize_window_comp);
                                    resize_window_comp(window.get()->get(), window.get_resolution().x,
                                        window.get_resolution().y);
                                }
                                else if (view_selector.current_view == tostf::vis::vis_view::reference_case) {
                                    window.add_resize_fun("window", resize_window_ref);
                                    resize_window_ref(window.get()->get(), window.get_resolution().x,
                                        window.get_resolution().y);
                                }
                                else {
                                    window.add_resize_fun("window", resize_window);
                                    resize_window(window.get()->get(), window.get_resolution().x, window.get_resolution().y);
                                }
                            }
                            if (is_selected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                }
                ImGui::End();
                ImGui::PopStyleVar(2);
            };

            const auto display_bottom_panel = [&]() {
                ImGui::SetNextWindowPos(ImVec2(0, window.get_resolution().y - vis_imgui_settings.bottom_panel_height));
                ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x, vis_imgui_settings.bottom_panel_height));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                const auto player_size = ImGui::CalcTextSize(ICON_MDI_PAUSE ICON_MDI_SKIP_PREVIOUS
                    ICON_MDI_SKIP_NEXT ICON_MDI_STOP ICON_MDI_REPEAT).x
                    + imgui_style.ItemSpacing.x * 6.0f + imgui_style.FramePadding.x * 5.0f;
                if (ImGui::Begin("##bottom_panel", nullptr, vis_imgui_settings.control_window_flags)) {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - player_size) / 2.0f);
                    if (comparer.is_playing()) {
                        if (ImGui::Button(ICON_MDI_PAUSE "##mainplayer")) {
                            comparer.pause();
                        }
                    }
                    else {
                        if (ImGui::Button(ICON_MDI_PLAY "##mainplayer")) {
                            comparer.play();
                        }
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MDI_SKIP_PREVIOUS "##mainplayer")) {
                        comparer.skip_previous();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MDI_STOP "##mainplayer")) {
                        comparer.stop();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(ICON_MDI_SKIP_NEXT "##mainplayer")) {
                        comparer.skip_next();
                    }
                    ImGui::SameLine();
                    if (comparer.is_repeating()) {
                        if (ImGui::Button(ICON_MDI_REPEAT "##mainplayer")) {
                            comparer.toggle_repeat();
                        }
                    }
                    else {
                        if (ImGui::Button(ICON_MDI_REPEAT_OFF "##mainplayer")) {
                            comparer.toggle_repeat();
                        }
                    }
                    ImGui::SameLine();
                }
                ImGui::End();
                ImGui::PopStyleVar(2);
            };

            const auto display_vis_settings = [&](std::unique_ptr<tostf::vis::Case> & curr_case,
                std::unique_ptr<tostf::vis::Case> & other_case, const std::string & name,
                const ImVec2 & pos, const ImVec2 & size, const bool active) {
                    bool clicked = active;
                    if (curr_case) {
                        const auto curr_vis = visualization_selector.get_current_vis_type();
                        if (active) {
                            const auto offset_size = ImVec2(size.x - vis_imgui_settings.vis_settings_window_width,
                                ImGui::CalcTextSize("A").y + imgui_style.FramePadding.y * 2.0f);
                            ImGui::SetNextWindowPos(pos + offset_size, 0, ImVec2(0, 0));
                            ImGui::SetNextWindowSize(ImVec2(vis_imgui_settings.vis_settings_window_width,
                                size.y - offset_size.y - vis_imgui_settings.bottom_panel_height));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                            if (ImGui::Begin(("Visualization settings##" + name).c_str(), &clicked,
                                vis_imgui_settings.control_window_flags)) {
                                if (ImGui::BeginCollapsingSection("Clipping Planes", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                                    const auto plane_count = static_cast<int>(curr_case->clipping.clipping_planes.size());
                                    bool changed = false;
                                    for (int p_id = 0; p_id < plane_count; ++p_id) {
                                        auto& clipping_plane = curr_case->clipping.clipping_planes.at(p_id);
                                        ImGui::Text("%s", ("Clipping plane " + std::to_string(p_id + 1)).c_str());
                                        std::string label("##plane" + std::to_string(p_id) + name);
                                        ImGui::SameLine();
                                        if (ImGui::Button((ICON_MDI_DELETE + label).c_str())) {
                                            curr_case->clipping.clipping_planes.erase(
                                                curr_case->clipping.clipping_planes.begin() + p_id);
                                            changed = true;
                                            break;
                                        }
                                        auto format_scale =
                                            "%." + std::to_string(glm::max(-tostf::get_float_exp(curr_case->mesh_scale) + 2, 3))
                                            + "f";
                                        changed = ImGui::DragFloat3(("Point on plane" + label).c_str(),
                                            &clipping_plane.point_on_plane[0],
                                            curr_case->mesh_scale * 0.01f, 0, 0,
                                            format_scale.c_str()) || changed;
                                        changed = ImGui::DragFloat3(("Plane normal" + label).c_str(),
                                            &clipping_plane.plane_normal[0],
                                            curr_case->mesh_scale * 0.01f, 0, 0,
                                            format_scale.c_str()) || changed;
                                        ImGui::Separator();
                                    }
                                    if (plane_count < curr_case->clipping.max_planes_count) {
                                        if (ImGui::Button(("Add clipping plane##" + name).c_str())) {
                                            curr_case->clipping.clipping_planes.emplace_back();
                                            changed = true;
                                        }
                                    }
                                    if (changed) {
                                        auto temp_planes = curr_case->clipping.clipping_planes;
                                        for (auto& c : temp_planes) {
                                            c.plane_normal = normalize(c.plane_normal);
                                        }
                                        curr_case->clipping.clipping_ssbo->set_data(temp_planes);
                                    }
                                    ImGui::EndCollapsingSection();
                                }
                                if (view_selector.current_view != tostf::vis::vis_view::both) {
                                    if (ImGui::BeginCollapsingSection("Overlay settings", nullptr, ImGuiTreeNodeFlags_DefaultOpen)
                                        ) {
                                        ImGui::Checkbox("Set active", &curr_case->overlay_settings.active);
                                        if (curr_case->overlay_settings.active) {
                                            ImGui::SliderFloat("Overlay radius", &curr_case->overlay_settings.mouse_radius, 0,
                                                1.0f);
                                            ImGui::Checkbox("Pattern", &curr_case->overlay_settings.pattern);
                                            if (curr_case->overlay_settings.pattern) {
                                                ImGui::DragFloat("Pattern scale", &curr_case->overlay_settings.scale, 0.1f, 0.0f,
                                                    FLT_MAX);
                                                ImGui::DragFloat("Pattern type", &curr_case->overlay_settings.type, 0.1f, 1.0f,
                                                    FLT_MAX);
                                            }
                                        }
                                        ImGui::EndCollapsingSection();
                                    }
                                }
                                if (ImGui::BeginCollapsingSection("Phong settings", nullptr)) {
                                    ImGui::Text("Ambient color");
                                    ImGui::ColorEdit4("##Ambient color", &visualization_selector.diffuse_settings.ambient_color[0]);
                                    ImGui::Text("Light position");
                                    ImGui::DragFloat4("##Light position", &visualization_selector.diffuse_settings.light_pos[0],
                                        0.01);
                                    ImGui::Text("Light color");
                                    ImGui::ColorEdit4("##Light color", &visualization_selector.diffuse_settings.light_color[0]);
                                    ImGui::Text("Shininess");
                                    ImGui::DragFloat("##Shininess", &visualization_selector.diffuse_settings.shininess, 0.01);
                                    visualization_selector.diffuse_ssbo->set_data(visualization_selector.diffuse_settings);
                                    ImGui::EndCollapsingSection();
                                }
                                if (curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs
                                    || curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs) {
                                    if (ImGui::BeginCollapsingSection("Flow seeding", nullptr, ImGuiTreeNodeFlags_DefaultOpen)) {
                                        const std::string label("##flow" + name);
                                        auto format_scale =
                                            "%." + std::to_string(glm::max(-tostf::get_float_exp(curr_case->mesh_scale) + 2, 3))
                                            + "f";
                                        curr_case->flow_renderer.updated =
                                            ImGui::DragFloat3(("Sphere position" + label).c_str(),
                                                &curr_case->flow_renderer.settings.seed_sphere_pos[0],
                                                curr_case->mesh_scale * 0.01f, 0, 0, format_scale.c_str())
                                            || curr_case->flow_renderer.updated;
                                        curr_case->flow_renderer.updated =
                                            ImGui::DragFloat(("Sphere radius" + label).c_str(),
                                                &curr_case->flow_renderer.settings.seed_sphere_radius,
                                                curr_case->mesh_scale * 0.01f, 0, 0, format_scale.c_str())
                                            || curr_case->flow_renderer.updated;
                                        format_scale =
                                            "%." + std::to_string(glm::max(-tostf::get_float_exp(curr_case->mesh_scale) + 4, 3))
                                            + "f";
                                        ImGui::DragFloat(("Flow line radius" + label).c_str(),
                                            &curr_case->flow_renderer.settings.line_radius,
                                            curr_case->mesh_scale * 0.01f, 0, 0, format_scale.c_str());
                                        curr_case->flow_renderer.updated =
                                            ImGui::DragInt(("Line count" + label).c_str(),
                                                &curr_case->flow_renderer.settings.line_count, 1, 0, INT_MAX)
                                            || curr_case->flow_renderer.updated;
                                        if (curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs) {
                                            curr_case->flow_renderer.updated =
                                                ImGui::Checkbox(("Independent of velocity" + label).c_str(),
                                                    &curr_case->flow_renderer.settings.equal_length)
                                                || curr_case->flow_renderer.updated;
                                            curr_case->flow_renderer.updated =
                                                ImGui::DragInt(("Step count" + label).c_str(),
                                                    &curr_case->flow_renderer.settings.step_count, 1, 0, INT_MAX)
                                                || curr_case->flow_renderer.updated;
                                            curr_case->flow_renderer.updated =
                                                ImGui::DragFloat(("Step size factor" + label).c_str(),
                                                    &curr_case->flow_renderer.settings.step_size_factor, 0.0001f, 0, 0,
                                                    "%.7f")
                                                || curr_case->flow_renderer.updated;
                                        }
                                        if (curr_vis == tostf::vis_type::glyphs || curr_vis == tostf::vis_type::pathline_glyphs) {
                                            ImGui::DragInt(("Glyphs per line" + label).c_str(),
                                                &curr_case->flow_renderer.settings.glyphs_per_line, 1, 1, INT_MAX);
                                        }
                                        if (curr_vis == tostf::vis_type::streamlines || curr_vis == tostf::vis_type::glyphs) {
                                            curr_case->flow_renderer.updated =
                                                ImGui::Checkbox(("Activate seeding" + label).c_str(),
                                                    &curr_case->flow_renderer.seeds_set)
                                                || curr_case->flow_renderer.updated;
                                        }
                                        else {
                                            if (curr_case->flow_renderer.pathlines_generated) {
                                                if (ImGui::Button(("Reset generated pathlines" + label).c_str())) {
                                                    curr_case->flow_renderer.pathlines_generated = false;
                                                }
                                            }
                                            else {
                                                ImGui::DragFloat(("Pathline integration time step" + label).c_str(),
                                                    &curr_case->flow_renderer.pathline_settings.step_size, 0.00001f, 0.0001f, 1.0f, "%.5f");
                                                if (ImGui::Button(("Generate pathline from current time step" + label).c_str())) {
                                                    curr_case->flow_renderer.pathlines_generated = true;
                                                    init_pathlines(curr_case);
                                                }
                                            }
                                        }
                                        ImGui::EndCollapsingSection();
                                    }
                                }
                                if (ImGui::BeginCollapsingSection("Timestep export")) {
                                    std::string temp_path = curr_case->export_path.string();
                                    ImGui::InputText(("##exportpath" + name).c_str(), &temp_path, ImGuiInputTextFlags_ReadOnly);
                                    curr_case->export_path = temp_path;
                                    const auto input_clicked = ImGui::IsItemClicked();
                                    ImGui::SameLine();
                                    if (ImGui::Button((ICON_MDI_FILE_EXPORT + ("##" + name)).c_str()) || input_clicked && curr_case
                                        ->
                                        export_path
                                        .empty()
                                        ) {
                                        auto folder_path = std::filesystem::current_path();
                                        if (!curr_case->export_path.empty() && exists(curr_case->export_path)) {
                                            folder_path = curr_case->export_path;
                                        }
                                        auto export_path = tostf::show_select_folder_dialog("Select export folder", folder_path);
                                        if (export_path) {
                                            curr_case->export_path = export_path.value();
                                        }
                                    }
                                    if (ImGui::Button(("Export current timestep##" + name).c_str())) {
                                        curr_case->export_current_step();
                                    }
                                    ImGui::EndCollapsingSection();
                                }
                                if (other_case) {
                                    ImGui::Separator();
                                    if (ImGui::Button("Synch visualization settings\nbetween views")) {
                                        other_case->clipping.clipping_planes = curr_case->clipping.clipping_planes;
                                        auto temp_planes = curr_case->clipping.clipping_planes;
                                        for (auto& c : temp_planes) {
                                            c.plane_normal = normalize(c.plane_normal);
                                        }
                                        other_case->clipping.clipping_ssbo->set_data(temp_planes);
                                        other_case->flow_renderer.settings = curr_case->flow_renderer.settings;
                                        other_case->flow_renderer.pathline_settings = curr_case->flow_renderer.pathline_settings;
                                        other_case->flow_renderer.updated = true;
                                        other_case->flow_renderer.seeds_set = curr_case->flow_renderer.seeds_set;
                                    }
                                }
                            }
                            ImGui::End();
                            ImGui::PopStyleVar();
                        }
                        const auto button_size = ImGui::CalcTextSize(ICON_MDI_SETTINGS);
                        ImGui::SetNextWindowPos(pos + size - ImVec2(0.0f, vis_imgui_settings.bottom_panel_height), 0, ImVec2(1, 1));
                        ImGui::SetNextWindowSize(button_size + ImVec2(15.0f, 15.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                        if (ImGui::Begin(("##vis_settings" + name).c_str(), nullptr, vis_imgui_settings.control_window_flags)) {
                            const auto button_pos = ImGui::GetCursorPos();
                            ImGui::Dummy(button_size);
                            const auto hovered = ImGui::IsItemHovered();
                            if (hovered) {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_ButtonHovered));
                            }
                            else {
                                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_Button));
                            }
                            ImGui::SetCursorPos(button_pos);
                            ImGui::Text(ICON_MDI_SETTINGS);
                            if (ImGui::IsItemClicked()) {
                                clicked = !active;
                            }
                            ImGui::PopStyleColor();
                        }
                        ImGui::PopStyleVar();
                        ImGui::End();
                    }
                    return clicked;
            };

            const auto display_player_controls = [&imgui_style, &vis_imgui_settings]
            (const std::string & name, std::unique_ptr<tostf::vis::Case> & curr_case, const ImVec2 player_pos,
                const float player_width, const float dt) {
                    if (curr_case) {
                        ImGui::SetNextWindowPos(player_pos, 0, ImVec2(0, 1));
                        ImGui::SetNextWindowSize(ImVec2(player_width, vis_imgui_settings.bottom_panel_height));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
                        if (ImGui::Begin(name.c_str(), nullptr, vis_imgui_settings.control_window_flags)) {
                            if (curr_case->player.is_playing()) {
                                if (ImGui::Button((std::string(ICON_MDI_PAUSE "##player") + name).c_str())) {
                                    curr_case->player.pause();
                                }
                                curr_case->player.advance_accumulator(dt);
                            }
                            else {
                                if (ImGui::Button((std::string(ICON_MDI_PLAY "##player") + name).c_str())) {
                                    curr_case->player.play();
                                }
                            }
                            ImGui::SameLine();
                            if (ImGui::Button((std::string(ICON_MDI_SKIP_PREVIOUS "##player") + name).c_str())) {
                                curr_case->player.skip_previous();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button((std::string(ICON_MDI_STOP "##player") + name).c_str())) {
                                curr_case->player.stop();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button((std::string(ICON_MDI_SKIP_NEXT "##player") + name).c_str())) {
                                curr_case->player.skip_next();
                            }
                            ImGui::SameLine();
                            if (curr_case->player.is_repeating()) {
                                if (ImGui::Button((std::string(ICON_MDI_REPEAT "##player") + name).c_str())) {
                                    curr_case->player.toggle_repeat();
                                }
                            }
                            else {
                                if (ImGui::Button((std::string(ICON_MDI_REPEAT_OFF "##player") + name).c_str())) {
                                    curr_case->player.toggle_repeat();
                                }
                            }
                            ImGui::SameLine();
                            auto progress_bar_pos = ImGui::GetCursorPos();
                            const auto progress_bar_height = ImGui::GetTextLineHeight();
                            ImGui::SetCursorPos(ImVec2(progress_bar_pos.x, progress_bar_pos.y + imgui_style.FramePadding.y));
                            ImGui::Text("%s", curr_case->player.get_progress_str().c_str());
                            ImGui::SameLine(0.0f, 5.0f);
                            progress_bar_pos = ImGui::GetCursorPos();
                            ImGui::SetCursorPos(ImVec2(progress_bar_pos.x, progress_bar_pos.y + progress_bar_height - 10.0f));
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
                            //ImVec4(0.53f, 0.53f, 0.56f, 1.0f));
                            ImGui::ProgressBar(curr_case->player.progress(), ImVec2(-1, 10.0f), "");
                            ImGui::PopStyleColor();
                            const auto progress_bar_width = ImGui::GetItemRectSize().x + 16.0f;
                            const auto progress_window_with = ImGui::GetWindowSize().x;
                            ImGui::SameLine(progress_window_with - progress_bar_width);
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
                            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
                            ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
                            ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));
                            ImGui::PushItemWidth(progress_bar_width);
                            const auto imgui_context = ImGui::GetCurrentContext();
                            const auto font_size_backup = imgui_context->FontSize;
                            imgui_context->FontSize = 10.0f - imgui_style.FramePadding.y * 2.0f;
                            ImGui::SliderInt("##player_progress_slider", &curr_case->player.current_step, 0,
                                curr_case->player.get_max_step_id(), "");
                            imgui_context->FontSize = font_size_backup;
                            ImGui::PopItemWidth();
                            ImGui::PopStyleColor(5);
                        }
                        ImGui::End();
                        ImGui::PopStyleVar();
                    }
            };

            auto current_time = std::chrono::high_resolution_clock::now();
            enable(tostf::gl::cap::depth_test);
            while (!window.should_close() && active_tool == 1) {
                if (window.is_minimized()) {
                    window.wait_events();
                    continue;
                }
                auto now = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> elapsed = now - current_time;
                current_time = now;
                const auto dt = elapsed.count();
                tostf::gl::clear_all();
                gui.start_frame();
                if (sim_info.finished.valid() && !tostf::is_future_ready(sim_info.finished)) {
                    auto win_size = window.get_resolution();
                    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
                    ImGui::SetNextWindowSize(ImVec2(win_size.x, win_size.y));
                    if (ImGui::Begin("##Simulation_progress", nullptr, vis_imgui_settings.control_window_flags)) {
                        ImGui::Text("Mesh generation");
                        ImGui::SameLine();
                        auto progress = sim_info.mesh_gen_progress / static_cast<float>(sim_info.case_count);
                        auto progress_str = std::to_string(sim_info.mesh_gen_progress) + " / " + std::to_string(sim_info.case_count);
                        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), progress_str.c_str());

                        ImGui::Text("Simulation");
                        ImGui::SameLine();
                        progress = sim_info.sim_progress / static_cast<float>(sim_info.case_count);
                        progress_str = std::to_string(sim_info.sim_progress) + " / " + std::to_string(sim_info.case_count);
                        ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), progress_str.c_str());
                        if (sim_info.postprocess) {
                            ImGui::Text("Postprocessing");
                            ImGui::SameLine();
                            progress = sim_info.postprocess_progress / static_cast<float>(sim_info.case_count);
                            progress_str = std::to_string(sim_info.postprocess_progress) + " / " + std::to_string(sim_info.case_count);
                            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f), progress_str.c_str());
                        }
                    }
                    ImGui::End();               
                }
                else {
                    int previous_step_ref = 0;
                    int previous_step_comp = 0;
                    const auto previous_vis = visualization_selector.get_current_vis_type();
                    if (window.check_key_action(tostf::win_mgr::key::r, tostf::win_mgr::key_action::press)) {
                        visualization_selector.volume_vis.scalar_to_texture->reload();
                        visualization_selector.volume_vis.volume_ray_casting->reload();
                        visualization_selector.surface_vis.shader->reload();
                        visualization_selector.flow_vis.render_streamlines->reload();
                        visualization_selector.flow_vis.render_surface_front->reload();
                        visualization_selector.flow_vis.render_surface_back->reload();
                        visualization_selector.flow_vis.render_glyphs->reload();
                        visualization_selector.flow_vis.generate_streamlines->reload();
                        visualization_selector.volume_vis.volume_to_plane->reload();
                        visualization_selector.volume_vis.volume_to_surface->reload();
                        visualization_selector.volume_vis.volume_to_plane_phong->reload();
                        visualization_selector.volume_vis.volume_to_surface_phong->reload();
                        visualization_selector.volume_vis.clipped_stencil->reload();
                        visualization_selector.volume_vis.clipped_stencil_planes->reload();
                        visualization_selector.flow_vis.render_sphere->reload();
                        visualization_selector.flow_vis.gen_pathlines_start_step->reload();
                        visualization_selector.flow_vis.gen_pathlines_half_step->reload();
                        visualization_selector.flow_vis.gen_pathlines_integrate->reload();
                        checkerboard_shader->reload();
                    }
                    display_top_panel();
                    if (comparer.has_reference()) {
                        previous_step_ref = comparer.reference->player.current_step;
                    }
                    if (comparer.has_comparing()) {
                        previous_step_comp = comparer.comparing->player.current_step;
                    }
                    const auto ref_case_window_pos = glm::vec2(reference_view.get_pos().x,
                        vis_imgui_settings.top_control_height);
                    auto case_window_size = glm::vec2(reference_view.get_size());
                    case_window_size.y += vis_imgui_settings.bottom_panel_height;
                    if (view_selector.current_view == tostf::vis::vis_view::both) {
                        ImGui::SetNextWindowSize(ImVec2(2.0f, case_window_size.y));
                        ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x + case_window_size.x - 1.0f,
                            vis_imgui_settings.top_control_height));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
                        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetColorU32(ImGuiCol_Border));
                        if (ImGui::Begin("##Seperator", nullptr, vis_imgui_settings.control_window_flags)) {}
                        ImGui::End();
                        ImGui::PopStyleColor();
                        ImGui::PopStyleVar();
                    }
                    if (comparer.has_reference() && case_select_settings.reference_future.valid()
                        && tostf::is_future_ready(case_select_settings.reference_future)) {
                        case_select_settings.reference_future = std::future<void>();
                        finish_case_loading(comparer.reference);
                        previous_step_ref = -1; // force field loading
                    }
                    if (view_selector.current_view == tostf::vis::vis_view::reference_case
                        || view_selector.current_view == tostf::vis::vis_view::both) {
                        if (!comparer.has_reference()) {
                            if (case_select_settings.reference_confirm) {
                                case_select_settings.reference_future =
                                    std::async(std::launch::async, [&comparer, &case_select_settings]() {
                                    comparer.set_reference(case_select_settings.reference_path.parent_path(),
                                        case_select_settings.reference_skip_first);
                                    case_select_settings.reference_path.clear();
                                        });
                                case_select_settings.reference_confirm = false;
                            }
                            if (case_select_settings.reference_future.valid()
                                && !tostf::is_future_ready(case_select_settings.reference_future)) {
                                ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x, ref_case_window_pos.y), 0, ImVec2(0, 0));
                                ImGui::SetNextWindowSize(ImVec2(case_window_size.x, case_window_size.y));
                                if (ImGui::Begin("##loading_reference", nullptr, vis_imgui_settings.control_window_flags)) {
                                    const auto loading_indicator_pos = (case_window_size - glm::vec2(50.0f, 50.0f)) / 2.0f;
                                    ImGui::SetCursorPos(ImVec2(loading_indicator_pos.x, loading_indicator_pos.y));
                                    ImGui::LoadingIndicatorCircle("##ref_loading_circle", 50.0f,
                                        ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered),
                                        ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
                                }
                                ImGui::End();
                            }
                            else {
                                ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x, ref_case_window_pos.y), 0, ImVec2(0, 0));
                                ImGui::SetNextWindowSize(ImVec2(case_window_size.x, case_window_size.y));
                                if (ImGui::Begin("##refcasesel_button", nullptr, vis_imgui_settings.control_window_flags)) {
                                    const auto button_size = ImGui::CalcTextSize("Load reference case");
                                    const auto button_pos = (case_window_size - glm::vec2(button_size.x, button_size.y)) / 2.0f;
                                    ImGui::SetCursorPos(ImVec2(button_pos.x, button_pos.y));
                                    if (ImGui::Button("Load reference case")) {
                                        case_select_settings.show_reference_select = true;
                                    }
                                    ImGui::End();
                                }
                                if (case_select_settings.show_reference_select || !sim_info.dir_path.empty()) {
                                    ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x + case_window_size.x / 2.0f,
                                        ref_case_window_pos.y + case_window_size.y / 2.0f), 0,
                                        ImVec2(0.5f, 0.5f));
                                    ImGui::SetNextWindowSize(ImVec2(case_window_size.x / 2.0f, 200.0f));
                                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                                    if (ImGui::Begin("Reference case selection", nullptr, vis_imgui_settings.case_window_flags)) {
                                        if (sim_info.dir_path.empty()) {
                                            std::string display_path;
                                            if (!case_select_settings.reference_path.empty()) {
                                                display_path = case_select_settings.reference_path.string();
                                            }
                                            ImGui::InputText("##importpathref", &display_path, ImGuiInputTextFlags_ReadOnly);
                                            const auto input_clicked = ImGui::IsItemClicked();
                                            ImGui::SameLine();
                                            if (ImGui::Button(ICON_MDI_FILE_IMPORT "##ref") || input_clicked && display_path.empty()) {
                                                auto folder_path = std::filesystem::current_path();
                                                if (!display_path.empty() && std::filesystem::exists(display_path)) {
                                                    folder_path = display_path;
                                                }
                                                auto load_case_path = tostf::show_open_file_dialog(
                                                    "Select case file", folder_path, { "*.foam" });
                                                if (load_case_path) {
                                                    case_select_settings.reference_path = load_case_path.value();
                                                }
                                            }
                                        }
                                        else {
                                            if (ImGui::BeginCombo("##comboreference", case_select_settings.reference_path.string().c_str())) {
                                                for (auto& f : std::filesystem::directory_iterator(sim_info.dir_path)) {
                                                    if (is_directory(f)) {
                                                        auto filename = f.path().filename().string() + ".foam";
                                                        auto curr_path = f.path() / filename;
                                                        bool is_selected = case_select_settings.reference_path == curr_path;
                                                        if (ImGui::Selectable(curr_path.string().c_str(), is_selected)) {
                                                            case_select_settings.reference_path = curr_path;
                                                        }
                                                        if (is_selected) {
                                                            ImGui::SetItemDefaultFocus();
                                                        }
                                                    }
                                                }
                                                ImGui::EndCombo();
                                            }
                                        }
                                        ImGui::Checkbox("Skip first timestep", &case_select_settings.reference_skip_first);
                                        if (ImGui::Button("Select case")) {
                                            if (!case_select_settings.reference_path.empty()) {
                                                case_select_settings.reference_confirm = true;
                                                case_select_settings.show_reference_select = false;
                                            }
                                        }
                                    }
                                    ImGui::End();
                                    ImGui::PopStyleVar();
                                }
                            }
                        }
                        else {
                            ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x, ref_case_window_pos.y), 0, ImVec2(0, 0));
                            ImGui::SetNextWindowSize(ImVec2(case_window_size.x, 0.0f));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                            bool keep_view = true;
                            const auto window_name = comparer.reference->path.lexically_relative(
                                comparer.reference->path.parent_path().parent_path()).string();
                            if (ImGui::Begin((window_name + "##ref_path").c_str(), &keep_view,
                                vis_imgui_settings.view_window_flags)) {
                            }
                            ImGui::End();
                            if (!keep_view) {
                                comparer.remove_reference();
                            }
                            ImGui::PopStyleVar(2);
                            display_player_controls("refplayer", comparer.reference,
                                ImVec2(ref_case_window_pos.x, ref_case_window_pos.y + case_window_size.y),
                                case_window_size.x, dt);
                            case_windows.display_vis_setting_ref = display_vis_settings(
                                comparer.reference, comparer.comparing, "ref", ImVec2(ref_case_window_pos.x, ref_case_window_pos.y),
                                ImVec2(case_window_size.x, case_window_size.y), case_windows.display_vis_setting_ref);
                        }
                    }
                    const auto comp_case_window_pos = glm::vec2(comparing_view.get_pos().x,
                        vis_imgui_settings.top_control_height);
                    if (comparer.has_comparing() && case_select_settings.comparison_future.valid()
                        && tostf::is_future_ready(case_select_settings.comparison_future)) {
                        case_select_settings.comparison_future = std::future<void>();
                        finish_case_loading(comparer.comparing);
                        previous_step_comp = -1; //force field loading
                    }
                    if (view_selector.current_view == tostf::vis::vis_view::comparing_case
                        || view_selector.current_view == tostf::vis::vis_view::both) {
                        case_window_size = glm::vec2(comparing_view.get_size());
                        case_window_size.y += vis_imgui_settings.bottom_panel_height;
                        if (!comparer.has_comparing()) {
                            if (case_select_settings.comparison_confirm) {
                                case_select_settings.comparison_future =
                                    std::async(std::launch::async, [&comparer, &case_select_settings]() {
                                    comparer.set_comparing_case(case_select_settings.comparison_path.parent_path(),
                                        case_select_settings.comparison_skip_first);
                                    case_select_settings.comparison_path.clear();
                                        });
                                case_select_settings.comparison_confirm = false;
                            }
                            if (case_select_settings.comparison_future.valid()
                                && !tostf::is_future_ready(case_select_settings.comparison_future)) {
                                ImGui::SetNextWindowPos(ImVec2(comp_case_window_pos.x, comp_case_window_pos.y), 0, ImVec2(0, 0));
                                ImGui::SetNextWindowSize(ImVec2(case_window_size.x, case_window_size.y));
                                if (ImGui::Begin("##loading_comparison", nullptr, vis_imgui_settings.control_window_flags)) {
                                    const auto loading_indicator_pos = (case_window_size - glm::vec2(50.0f, 50.0f)) / 2.0f;
                                    ImGui::SetCursorPos(ImVec2(loading_indicator_pos.x, loading_indicator_pos.y));
                                    ImGui::LoadingIndicatorCircle("##comp_loading_circle", 50.0f,
                                        ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered),
                                        ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
                                }
                                ImGui::End();
                            }
                            else {
                                ImGui::SetNextWindowPos(ImVec2(comp_case_window_pos.x, comp_case_window_pos.y), 0, ImVec2(0, 0));
                                ImGui::SetNextWindowSize(ImVec2(case_window_size.x, case_window_size.y));
                                if (ImGui::Begin("##compcasesel_button", nullptr, vis_imgui_settings.control_window_flags)) {
                                    const auto button_size = ImGui::CalcTextSize("Load comparing case");
                                    const auto button_pos = (case_window_size - glm::vec2(button_size.x, button_size.y)) / 2.0f;
                                    ImGui::SetCursorPos(ImVec2(button_pos.x, button_pos.y));
                                    if (ImGui::Button("Load comparing case")) {
                                        case_select_settings.show_comparison_select = true;
                                    }
                                }
                                ImGui::End();
                                if (case_select_settings.show_comparison_select || !sim_info.dir_path.empty()) {
                                    ImGui::SetNextWindowPos(ImVec2(comp_case_window_pos.x + case_window_size.x / 2.0f,
                                        comp_case_window_pos.y + case_window_size.y / 2.0f), 0,
                                        ImVec2(0.5f, 0.5f));
                                    ImGui::SetNextWindowSize(ImVec2(case_window_size.x / 2.0f, 200.0f));
                                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                                    if (ImGui::Begin("Comparison case selection", nullptr, vis_imgui_settings.case_window_flags)) {
                                        if (sim_info.dir_path.empty()) {
                                            std::string display_path;
                                            if (!case_select_settings.comparison_path.empty()) {
                                                display_path = case_select_settings.comparison_path.string();
                                            }
                                            ImGui::InputText("##importpathcomp", &display_path, ImGuiInputTextFlags_ReadOnly);
                                            const auto input_clicked = ImGui::IsItemClicked();
                                            ImGui::SameLine();
                                            if (ImGui::Button(ICON_MDI_FILE_IMPORT "##comp") || input_clicked && display_path.empty()) {
                                                auto folder_path = std::filesystem::current_path();
                                                if (!display_path.empty() && std::filesystem::exists(display_path)) {
                                                    folder_path = display_path;
                                                }
                                                auto load_case_path = tostf::show_open_file_dialog(
                                                    "Select case file", folder_path, { "*.foam" });
                                                if (load_case_path) {
                                                    case_select_settings.comparison_path = load_case_path.value();
                                                }
                                            }
                                        }
                                        else {
                                            if (ImGui::BeginCombo("##combocomparison", case_select_settings.comparison_path.string().c_str())) {
                                                for (auto& f : std::filesystem::directory_iterator(sim_info.dir_path)) {
                                                    if (is_directory(f)) {
                                                        auto filename = f.path().filename().string() + ".foam";
                                                        auto curr_path = f.path() / filename;
                                                        bool is_selected = case_select_settings.comparison_path == curr_path;
                                                        if (ImGui::Selectable(curr_path.string().c_str(), is_selected)) {
                                                            case_select_settings.comparison_path = curr_path;
                                                        }
                                                        if (is_selected) {
                                                            ImGui::SetItemDefaultFocus();
                                                        }
                                                    }
                                                }
                                                ImGui::EndCombo();
                                            }
                                        }
                                        ImGui::Checkbox("Skip first timestep", &case_select_settings.comparison_skip_first);
                                        if (ImGui::Button("Select case")) {
                                            if (!case_select_settings.comparison_path.empty()) {
                                                case_select_settings.show_comparison_select = false;
                                                case_select_settings.comparison_confirm = true;
                                            }
                                        }
                                    }
                                    ImGui::End();
                                    ImGui::PopStyleVar();
                                }
                            }
                        }
                        else {
                            ImGui::SetNextWindowPos(ImVec2(comp_case_window_pos.x, comp_case_window_pos.y), 0, ImVec2(0, 0));
                            ImGui::SetNextWindowSize(ImVec2(case_window_size.x, 0.0f));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
                            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                            bool keep_view = true;
                            const auto window_name = comparer.comparing->path.lexically_relative(
                                comparer.comparing->path.parent_path().parent_path()).string();
                            if (ImGui::Begin((window_name + "##comp_path").c_str(), &keep_view,
                                vis_imgui_settings.view_window_flags)) {
                            }
                            ImGui::End();
                            if (!keep_view) {
                                comparer.remove_comparing_case();
                            }
                            ImGui::PopStyleVar(2);
                            display_player_controls("compplayer", comparer.comparing,
                                ImVec2(comp_case_window_pos.x, comp_case_window_pos.y + case_window_size.y),
                                case_window_size.x, dt);
                            case_windows.display_vis_setting_comp = display_vis_settings(
                                comparer.comparing, comparer.reference, "comp",
                                ImVec2(comp_case_window_pos.x, comp_case_window_pos.y),
                                ImVec2(case_window_size.x, case_window_size.y), case_windows.display_vis_setting_comp);
                        }
                    }
                    if (view_selector.current_view == tostf::vis::vis_view::both) {
                        display_bottom_panel();
                    }

                    if (comparer.has_reference()) {
                        if (previous_step_ref != comparer.reference->player.current_step
                            || previous_vis != visualization_selector.get_current_vis_type()
                            || comparer.reference->flow_renderer.updated) {
                            if (visualization_selector.get_current_vis_type() == tostf::vis_type::streamlines
                                || visualization_selector.get_current_vis_type() == tostf::vis_type::glyphs) {
                                comparer.reference->flow_renderer.updated = false;
                                init_streamlines(comparer.reference);
                            }
                            if (comparer.selected_field != -1) {
                                load_field(comparer.reference, comparer.get_field_name(comparer.selected_field));
                            }
                        }
                    }
                    if (comparer.has_comparing()) {
                        if (previous_step_comp != comparer.comparing->player.current_step
                            || previous_vis != visualization_selector.get_current_vis_type()
                            || comparer.comparing->flow_renderer.updated) {
                            if (visualization_selector.get_current_vis_type() == tostf::vis_type::streamlines
                                || visualization_selector.get_current_vis_type() == tostf::vis_type::glyphs) {
                                comparer.comparing->flow_renderer.updated = false;
                                init_streamlines(comparer.comparing);
                            }
                            if (comparer.selected_field != -1) {
                                load_field(comparer.comparing, comparer.get_field_name(comparer.selected_field));
                            }
                        }
                    }
                    cam->update(window, dt);
                    if (comparer.reference && (view_selector.current_view == tostf::vis::vis_view::reference_case
                        || view_selector.current_view == tostf::vis::vis_view::both)) {
                        vis_display_legend(ImVec2(ref_case_window_pos.x + 20.0f,
                            ref_case_window_pos.y + case_window_size.y - comparer.legend.height - 40.0f
                            - vis_imgui_settings.bottom_panel_height), "ref");
                        reference_view.activate();
                        auto view_vp = reference_view.get_viewport();
                        if (case_windows.display_vis_setting_ref) {
                            view_vp.width -= static_cast<int>(vis_imgui_settings.vis_settings_window_width);
                            set_viewport(view_vp);
                        }
                        cam->resize();
                        visualization_selector.update_camera(cam->get_view(), cam->get_projection());
                        render_visualization(comparer.reference);
                        if (comparer.reference->overlay_settings.active
                            && view_selector.current_view == tostf::vis::vis_view::reference_case
                            && comparer.comparing) {
                            tostf::gl::set_viewport(reference_view.get_size().x, reference_view.get_size().y);
                            reference_fbo->bind();
                            tostf::gl::clear_all();
                            render_visualization(comparer.comparing);
                            tostf::FBO::unbind();
                            set_viewport(view_vp);
                            render_overlay(reference_texture, comparer.reference->overlay_settings);
                        }
                    }
                    if (comparer.comparing && (view_selector.current_view == tostf::vis::vis_view::comparing_case
                        || view_selector.current_view == tostf::vis::vis_view::both)) {
                        vis_display_legend(ImVec2(comp_case_window_pos.x + 20.0f,
                            comp_case_window_pos.y + case_window_size.y - comparer.legend.height - 40.0f
                            - vis_imgui_settings.bottom_panel_height), "comp");
                        comparing_view.activate();
                        auto view_vp = comparing_view.get_viewport();
                        if (case_windows.display_vis_setting_comp) {
                            view_vp.width -= static_cast<int>(vis_imgui_settings.vis_settings_window_width);
                            set_viewport(view_vp);
                        }
                        cam->resize();
                        visualization_selector.update_camera(cam->get_view(), cam->get_projection());
                        render_visualization(comparer.comparing);
                        if (comparer.comparing->overlay_settings.active
                            && view_selector.current_view == tostf::vis::vis_view::comparing_case
                            && comparer.reference) {
                            tostf::gl::set_viewport(comparing_view.get_size().x, reference_view.get_size().y);
                            comparing_fbo->bind();
                            tostf::gl::clear_all();
                            render_visualization(comparer.reference);
                            tostf::FBO::unbind();
                            set_viewport(view_vp);
                            render_overlay(comparing_texture, comparer.comparing->overlay_settings);
                        }
                    }
                }
                main_view.activate();
                gui.render();
                window.swap_buffers();
                //gl_debug_logger.retrieve_log();
                window.poll_events();
            }
        }
    }
    tostf::win_mgr::terminate();
    return 0;
}
