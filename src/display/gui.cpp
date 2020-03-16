//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "gui.hpp"
#include <glad/glad.h>
#include <cassert>
#include <utility>
#include <iconfont/IconsMaterialDesignIcons.h>

tostf::Gui::Gui(std::shared_ptr<win_mgr::win_obj> window, const float font_size, const bool install_callbacks_flag,
                const std::filesystem::path& vertex_path, const std::filesystem::path& fragment_path)
    : _window(std::move(window)), _time(0) {
    _gui_shader = std::make_unique<V_F_shader>(vertex_path, fragment_path);
    _mouse_button_pressed = std::make_unique<std::array<bool, 5>>();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    const auto size = _window->get_res();
    const auto framebuffer_size = _window->get_framebuffer_size();
    io.DisplaySize = ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
    io.DisplayFramebufferScale = ImVec2(size.x > 0 ? static_cast<float>(framebuffer_size.x) / size.x : 0,
                                        size.y > 0 ? static_cast<float>(framebuffer_size.y) / size.y : 0);
    io.Fonts->Clear();

    ImFontConfig config;
    config.OversampleH = 6;
    config.OversampleV = 3;
    io.Fonts->AddFontFromFileTTF("../../resources/fonts/OpenSans-Regular.ttf", font_size, &config);
    const std::array<ImWchar, 3> icons_range{ICON_MIN_MDI, ICON_MAX_MDI, 0};
    ImFontConfig icons_config;
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("../../resources/fonts/" FONT_ICON_FILE_NAME_MDI, font_size, &icons_config,
                                 icons_range.data());
    io.Fonts->AddFontFromFileTTF("../../resources/fonts/OpenSans-Regular.ttf", 18.0f, &config);
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("../../resources/fonts/" FONT_ICON_FILE_NAME_MDI, 18.0f, &icons_config,
        icons_range.data());
    io.Fonts->Build();
    stbi_uc* pixel_ptr;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixel_ptr, &width, &height);
    // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

    const tex_res tex_res = {width, height, 1};
    _font_texture = std::make_unique<Texture2D>(tex_res);
    auto pixels = std::unique_ptr<stbi_uc>(pixel_ptr);
    _font_texture->set_data(pixels);
    pixels.release();

    const auto tex_handle = _font_texture->get_tex_handle();
    _gui_shader->update_uniform("tex", tex_handle);
    // Store our identifier
    io.Fonts->TexID = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(_font_texture->get_name()));

    // Setup back-end capabilities flags
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    // We can honor io.WantSetMousePos requests (optional, rarely used)

    map_keys();
    setup_mouse_cursors();

    io.SetClipboardTextFn = win_mgr::extern_set_clipboard_string;
    io.GetClipboardTextFn = win_mgr::extern_get_clipboard_string;
    io.ClipboardUserData = _window->get();

    if (install_callbacks_flag) {
        install_callbacks();
    }
    init_buffers();
    ImGui::StyleColorsDark();
}

tostf::Gui::~Gui() {
    for (auto& cursor : _mouse_cursors) {
        win_mgr::destroy_cursor(cursor);
    }
    if (_font_texture != nullptr) {
        auto& io = ImGui::GetIO();
        io.Fonts->TexID = nullptr;
    }
    if (glIsBuffer(_vbo_handle)) {
        glDeleteBuffers(1, &_vbo_handle);
    }
    if (glIsBuffer(_ebo_handle)) {
        glDeleteBuffers(1, &_ebo_handle);
    }
    if (glIsVertexArray(_vao_handle)) {
        glDeleteVertexArrays(1, &_vao_handle);
    }
    ImGui::DestroyContext();
}

void tostf::Gui::start_frame() {
    auto& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt());
    // Font atlas needs to be built, call renderer _NewFrame() function e.g. ImGui_ImplOpenGL3_NewFrame() 

    // Setup display size
    const auto size = _window->get_res();
    const auto size_x = static_cast<float>(size.x);
    const auto size_y = static_cast<float>(size.y);
    const auto framebuffer_size = _window->get_framebuffer_size();
    io.DisplaySize = ImVec2(size_x, size_y);
    io.DisplayFramebufferScale = ImVec2(size_x > 0 ? static_cast<float>(framebuffer_size.x) / size_x : 0,
                                        size_y > 0 ? static_cast<float>(framebuffer_size.y) / size_y : 0);

    // Setup time step
    const auto current_time = win_mgr::get_time();
    io.DeltaTime = _time > 0.0 ? static_cast<float>(current_time - _time) : static_cast<float>(1.0f / 60.0f);
    _time = current_time;

    update_mouse();
    update_cursor();
    ImGui::NewFrame();
}

void tostf::Gui::render() const {
    ImGui::Render();
    _window->make_context_current();
    const auto framebuffer_size = _window->get_framebuffer_size();

    glViewport(0, 0, framebuffer_size.x, framebuffer_size.y);
    // Backup GL state
    GLint last_texture, last_array_buffer, last_vertex_array;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

    auto draw_data = ImGui::GetDrawData();

    auto& io = ImGui::GetIO();
    const auto fb_width = static_cast<int>(draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
    const auto fb_height = static_cast<int>(draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }
    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    // Backup GL state
    GLenum last_active_texture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, reinterpret_cast<GLint*>(&last_active_texture));
    glActiveTexture(GL_TEXTURE0);
    GLint last_polygon_mode[2];
    glGetIntegerv(GL_POLYGON_MODE, last_polygon_mode);
    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_scissor_box[4];
    glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
    GLenum last_blend_src_rgb;
    glGetIntegerv(GL_BLEND_SRC_RGB, reinterpret_cast<GLint*>(&last_blend_src_rgb));
    GLenum last_blend_dst_rgb;
    glGetIntegerv(GL_BLEND_DST_RGB, reinterpret_cast<GLint*>(&last_blend_dst_rgb));
    GLenum last_blend_src_alpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, reinterpret_cast<GLint*>(&last_blend_src_alpha));
    GLenum last_blend_dst_alpha;
    glGetIntegerv(GL_BLEND_DST_ALPHA, reinterpret_cast<GLint*>(&last_blend_dst_alpha));
    GLenum last_blend_equation_rgb;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, reinterpret_cast<GLint*>(&last_blend_equation_rgb));
    GLenum last_blend_equation_alpha;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, reinterpret_cast<GLint*>(&last_blend_equation_alpha));
    const auto last_enable_blend = glIsEnabled(GL_BLEND);
    const auto last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
    const auto last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
    const auto last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

    // Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayMin is typically (0,0) for single viewport apps.
    glViewport(0, 0, static_cast<GLsizei>(fb_width), static_cast<GLsizei>(fb_height));
    const auto l = draw_data->DisplayPos.x;
    const auto r = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
    const auto t = draw_data->DisplayPos.y;
    const auto b = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
    const glm::mat4 ortho_projection =
    {
        {2.0f / (r - l), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (t - b), 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {(r + l) / (l - r), (t + b) / (b - t), 0.0f, 1.0f},
    };
    _gui_shader->use();
    _gui_shader->update_uniform("ProjMtx", ortho_projection);
    _gui_shader->update_uniform("tex", 0);

    glBindVertexArray(_vao_handle);
    // Draw
    const auto pos = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = nullptr;

        glBindBuffer(GL_ARRAY_BUFFER, _vbo_handle);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmd_list->VtxBuffer.Size) * sizeof(ImDrawVert),
                     static_cast<const GLvoid*>(cmd_list->VtxBuffer.Data), GL_STREAM_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo_handle);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cmd_list->IdxBuffer.Size) * sizeof(ImDrawIdx),
                     static_cast<const GLvoid*>(cmd_list->IdxBuffer.Data), GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            auto pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                // User callback (registered via ImDrawList::AddCallback)
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else {
                const auto clip_rect = ImVec4(pcmd->ClipRect.x - pos.x, pcmd->ClipRect.y - pos.y,
                                              pcmd->ClipRect.z - pos.x,
                                              pcmd->ClipRect.w - pos.y);
                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
                    // Apply scissor/clipping rectangle
                    glScissor(static_cast<int>(clip_rect.x), static_cast<int>(fb_height - clip_rect.w),
                              static_cast<int>(clip_rect.z - clip_rect.x),
                              static_cast<int>(clip_rect.w - clip_rect.y));

                    // Bind texture, Draw
                    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(reinterpret_cast<intptr_t>(pcmd->TextureId)));
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(pcmd->ElemCount),
                                   sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
                }
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    // Restore modified GL state
    glActiveTexture(last_active_texture);
    glBindVertexArray(last_vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
    glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
    glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
    if (last_enable_blend) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    if (last_enable_cull_face) glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (last_enable_depth_test) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
    if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(last_polygon_mode[0]));
    glViewport(last_viewport[0], last_viewport[1], static_cast<GLsizei>(last_viewport[2]),
               static_cast<GLsizei>(last_viewport[3]));
    glScissor(last_scissor_box[0], last_scissor_box[1], static_cast<GLsizei>(last_scissor_box[2]),
              static_cast<GLsizei>(last_scissor_box[3]));
}

void tostf::Gui::map_keys() {
    auto& io = ImGui::GetIO();

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = to_underlying_type(win_mgr::key::tab);
    io.KeyMap[ImGuiKey_LeftArrow] = to_underlying_type(win_mgr::key::left);
    io.KeyMap[ImGuiKey_RightArrow] = to_underlying_type(win_mgr::key::right);
    io.KeyMap[ImGuiKey_UpArrow] = to_underlying_type(win_mgr::key::up);
    io.KeyMap[ImGuiKey_DownArrow] = to_underlying_type(win_mgr::key::down);
    io.KeyMap[ImGuiKey_PageUp] = to_underlying_type(win_mgr::key::page_up);
    io.KeyMap[ImGuiKey_PageDown] = to_underlying_type(win_mgr::key::page_down);
    io.KeyMap[ImGuiKey_Home] = to_underlying_type(win_mgr::key::home);
    io.KeyMap[ImGuiKey_End] = to_underlying_type(win_mgr::key::end);
    io.KeyMap[ImGuiKey_Insert] = to_underlying_type(win_mgr::key::insert);
    io.KeyMap[ImGuiKey_Delete] = to_underlying_type(win_mgr::key::del);
    io.KeyMap[ImGuiKey_Backspace] = to_underlying_type(win_mgr::key::backspace);
    io.KeyMap[ImGuiKey_Space] = to_underlying_type(win_mgr::key::space);
    io.KeyMap[ImGuiKey_Enter] = to_underlying_type(win_mgr::key::enter);
    io.KeyMap[ImGuiKey_Escape] = to_underlying_type(win_mgr::key::escape);
    io.KeyMap[ImGuiKey_A] = to_underlying_type(win_mgr::key::a);
    io.KeyMap[ImGuiKey_C] = to_underlying_type(win_mgr::key::c);
    io.KeyMap[ImGuiKey_V] = to_underlying_type(win_mgr::key::v);
    io.KeyMap[ImGuiKey_X] = to_underlying_type(win_mgr::key::x);
    io.KeyMap[ImGuiKey_Y] = to_underlying_type(win_mgr::key::y);
    io.KeyMap[ImGuiKey_Z] = to_underlying_type(win_mgr::key::z);
}

void tostf::Gui::setup_mouse_cursors() {
    _mouse_button_pressed->at(0) = false;
    _mouse_button_pressed->at(1) = false;
    _mouse_button_pressed->at(2) = false;
    _mouse_button_pressed->at(3) = false;
    _mouse_button_pressed->at(4) = false;
    _mouse_cursors[ImGuiMouseCursor_Arrow] = create_cursor(win_mgr::cursor_type::arrow);
    _mouse_cursors[ImGuiMouseCursor_TextInput] = create_cursor(win_mgr::cursor_type::ibeam);
    _mouse_cursors[ImGuiMouseCursor_ResizeAll] = create_cursor(win_mgr::cursor_type::arrow);
    _mouse_cursors[ImGuiMouseCursor_ResizeNS] = create_cursor(win_mgr::cursor_type::resize_ns);
    _mouse_cursors[ImGuiMouseCursor_ResizeEW] = create_cursor(win_mgr::cursor_type::resize_we);
    _mouse_cursors[ImGuiMouseCursor_ResizeNESW] = create_cursor(win_mgr::cursor_type::resize_nesw);
    _mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = create_cursor(win_mgr::cursor_type::resize_nwse);
    _mouse_cursors[ImGuiMouseCursor_Hand] = create_cursor(win_mgr::cursor_type::hand);
}

void tostf::Gui::install_callbacks() {
    auto* mouse_button_pressed = _mouse_button_pressed.get();
    const auto mouse_callback = [mouse_button_pressed](const win_mgr::raw_win_ptr, const int button, const int action,
                                                       int) {
        if (action == to_underlying_type(win_mgr::mouse_action::press) && button >= 0
            && button < static_cast<int>(mouse_button_pressed->size())) {
            mouse_button_pressed->at(button) = true;
        }
    };
    const auto scroll_callback = [](win_mgr::raw_win_ptr, const double xoffset, const double yoffset) {
        auto& io = ImGui::GetIO();
        io.MouseWheelH += static_cast<float>(xoffset);
        io.MouseWheel += static_cast<float>(yoffset);
    };

    const auto key_callback = [](win_mgr::raw_win_ptr, const int key, int, const int action, int mods) {
        auto& io = ImGui::GetIO();
        if (action == to_underlying_type(win_mgr::key_action::press)) {
            io.KeysDown[key] = true;
        }
        if (action == to_underlying_type(win_mgr::key_action::release))
            io.KeysDown[key] = false;

        (void) mods; // Modifiers are not reliable across systems
        io.KeyCtrl = io.KeysDown[to_underlying_type(win_mgr::key::left_ctrl)]
                     || io.KeysDown[to_underlying_type(win_mgr::key::right_ctrl)];
        io.KeyShift = io.KeysDown[to_underlying_type(win_mgr::key::left_shift)]
                      || io.KeysDown[to_underlying_type(win_mgr::key::right_shift)];
        io.KeyAlt = io.KeysDown[to_underlying_type(win_mgr::key::left_alt)]
                    || io.KeysDown[to_underlying_type(win_mgr::key::right_alt)];
        io.KeySuper = io.KeysDown[to_underlying_type(win_mgr::key::left_super)]
                      || io.KeysDown[to_underlying_type(win_mgr::key::right_super)];
    };

    const auto char_callback = [](win_mgr::raw_win_ptr, const unsigned int c) {
        auto& io = ImGui::GetIO();
        if (c > 0 && c < 0x10000) {
            io.AddInputCharacter(static_cast<unsigned short>(c));
        }
    };
    _window->add_mouse_callback("imgui", mouse_callback);
    _window->add_scroll_callback("imgui", scroll_callback);
    _window->add_key_callback("imgui", key_callback);
    _window->add_char_callback("imgui", char_callback);
}

void tostf::Gui::init_buffers() {
    glGenBuffers(1, &_vbo_handle);
    glGenBuffers(1, &_ebo_handle);
    glGenVertexArrays(1, &_vao_handle);
    glBindVertexArray(_vao_handle);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo_handle);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          reinterpret_cast<GLvoid*>(IM_OFFSETOF(ImDrawVert, pos)));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert),
                          reinterpret_cast<GLvoid*>(IM_OFFSETOF(ImDrawVert, uv)));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                          reinterpret_cast<GLvoid*>(IM_OFFSETOF(ImDrawVert, col)));
}

void tostf::Gui::update_mouse() {
    // Update buttons
    auto& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++) {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = _mouse_button_pressed->at(i)
                          || _window->check_mouse(win_mgr::to_mouse_button(i), win_mgr::mouse_action::press);
        _mouse_button_pressed->at(i) = false;
    }

    // Update mouse position
    const auto mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    if (_window->is_focused()) {
        if (io.WantSetMousePos) {
            _window->set_cursor_pos({
                static_cast<double>(mouse_pos_backup.x),
                static_cast<double>(mouse_pos_backup.y)
            });
        }
        else {
            const auto mouse = _window->get_cursor_pos();
            io.MousePos = ImVec2(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
        }
    }
}

void tostf::Gui::update_cursor() {
    auto& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange
        || _window->check_cursor_mode(win_mgr::cursor_mode::disabled)) {
        return;
    }

    const auto imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        _window->set_cursor_mode(win_mgr::cursor_mode::hidden);
    }
    else {
        // Show OS mouse cursor
        // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
        _window->set_cursor(_mouse_cursors[imgui_cursor]
                                ? _mouse_cursors[imgui_cursor]
                                : _mouse_cursors[ImGuiMouseCursor_Arrow]);
        _window->set_cursor_mode(win_mgr::cursor_mode::normal);
    }
}

void ImGui::StyleColorsVS(ImGuiStyle* dst) {
    const auto style = dst ? dst : &GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = colors[ImGuiCol_FrameBgHovered];
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
    colors[ImGuiCol_TitleBg] = colors[ImGuiCol_TitleBgActive];
    colors[ImGuiCol_TitleBgCollapsed] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_FrameBg];
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.24f, 0.24f, 0.26f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.62f, 0.62f, 0.62f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.94f, 0.92f, 0.94f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab] = colors[ImGuiCol_TitleBgActive];
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
    colors[ImGuiCol_Button] = colors[ImGuiCol_TitleBgActive]; //ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.11f, 0.59f, 0.92f, 1.00f); //ImVec4(0.11f, 0.59f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_TitleBgActive]; //ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    colors[ImGuiCol_Header] = colors[ImGuiCol_TitleBg];
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.24f, 0.24f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_TitleBgActive];
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.32f, 0.69f, 0.94f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = colors[ImGuiCol_ButtonHovered];//ImVec4(0.53f, 0.19f, 0.78f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.73f, 0.09f, 0.98f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

ImTextureID ImGui::ConvertGLuintToImTex(const GLuint t_id) {
    return reinterpret_cast<ImTextureID>(static_cast<intptr_t>(t_id));
}
