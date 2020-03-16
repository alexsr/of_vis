//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "camera.hpp"
#include "gpu_interface/shader_program.hpp"
#include "gpu_interface/texture.hpp"

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <array>

namespace tostf
{
    class Gui {
    public:
        explicit Gui(std::shared_ptr<win_mgr::win_obj> window, float font_size = 18.0f, bool install_callbacks_flag = true,
                     const std::filesystem::path& vertex_path = "../../shader/gui/imgui.vert",
                     const std::filesystem::path& fragment_path = "../../shader/gui/imgui.frag");
        Gui(const Gui&) = delete;
        Gui& operator=(const Gui&) = delete;
        Gui(Gui&&) = default;
        Gui& operator=(Gui&&) = default;
        ~Gui();
        void start_frame();
        void render() const;
    private:
        static void map_keys();
        void setup_mouse_cursors();
        void install_callbacks();
        void init_buffers();
        void update_mouse();
        void update_cursor();
        std::unique_ptr<V_F_shader> _gui_shader;
        std::unique_ptr<Texture2D> _font_texture;
        GLuint _vbo_handle{};
        GLuint _ebo_handle{};
        GLuint _vao_handle{};
        std::shared_ptr<win_mgr::win_obj> _window;
        std::array<win_mgr::raw_cursor_ptr, ImGuiMouseCursor_COUNT> _mouse_cursors{};
        std::unique_ptr<std::array<bool, 5>> _mouse_button_pressed;
        double _time;
    };
}

namespace ImGui
{
    void StyleColorsVS(ImGuiStyle* dst = nullptr);
    ImTextureID ConvertGLuintToImTex(GLuint t_id);
}
