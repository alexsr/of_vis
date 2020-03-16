//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <string>
#include "imgui.h"

namespace ImGui
{
    bool BeginButtonMenu(const std::string& name, bool opened = false);
    bool BeginBarMenu(const char* label, bool first_menu = false, bool enabled = true);
    bool BeginBarMenuBar();
    void EndBarMenuBar();
    bool BeginBarMenuWindow(const std::string& name, ImGuiWindowFlags flags = 0);
    void EndBarMenuWindow();
    bool BeginTitlebar(const std::string& name = "Titlebar", ImGuiWindowFlags flags = 0);
    void EndTitlebar();
    bool SectionNode(ImGuiID id, ImGuiTreeNodeFlags flags, const char* label, const char* label_end = nullptr);
    bool BeginCollapsingSection(const std::string& label, bool* p_open = nullptr, ImGuiWindowFlags flags = 0);
    void EndCollapsingSection();
    void HorizontalLine(float thickness, ImVec4 color);
    bool CustomSelectable(const char* label, bool selected = false, ImGuiSelectableFlags flags = 0,
                          const ImVec2& size = ImVec2(0, 0));
    bool CustomSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags = 0,
                          const ImVec2& size_arg = ImVec2(0, 0));
    bool CloseButtonNode(ImGuiID id, const ImVec2& pos, float radius, bool is_open);
    void RotatedImage(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0),
                      const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tint_col = ImVec4(1, 1, 1, 1),
                      const ImVec4& border_col = ImVec4(0, 0, 0, 0));
    void LoadingIndicatorCircle(const std::string& label, float indicator_radius, const ImVec4& main_color,
                                const ImVec4& backdrop_color, int circle_count = 12, float speed = 5.0f);
}
