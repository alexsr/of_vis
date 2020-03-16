//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "utility/logging.hpp"
#include "display/window.hpp"
#include "display/gui.hpp"
#include "gpu_interface/debug.hpp"
#include "core/shader_settings.hpp"
#include "foam_processing/foam_loader.hpp"
#include "file/file_handling.hpp"
#include "core/gpu_interface.hpp"
#include "geometry/mesh_handling.hpp"
#include "gpu_interface/storage_buffer.hpp"
#include "iconfont/IconsMaterialDesignIcons.h"
#include "display/tostf_imgui_widgets.hpp"
#include "gpu_interface/fbo.hpp"
#include "visualization/control.hpp"
#include "display/view.hpp"
#include "imgui/imgui_internal.h"
#include "visualization/visualization.hpp"

struct Imgui_settings {
    int bottom_panel_height = 50;
    int top_control_height = 40;
    float field_box_width = 150.0f;
    float view_box_width = 200.0f;
    float colormap_box_width = 300.0f;
    float colormap_image_width = 256.0f;
    float vis_settings_window_width = 300.0f;
    bool dark_mode = false;
    ImGuiWindowFlags case_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize
                                         | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove;
    ImGuiWindowFlags control_window_flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration |
                                            ImGuiWindowFlags_NoBringToFrontOnFocus |
                                            ImGuiWindowFlags_NoFocusOnAppearing;
    ImGuiWindowFlags view_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize
                                         | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove
                                         | ImGuiWindowFlags_NoBringToFrontOnFocus
                                         | ImGuiWindowFlags_NoFocusOnAppearing;
};

struct Case_selection_settings {
    bool show_reference_select = false;
    bool show_comparison_select = false;
    std::filesystem::path reference_path;
    std::filesystem::path comparison_path;
    bool reference_skip_first = true;
    bool comparison_skip_first = true;
    bool reference_confirm = false;
    bool comparison_confirm = false;
    std::future<void> reference_future = std::future<void>();
    std::future<void> comparison_future = std::future<void>();
};

struct Case_imgui_windows {
    bool display_vis_setting_ref = false;
    bool display_vis_setting_comp = false;
};

auto main() -> int {
    tostf::cmd::enable_color();
    const tostf::window_config win_conf{
        "Visualization tool", 1600, 900, 1, tostf::win_mgr::default_window_flags
    };
    const tostf::gl_settings gl_config{
        4, 6, true, tostf::gl_profile::core, 4,
        {1, 1, 1, 1}, {tostf::gl::cap::depth_test}
    };
    tostf::Window window(win_conf, gl_config);
    window.set_minimum_size({1200, 800});
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
        tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
    }
    //tostf::gl::Debug_logger gl_debug_logger;
    //gl_debug_logger.disable_severity(tostf::gl::dbg::severity::notification);
    Imgui_settings imgui_settings;
    Case_selection_settings case_select_settings;
    Case_imgui_windows case_windows;
    auto& imgui_style = ImGui::GetStyle();
    imgui_style.WindowBorderSize = 0.0f;
    const float cam_near = 0.001f;
    auto checkerboard_shader = std::make_unique<tostf::V_F_shader>("../../shader/rendering/screenfilling_quad.vert",
                                                                   "../../shader/rendering/checkerboard_texture.frag");
    auto compositing_shader = std::make_unique<tostf::V_F_shader>("../../shader/rendering/screenfilling_quad.vert",
                                                                  "../../shader/rendering/texture_minimal.frag");
    tostf::Trackball_camera cam(1.0f, 0.2f, 0.1f, true, glm::vec3(0.0f), 60.0f, cam_near);
    const auto view_size = tostf::ui_size(window.get_resolution().x / 2, window.get_resolution().y
                                                                         - 2 * imgui_settings.bottom_panel_height
                                                                         - imgui_settings.top_control_height);
    tostf::View reference_view(tostf::ui_pos(0, 2 * imgui_settings.bottom_panel_height), view_size);
    auto reference_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{reference_view.get_size(), 1.0f});
    auto reference_fbo = std::make_unique<tostf::FBO>(reference_texture->get_res(),
                                                      std::vector<tostf::FBO_texture_def>{
                                                          reference_texture->get_fbo_tex_def()
                                                      },
                                                      std::vector<tostf::FBO_renderbuffer_def>{
                                                          tostf::FBO_renderbuffer_def{
                                                              tostf::tex::intern::depth32f_stencil8, false, 0
                                                          }
                                                      });
    tostf::View comparing_view(tostf::ui_pos(view_size.x, 2 * imgui_settings.bottom_panel_height), view_size);
    auto comparing_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{comparing_view.get_size(), 1.0f});
    auto comparing_fbo = std::make_unique<tostf::FBO>(comparing_texture->get_res(),
                                                      std::vector<tostf::FBO_texture_def>{
                                                          comparing_texture->get_fbo_tex_def()
                                                      },
                                                      std::vector<tostf::FBO_renderbuffer_def>{
                                                          tostf::FBO_renderbuffer_def{
                                                              tostf::tex::intern::depth32f_stencil8, false, 0
                                                          }
                                                      });
    tostf::View main_view(window.get_resolution());
    const auto resize_window = [&](
        tostf::win_mgr::raw_win_ptr, const int x, const int y) {
        main_view.resize({x, y});
        const auto new_view_size = tostf::ui_size(x / 2, y - 2 * imgui_settings.bottom_panel_height
                                                         - imgui_settings.top_control_height);
        reference_view.resize(new_view_size);
        comparing_view.resize(new_view_size);
        reference_view.set_pos({0, 2 * imgui_settings.bottom_panel_height});
        comparing_view.set_pos({new_view_size.x, 2 * imgui_settings.bottom_panel_height});
        if (x != 0 && y != 0) {
            reference_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{reference_view.get_size(), 1.0f});
            reference_fbo = std::make_unique<tostf::FBO>(reference_texture->get_res(),
                                                         std::vector<tostf::FBO_texture_def>{
                                                             reference_texture->get_fbo_tex_def()
                                                         },
                                                         std::vector<tostf::FBO_renderbuffer_def>{
                                                             tostf::FBO_renderbuffer_def{
                                                                 tostf::tex::intern::depth32f_stencil8, false, 0
                                                             }
                                                         });
            comparing_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{comparing_view.get_size(), 1.0f});
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
        main_view.resize({x, y});
        reference_view.resize({x, y - imgui_settings.bottom_panel_height - imgui_settings.top_control_height});
        reference_view.set_pos({0, imgui_settings.bottom_panel_height});
        if (x != 0 && y != 0) {
            reference_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{reference_view.get_size(), 1.0f});
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
        main_view.resize({x, y});
        comparing_view.resize({x, y - imgui_settings.bottom_panel_height - imgui_settings.top_control_height});
        comparing_view.set_pos({0, imgui_settings.bottom_panel_height});
        if (x != 0 && y != 0) {
            comparing_texture = std::make_unique<tostf::Texture2D>(tostf::tex_res{comparing_view.get_size(), 1.0f});
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
    tostf::View_selector view_selector;
    tostf::Visualization_selector visualization_selector;
    tostf::Case_comparer comparer;
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

    const auto render_overlay = [&](const std::unique_ptr<tostf::Texture2D>& texture,
                                    const tostf::Overlay_settings& settings) {
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

    const auto render_visualization = [&](const std::unique_ptr<tostf::Case>& curr_case) {
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

    const auto integrate_pathlines = [&](const std::unique_ptr<tostf::Case>& curr_case, const int curr_step) {
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
            if (imgui_settings.dark_mode) {
                tostf::gl::set_clear_color(0.3f, 0.3f, 0.3f, 1);
            }
            else {
                tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
            }
        }
    };

    const auto init_pathlines = [&](const std::unique_ptr<tostf::Case>& curr_case) {
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

    const auto init_streamlines = [&](const std::unique_ptr<tostf::Case>& curr_case) {
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
                if (imgui_settings.dark_mode) {
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

    const auto load_field = [&](const std::unique_ptr<tostf::Case>& curr_case, const std::string& field_name) {
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
                || curr_vis == tostf::vis_type::pathlines || curr_vis == tostf::vis_type::pathline_glyphs ) {
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
                if (imgui_settings.dark_mode) {
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

    const auto finish_case_loading = [&](const std::unique_ptr<tostf::Case>& curr_case) {
        const auto current_cam_radius = length(glm::vec3(curr_case->mesh.mesh_bb.max
                                                         - curr_case->mesh.mesh_bb.min)) / 2.0f;
        cam.set_radius(current_cam_radius);
        cam.set_near(curr_case->mesh_scale * 0.1f);
        cam.set_move_speed(current_cam_radius);
        curr_case->init_renderers();
    };

    const auto display_legend = [&](const ImVec2& pos, const std::string& name) {
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
            if (!imgui_settings.dark_mode) {
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
            if (!imgui_settings.dark_mode) {
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
        ImGui::PopStyleVar(5);
    };

    const auto display_top_panel = [&]() {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x, imgui_settings.top_control_height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 5.0f));
        const auto eye_symbol_size = ImGui::CalcTextSize(ICON_MDI_EYE);
        if (ImGui::Begin("##toppanel", nullptr, imgui_settings.control_window_flags)) {
            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, window.get_resolution().x - imgui_settings.view_box_width
                                     - imgui_style.FramePadding.x * 5.0f - eye_symbol_size.x);
            ImGui::SetColumnWidth(
                1, imgui_settings.view_box_width + imgui_style.FramePadding.x * 5.0f + eye_symbol_size.x);
            ImGui::PushItemWidth(imgui_settings.view_box_width);
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
            ImGui::PushItemWidth(imgui_settings.field_box_width);
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
            ImGui::PushItemWidth(imgui_settings.colormap_box_width);
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
                    ImGui::SameLine(imgui_settings.colormap_box_width + font_width * color_maps.max_name_length
                                    - imgui_settings.colormap_image_width);
                    ImGui::Image(ImGui::ConvertGLuintToImTex(m_it->second.tex->get_name()),
                                 ImVec2(imgui_settings.colormap_image_width, image_height));
                }
                ImGui::EndCombo();
            }
            ImGui::SetCursorPos(ImVec2(combo_pos.x + imgui_style.FramePadding.x,
                                       combo_pos.y + imgui_style.FramePadding.y));
            ImGui::Image(ImGui::ConvertGLuintToImTex(color_maps.curr_map_it->second.tex->get_name()),
                         ImVec2(imgui_settings.colormap_image_width, image_height));
            ImGui::PopItemWidth();
            ImGui::SetCursorPos(ImVec2(combo_pos.x + imgui_settings.colormap_box_width, combo_pos.y));
            ImGui::SameLine(0, imgui_settings.colormap_box_width
                               - imgui_settings.colormap_image_width + imgui_style.ItemSpacing.x);
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
            if (imgui_settings.dark_mode) {
                dark_mode_str = "Deactivate dark mode";
            }
            if (ImGui::Button(dark_mode_str.c_str())) {
                imgui_settings.dark_mode = !imgui_settings.dark_mode;
                if (imgui_settings.dark_mode) {
                    tostf::gl::set_clear_color(0.3f, 0.3f, 0.3f, 1);
                }
                else {
                    tostf::gl::set_clear_color(1.0f, 1.0f, 1.0f, 1);
                }
            }
            ImGui::NextColumn();
            ImGui::PushItemWidth(imgui_settings.view_box_width);
            if (ImGui::BeginCombo(ICON_MDI_EYE "##comboselectview", to_string(view_selector.current_view).c_str())) {
                for (int i = 0; i < to_underlying_type(tostf::vis_view::count); ++i) {
                    const bool is_selected = to_underlying_type(view_selector.current_view) == i;
                    if (ImGui::Selectable(to_string(static_cast<tostf::vis_view>(i)).c_str(), is_selected)) {
                        view_selector.current_view = static_cast<tostf::vis_view>(i);
                        if (view_selector.current_view == tostf::vis_view::comparing_case) {
                            window.add_resize_fun("window", resize_window_comp);
                            resize_window_comp(window.get()->get(), window.get_resolution().x,
                                               window.get_resolution().y);
                        }
                        else if (view_selector.current_view == tostf::vis_view::reference_case) {
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
        ImGui::SetNextWindowPos(ImVec2(0, window.get_resolution().y - imgui_settings.bottom_panel_height));
        ImGui::SetNextWindowSize(ImVec2(window.get_resolution().x, imgui_settings.bottom_panel_height));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
        const auto player_size = ImGui::CalcTextSize(ICON_MDI_PAUSE ICON_MDI_SKIP_PREVIOUS
                                     ICON_MDI_SKIP_NEXT ICON_MDI_STOP ICON_MDI_REPEAT).x
                                 + imgui_style.ItemSpacing.x * 6.0f + imgui_style.FramePadding.x * 5.0f;
        if (ImGui::Begin("##bottom_panel", nullptr, imgui_settings.control_window_flags)) {
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

    const auto display_vis_settings = [&](std::unique_ptr<tostf::Case>& curr_case,
                                          std::unique_ptr<tostf::Case>& other_case, const std::string& name,
                                          const ImVec2& pos, const ImVec2& size, const bool active) {
        bool clicked = active;
        if (curr_case) {
            const auto curr_vis = visualization_selector.get_current_vis_type();
            if (active) {
                const auto offset_size = ImVec2(size.x - imgui_settings.vis_settings_window_width,
                                                ImGui::CalcTextSize("A").y + imgui_style.FramePadding.y * 2.0f);
                ImGui::SetNextWindowPos(pos + offset_size, 0, ImVec2(0, 0));
                ImGui::SetNextWindowSize(ImVec2(imgui_settings.vis_settings_window_width,
                                                size.y - offset_size.y - imgui_settings.bottom_panel_height));
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                if (ImGui::Begin(("Visualization settings##" + name).c_str(), &clicked,
                                 imgui_settings.control_window_flags)) {
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
                    if (view_selector.current_view != tostf::vis_view::both) {
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
            ImGui::SetNextWindowPos(pos + size - ImVec2(0.0f, imgui_settings.bottom_panel_height), 0, ImVec2(1, 1));
            ImGui::SetNextWindowSize(button_size + ImVec2(15.0f, 15.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
            if (ImGui::Begin(("##vis_settings" + name).c_str(), nullptr, imgui_settings.control_window_flags)) {
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

    const auto display_player_controls = [&imgui_style, &imgui_settings]
    (const std::string& name, std::unique_ptr<tostf::Case>& curr_case, const ImVec2 player_pos,
     const float player_width, const float dt) {
        if (curr_case) {
            ImGui::SetNextWindowPos(player_pos, 0, ImVec2(0, 1));
            ImGui::SetNextWindowSize(ImVec2(player_width, imgui_settings.bottom_panel_height));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
            if (ImGui::Begin(name.c_str(), nullptr, imgui_settings.control_window_flags)) {
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
    while (!window.should_close()) {
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
                                                   imgui_settings.top_control_height);
        auto case_window_size = glm::vec2(reference_view.get_size());
        case_window_size.y += imgui_settings.bottom_panel_height;
        if (view_selector.current_view == tostf::vis_view::both) {
            ImGui::SetNextWindowSize(ImVec2(2.0f, case_window_size.y));
            ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x + case_window_size.x - 1.0f,
                                           imgui_settings.top_control_height));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetColorU32(ImGuiCol_Border));
            if (ImGui::Begin("##Seperator", nullptr, imgui_settings.control_window_flags)) { }
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
        if (view_selector.current_view == tostf::vis_view::reference_case
            || view_selector.current_view == tostf::vis_view::both) {
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
                    if (ImGui::Begin("##loading_reference", nullptr, imgui_settings.control_window_flags)) {
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
                    if (ImGui::Begin("##refcasesel_button", nullptr, imgui_settings.control_window_flags)) {
                        const auto button_size = ImGui::CalcTextSize("Load reference case");
                        const auto button_pos = (case_window_size - glm::vec2(button_size.x, button_size.y)) / 2.0f;
                        ImGui::SetCursorPos(ImVec2(button_pos.x, button_pos.y));
                        if (ImGui::Button("Load reference case")) {
                            case_select_settings.show_reference_select = true;
                        }
                    }
                    ImGui::End();
                    if (case_select_settings.show_reference_select) {
                        ImGui::SetNextWindowPos(ImVec2(ref_case_window_pos.x + case_window_size.x / 2.0f,
                                                       ref_case_window_pos.y + case_window_size.y / 2.0f), 0,
                                                ImVec2(0.5f, 0.5f));
                        ImGui::SetNextWindowSize(ImVec2(case_window_size.x / 2.0f, 200.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                        if (ImGui::Begin("Reference case selection", nullptr, imgui_settings.case_window_flags)) {
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
                                    "Select case file", folder_path, {"*.foam"});
                                if (load_case_path) {
                                    case_select_settings.reference_path = load_case_path.value();
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
                                 imgui_settings.view_window_flags)) { }
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
                                                    imgui_settings.top_control_height);
        if (comparer.has_comparing() && case_select_settings.comparison_future.valid()
            && tostf::is_future_ready(case_select_settings.comparison_future)) {
            case_select_settings.comparison_future = std::future<void>();
            finish_case_loading(comparer.comparing);
            previous_step_comp = -1; //force field loading
        }
        if (view_selector.current_view == tostf::vis_view::comparing_case
            || view_selector.current_view == tostf::vis_view::both) {
            case_window_size = glm::vec2(comparing_view.get_size());
            case_window_size.y += imgui_settings.bottom_panel_height;
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
                    if (ImGui::Begin("##loading_comparison", nullptr, imgui_settings.control_window_flags)) {
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
                    if (ImGui::Begin("##compcasesel_button", nullptr, imgui_settings.control_window_flags)) {
                        const auto button_size = ImGui::CalcTextSize("Load comparing case");
                        const auto button_pos = (case_window_size - glm::vec2(button_size.x, button_size.y)) / 2.0f;
                        ImGui::SetCursorPos(ImVec2(button_pos.x, button_pos.y));
                        if (ImGui::Button("Load comparing case")) {
                            case_select_settings.show_comparison_select = true;
                        }
                    }
                    ImGui::End();
                    if (case_select_settings.show_comparison_select) {
                        ImGui::SetNextWindowPos(ImVec2(comp_case_window_pos.x + case_window_size.x / 2.0f,
                                                       comp_case_window_pos.y + case_window_size.y / 2.0f), 0,
                                                ImVec2(0.5f, 0.5f));
                        ImGui::SetNextWindowSize(ImVec2(case_window_size.x / 2.0f, 200.0f));
                        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
                        if (ImGui::Begin("Comparison case selection", nullptr, imgui_settings.case_window_flags)) {
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
                                    "Select case file", folder_path, {"*.foam"});
                                if (load_case_path) {
                                    case_select_settings.comparison_path = load_case_path.value();
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
                                 imgui_settings.view_window_flags)) { }
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
        if (view_selector.current_view == tostf::vis_view::both) {
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
        cam.update(window, dt);
        if (comparer.reference && (view_selector.current_view == tostf::vis_view::reference_case
                                   || view_selector.current_view == tostf::vis_view::both)) {
            display_legend(ImVec2(ref_case_window_pos.x + 20.0f,
                                  ref_case_window_pos.y + case_window_size.y - comparer.legend.height - 40.0f
                                  - imgui_settings.bottom_panel_height), "ref");
            reference_view.activate();
            auto view_vp = reference_view.get_viewport();
            if (case_windows.display_vis_setting_ref) {
                view_vp.width -= static_cast<int>(imgui_settings.vis_settings_window_width);
                set_viewport(view_vp);
            }
            cam.resize();
            visualization_selector.update_camera(cam.get_view(), cam.get_projection());
            render_visualization(comparer.reference);
            if (comparer.reference->overlay_settings.active
                && view_selector.current_view == tostf::vis_view::reference_case
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
        if (comparer.comparing && (view_selector.current_view == tostf::vis_view::comparing_case
                                   || view_selector.current_view == tostf::vis_view::both)) {
            display_legend(ImVec2(comp_case_window_pos.x + 20.0f,
                                  comp_case_window_pos.y + case_window_size.y - comparer.legend.height - 40.0f
                                  - imgui_settings.bottom_panel_height), "comp");
            comparing_view.activate();
            auto view_vp = comparing_view.get_viewport();
            if (case_windows.display_vis_setting_comp) {
                view_vp.width -= static_cast<int>(imgui_settings.vis_settings_window_width);
                set_viewport(view_vp);
            }
            cam.resize();
            visualization_selector.update_camera(cam.get_view(), cam.get_projection());
            render_visualization(comparer.comparing);
            if (comparer.comparing->overlay_settings.active
                && view_selector.current_view == tostf::vis_view::comparing_case
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
        main_view.activate();
        gui.render();
        window.swap_buffers();
        //gl_debug_logger.retrieve_log();
        window.poll_events();
    }
    tostf::win_mgr::terminate();
    return 0;
}
