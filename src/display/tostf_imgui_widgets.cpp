//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "tostf_imgui_widgets.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui_internal.h"
#include <algorithm>
#include <cmath>

bool ImGui::BeginButtonMenu(const std::string& name, bool opened) {
    if (Button(name.c_str())) {}
    const auto hovered = IsItemHovered();
    if (hovered) {
        opened = true;
    }
    if (opened) {
        const ImVec2 button_pos(GetItemRectMin().x, GetItemRectMax().y - 1);
        SetNextWindowPos(button_pos);
        Begin((name + "_menu").c_str(), nullptr,
              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar
              | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
              ImGuiWindowFlags_NoTitleBar);
        if (!hovered && !IsWindowHovered()) {
            opened = false;
            End();
        }
    }
    return opened;
}

bool ImGui::BeginBarMenu(const char* label, const bool first_menu, const bool enabled) {
    auto window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    auto& g = *GImGui;
    const auto& style = g.Style;
    const auto id = window->GetID(label);
    const char* str = "##barmenubar";
    const char* str_end = nullptr;
    IM_ASSERT(window->IDStack.Size > 1);
    const auto seed = window->IDStack.Data[window->IDStack.Size - 2];
    const ImGuiID barmenubar_id = ImHash(str, str_end ? static_cast<int>(str_end - str) : 0, seed);
    if (barmenubar_id != window->IDStack.back()) {
        return false;
    }
    const auto label_size = CalcTextSize(label, nullptr, true);

    auto menu_is_open = IsPopupOpen(id);
    const auto menuset_is_open = !(window->Flags & ImGuiWindowFlags_Popup)
                                 && (g.OpenPopupStack.Size > g.CurrentPopupStack.Size
                                     && g.OpenPopupStack[g.CurrentPopupStack.Size].OpenParentId
                                     == window->IDStack.back());
    const auto backed_nav_window = g.NavWindow;
    if (menuset_is_open) {
        g.NavWindow = window;
    }
    // Odd hack to allow hovering across menus of a same menu-set (otherwise we wouldn't be able to hover parent)

    const auto pos = window->DC.CursorPos;
    // Menu inside an horizontal menu bar
    // Selectable extend their highlight by half ItemSpacing in each direction.
    // For ChildMenu, the popup position will be overwritten by the call to FindBestWindowPosForPopup() in Begin()
    const auto popup_pos = ImVec2(pos.x - (first_menu ? style.WindowPadding.x : style.FramePadding.x),
                                  pos.y - style.FramePadding.y + window->DC.MenuBarOffset.y + window->CalcFontSize() +
                                  GImGui->Style.FramePadding.y * 2.0f);
    window->DC.CursorPos.x += static_cast<float>(static_cast<int>(style.ItemSpacing.x * 0.5f));
    PushStyleVar(ImGuiStyleVar_ItemSpacing, style.ItemSpacing * 2.0f);
    const auto w = label_size.x;
    const auto pressed = Selectable(label, menu_is_open,
                                    ImGuiSelectableFlags_NoHoldingActiveID | ImGuiSelectableFlags_PressedOnClick |
                                    ImGuiSelectableFlags_DontClosePopups |
                                    (!enabled ? ImGuiSelectableFlags_Disabled : 0),
                                    ImVec2(w, 0.0f));
    PopStyleVar();
    window->DC.CursorPos.x += static_cast<float>(static_cast<int>(style.ItemSpacing.x * (-1.0f + 0.5f)));
    // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().

    const auto hovered = enabled && ItemHoverable(window->DC.LastItemRect, id);
    if (menuset_is_open) {
        g.NavWindow = backed_nav_window;
    }

    bool want_open = false;
    bool want_close = false;
    // Menu bar
    if (menu_is_open && pressed && menuset_is_open) {
        // Click an open menu again to close it
        want_close = true;
        want_open = menu_is_open = false;
    }
    else if (pressed || hovered && menuset_is_open && !menu_is_open) {
        // First click to open, then hover to open others
        want_open = true;
    }
    else if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Down) {
        // Nav-Down to open
        want_open = true;
        NavMoveRequestCancel();
    }

    if (!enabled) {
        // explicitly close if an open menu becomes disabled, facilitate users code a lot in pattern such as 'if (BeginMenu("options", has_object)) { ..use object.. }'
        want_close = true;
    }
    if (!want_close && IsPopupOpen(id)) { }
    if (want_close && IsPopupOpen(id)) {
        ClosePopupToLevel(g.CurrentPopupStack.Size);
    }

    if (!menu_is_open && want_open && g.OpenPopupStack.Size > g.CurrentPopupStack.Size) {
        // Don't recycle same menu level in the same frame, first close the other menu and yield for a frame.
        OpenPopup(label);
        return false;
    }

    menu_is_open |= want_open;
    if (want_open) {
        OpenPopup(label);
    }

    if (menu_is_open) {
        // Sub-menus are ChildWindow so that mouse can be hovering across them (otherwise top-most popup menu would steal focus and not allow hovering on parent menu)
        SetNextWindowPos(popup_pos, ImGuiCond_Always);
        const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
                           ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings |
                           (window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_ChildMenu)
                                ? ImGuiWindowFlags_ChildMenu | ImGuiWindowFlags_ChildWindow
                                : ImGuiWindowFlags_ChildMenu);
        PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
        menu_is_open = BeginPopupEx(id, flags);
        PopStyleVar();
        // menu_is_open can be 'false' when the popup is completely clipped (e.g. zero size display)
    }
    return menu_is_open;
}

bool ImGui::BeginBarMenuBar() {
    //PushStyleVar(ImGuiStyleVar_)
    const auto window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent++;
    window->DC.NavLayerCurrentMask <<= 1;
    BeginGroup(); // Backup position on layer 0
    PushID("##barmenubar");
    return true;
}

void ImGui::EndBarMenuBar() {
    auto window = GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }
    PopID();
    EndGroup(); // Restore position on layer 0
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent--;
    window->DC.NavLayerCurrentMask >>= 1;
}

bool ImGui::BeginBarMenuWindow(const std::string& name, ImGuiWindowFlags flags) {
    flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar;
    auto& g = *GImGui;
    const auto& style = g.Style;
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(style.WindowPadding.x, 0));
    if (Begin(name.c_str(), nullptr, flags)) {
        PopStyleVar();
        BeginBarMenuBar();
        return true;
    }
    PopStyleVar();
    return false;
}

void ImGui::EndBarMenuWindow() {
    EndBarMenuBar();
    End();
}

bool ImGui::BeginTitlebar(const std::string& name, ImGuiWindowFlags flags) {
    flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;
    Begin(name.c_str(), nullptr, flags);
    const auto window = GetCurrentWindow();
    if (window->SkipItems) {
        return false;
    }
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.NavLayerCurrent++;
    window->DC.NavLayerCurrentMask <<= 1;
    BeginGroup(); // Backup position on layer 0
    PushID("##titlebar");
    return true;
}

void ImGui::EndTitlebar() {
    auto window = GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }
    PopID();
    EndGroup(); // Restore position on layer 0
    window->DC.LayoutType = ImGuiLayoutType_Vertical;
    window->DC.NavLayerCurrent--;
    window->DC.NavLayerCurrentMask >>= 1;
    End();
}

bool ImGui::SectionNode(const ImGuiID id, const ImGuiTreeNodeFlags flags, const char* label, const char* label_end) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 padding = style.FramePadding;

    if (!label_end)
        label_end = FindRenderedTextEnd(label);
    const ImVec2 label_size = CalcTextSize(label, label_end, false);

    // We vertically grow up to current line height up the typical widget height.
    const float text_base_offset_y = ImMax(padding.y, window->DC.CurrentLineTextBaseOffset);
    // Latch before ItemSize changes it
    const float frame_height = ImMax(ImMin(window->DC.CurrentLineSize.y, g.FontSize + style.FramePadding.y * 2),
                                     label_size.y + padding.y * 2);
    ImRect frame_bb = ImRect(window->DC.CursorPos,
                             ImVec2(window->Pos.x + GetContentRegionMax().x, window->DC.CursorPos.y + frame_height));
    // Framed header expand a little outside the default padding
    frame_bb.Min.x -= static_cast<float>(static_cast<int>(window->WindowPadding.x * 0.5f)) - 1;
    frame_bb.Max.x += static_cast<float>(static_cast<int>(window->WindowPadding.x * 0.5f)) - 1;

    const float text_offset_x = g.FontSize + padding.x * 3;
    // Collapser arrow width + Spacing
    const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);
    // Include collapser
    ItemSize(ImVec2(text_width, frame_height), text_base_offset_y);

    // For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
    // (Ideally we'd want to add a flag for the user to specify if we want the hit test to be done up to the right side of the content or not)
    const ImRect interact_bb = frame_bb;
    bool is_open = TreeNodeBehaviorIsOpen(id, flags);

    // Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
    // For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
    // This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
    if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(
            flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
        window->DC.TreeDepthMayJumpToParentOnPop |= (1 << window->DC.TreeDepth);

    bool item_add = ItemAdd(interact_bb, id);
    window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
    window->DC.LastItemDisplayRect = frame_bb;

    if (!item_add) {
        if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
            PushStyleVar(ImGuiStyleVar_IndentSpacing, g.FontSize * 0.5f);
            TreePushRawID(id);
        }
        return is_open;
    }

    // Flags that affects opening behavior:
    // - 0(default) ..................... single-click anywhere to open
    // - OpenOnDoubleClick .............. double-click anywhere to open
    // - OpenOnArrow .................... single-click on arrow to open
    // - OpenOnDoubleClick|OpenOnArrow .. single-click on arrow or double-click anywhere to open
    ImGuiButtonFlags button_flags = ImGuiButtonFlags_NoKeyModifiers | ((flags & ImGuiTreeNodeFlags_AllowItemOverlap)
                                                                           ? ImGuiButtonFlags_AllowItemOverlap
                                                                           : 0);
    if (!(flags & ImGuiTreeNodeFlags_Leaf))
        button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;
    if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
        button_flags |= ImGuiButtonFlags_PressedOnDoubleClick | ((flags & ImGuiTreeNodeFlags_OpenOnArrow)
                                                                     ? ImGuiButtonFlags_PressedOnClickRelease
                                                                     : 0);

    bool hovered, held;
    const bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
    if (!(flags & ImGuiTreeNodeFlags_Leaf)) {
        bool toggled = false;
        if (pressed) {
            toggled = !(flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) || (
                          g.NavActivateId == id);
            if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
                toggled |= IsMouseHoveringRect(interact_bb.Min,
                                               ImVec2(interact_bb.Min.x + text_offset_x, interact_bb.Max.y)) && (!g.
                    NavDisableMouseHover);
            if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
                toggled |= g.IO.MouseDoubleClicked[0];
            if (g.DragDropActive && is_open)
                // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
                toggled = false;
        }

        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open) {
            toggled = true;
            NavMoveRequestCancel();
        }
        if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open)
            // If there's something upcoming on the line we may want to give it the priority?
        {
            toggled = true;
            NavMoveRequestCancel();
        }

        if (toggled) {
            is_open = !is_open;
            window->DC.StateStorage->SetInt(id, is_open);
        }
    }
    if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
        SetItemAllowOverlap();

    // Render
    const ImU32 col = GetColorU32((held && hovered || is_open)
                                      ? ImGuiCol_TitleBgActive
                                      : hovered
                                      ? is_open
                                            ? ImGuiCol_ButtonHovered
                                            : ImGuiCol_HeaderHovered
                                      : ImGuiCol_TitleBg);
    const ImVec2 text_pos = frame_bb.Min + ImVec2(text_offset_x, text_base_offset_y);

    // Framed type
    RenderFrame(frame_bb.Min, frame_bb.Max, col, true, style.FrameRounding);
    RenderNavHighlight(frame_bb, id, ImGuiNavHighlightFlags_TypeThin);
    RenderArrow(frame_bb.Min + ImVec2(padding.x, text_base_offset_y), is_open ? ImGuiDir_Down : ImGuiDir_Right,
                1.0f);
    if (g.LogEnabled) {
        // NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
        const char log_prefix[] = "\n##";
        const char log_suffix[] = "##";
        LogRenderedText(&text_pos, log_prefix, log_prefix + 3);
        RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
        LogRenderedText(&text_pos, log_suffix + 1, log_suffix + 3);
    }
    else {
        RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
    }


    if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen)) {
        PushStyleVar(ImGuiStyleVar_IndentSpacing, g.FontSize * 0.5f);
        TreePushRawID(id);
    }
    return is_open;
}

bool ImGui::BeginCollapsingSection(const std::string& label, bool* p_open, const ImGuiTreeNodeFlags flags) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    if (p_open && !*p_open)
        return false;

    const auto id = window->GetID(label.c_str());
    const bool is_open = SectionNode(
        id, flags | ImGuiTreeNodeFlags_NoAutoOpenOnLog | ImGuiTreeNodeFlags_Framed
            | (p_open ? ImGuiTreeNodeFlags_AllowItemOverlap : 0),
        label.c_str());
    if (p_open) {
        // Create a small overlapping close button // FIXME: We can evolve this into user accessible helpers to add extra buttons on title bars, headers, etc.
        ImGuiItemHoveredDataBackup last_item_backup;
        ImGuiContext& g = *GImGui;
        const float button_radius = g.FontSize * 0.5f;
        const ImVec2 button_center = ImVec2(
            ImMin(window->DC.LastItemRect.Max.x, window->ClipRect.Max.x) - g.Style.FramePadding.x - button_radius,
            window->DC.LastItemRect.GetCenter().y);

        if (CloseButtonNode(window->GetID(reinterpret_cast<void*>(static_cast<intptr_t>(id + 1))), button_center,
                            button_radius, is_open))
            *p_open = false;
        last_item_backup.Restore();
    }

    return is_open;
}

void ImGui::EndCollapsingSection() {
    TreePop();
    PopStyleVar();
    Spacing();
    Spacing();
    PushStyleColor(ImGuiCol_Separator, GetColorU32(ImGuiCol_Border));
    PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    PopStyleVar();
    PopStyleColor();
}

void ImGui::HorizontalLine(const float thickness, const ImVec4 color) {
    const auto p = GetCursorScreenPos();
    GetWindowDrawList()->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + GetWindowSize().x, p.y + thickness),
                                       ColorConvertFloat4ToU32(color));
}

// Button to close a window
bool ImGui::CloseButton(const ImGuiID id, const ImVec2& pos, const float radius) {
    auto& g = *GImGui;
    auto window = g.CurrentWindow;

    // We intentionally allow interaction when clipped so that a mechanical Alt,Right,Validate sequence close a window.
    // (this isn't the regular behavior of buttons, but it doesn't affect the user much because navigation tends to keep items visible).
    const ImRect bb(pos - ImVec2(radius, radius), pos + ImVec2(radius, radius));
    const bool is_clipped = !ItemAdd(bb, id);

    bool hovered, held;
    const bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (is_clipped)
        return pressed;

    // Render
    auto center = bb.GetCenter();
    if (hovered) {
        const auto button_extends = ImVec2(ImMax(2.0f, radius), ImMax(2.0f, radius));
        window->DrawList->AddRectFilled(center - button_extends, center + button_extends,
                                        GetColorU32(held && hovered
                                                        ? ImGuiCol_ButtonActive
                                                        : hovered
                                                        ? window->Collapsed
                                                              ? ImGuiCol_HeaderHovered
                                                              : ImGuiCol_ButtonHovered
                                                        : ImGuiCol_Button));
    }


    const float cross_extent = radius * 0.7071f - 1.0f;
    const ImU32 cross_col = GetColorU32(ImGuiCol_Text);
    center -= ImVec2(0.5f, 0.5f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent),
                              center + ImVec2(-cross_extent, -cross_extent), cross_col, 1.0f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent),
                              center + ImVec2(-cross_extent, +cross_extent), cross_col, 1.0f);

    return pressed;
}

// Button to close a window
bool ImGui::CloseButtonNode(const ImGuiID id, const ImVec2& pos, const float radius, const bool is_open) {
    auto& g = *GImGui;
    auto window = g.CurrentWindow;

    // We intentionally allow interaction when clipped so that a mechanical Alt,Right,Validate sequence close a window.
    // (this isn't the regular behavior of buttons, but it doesn't affect the user much because navigation tends to keep items visible).
    const ImRect bb(pos - ImVec2(radius, radius), pos + ImVec2(radius, radius));
    const bool is_clipped = !ItemAdd(bb, id);

    bool hovered, held;
    const bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    if (is_clipped)
        return pressed;

    // Render
    auto center = bb.GetCenter();
    if (hovered) {
        const auto button_extends = ImVec2(ImMax(2.0f, radius), ImMax(2.0f, radius));
        window->DrawList->AddRectFilled(center - button_extends, center + button_extends,
                                        GetColorU32(held && hovered
                                                        ? ImGuiCol_ButtonActive
                                                        : hovered
                                                        ? window->Collapsed || !is_open
                                                              ? ImGuiCol_HeaderHovered
                                                              : ImGuiCol_ButtonHovered
                                                        : ImGuiCol_Button));
    }


    const float cross_extent = radius * 0.7071f - 1.0f;
    const ImU32 cross_col = GetColorU32(ImGuiCol_Text);
    center -= ImVec2(0.5f, 0.5f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, +cross_extent),
                              center + ImVec2(-cross_extent, -cross_extent), cross_col, 1.0f);
    window->DrawList->AddLine(center + ImVec2(+cross_extent, -cross_extent),
                              center + ImVec2(-cross_extent, +cross_extent), cross_col, 1.0f);

    return pressed;
}

void ImGui::RotatedImage(ImTextureID user_texture_id, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1,
                         const ImVec4& tint_col, const ImVec4& border_col) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;
    ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size);
    if (border_col.w > 0.0f) {
        bb.Max += ImVec2(2, 2);
    }
    ItemSize(bb);
    if (!ItemAdd(bb, 0))
        return;

    if (border_col.w > 0.0f) {
        window->DrawList->AddRect(bb.Min, bb.Max, GetColorU32(border_col), 0.0f);
        bb.Min += ImVec2(1, 1);
        bb.Max -= ImVec2(1, 1);
    }
    if ((GetColorU32(tint_col) & IM_COL32_A_MASK) != 0) {
        window->DrawList->PushTextureID(user_texture_id);
        window->DrawList->PrimReserve(6, 4);
        window->DrawList->PrimQuadUV(ImVec2(bb.Min.x, bb.Max.y), ImVec2(bb.Max.x, bb.Max.y),
                                     ImVec2(bb.Max.x, bb.Min.y), ImVec2(bb.Min.x, bb.Min.y),
                                     uv0, ImVec2(uv0.x, uv1.y), uv1, ImVec2(uv1.x, uv0.y), GetColorU32(tint_col));
        window->DrawList->PopTextureID();
    }
}

void ImGui::LoadingIndicatorCircle(const std::string& label, const float indicator_radius, const ImVec4& main_color,
                                   const ImVec4& backdrop_color, const int circle_count, const float speed) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) {
        return;
    }

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label.c_str());

    const ImVec2 pos = window->DC.CursorPos;
    const float circle_radius = indicator_radius / 10.0f;
    const ImRect bb(pos, ImVec2(pos.x + indicator_radius * 2.0f, pos.y + indicator_radius * 2.0f));
    ItemSize(bb);
    if (!ItemAdd(bb, id)) {
        return;
    }
    const float t = g.Time;
    const auto degree_offset = 2.0f * IM_PI / circle_count;
    for (int i = 0; i < circle_count; ++i) {
        const auto x = indicator_radius * std::sin(degree_offset * i);
        const auto y = indicator_radius * std::cos(degree_offset * i);
        const auto growth = std::max(0.0f, std::sin(t * speed - i * degree_offset));
        ImVec4 color;
        color.x = main_color.x * growth + backdrop_color.x * (1.0f - growth);
        color.y = main_color.y * growth + backdrop_color.y * (1.0f - growth);
        color.z = main_color.z * growth + backdrop_color.z * (1.0f - growth);
        color.w = 1.0f;
        window->DrawList->AddCircleFilled(ImVec2(pos.x + indicator_radius + x - 2.0f * circle_radius,
                                                 pos.y + indicator_radius - y - 2.0f * circle_radius),
                                          circle_radius + growth * circle_radius, GetColorU32(color));
    }
}

bool ImGui::CollapseButton(ImGuiID id, const ImVec2& pos) {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;

    ImRect bb(pos, pos + ImVec2(g.FontSize, g.FontSize) + g.Style.FramePadding * 2.0f);
    ItemAdd(bb, id);
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, ImGuiButtonFlags_None);

    ImU32 col = GetColorU32((held && hovered)
                                ? ImGuiCol_ButtonActive
                                : hovered
                                ? window->Collapsed
                                      ? ImGuiCol_HeaderHovered
                                      : ImGuiCol_ButtonHovered
                                : ImGuiCol_Button);
    auto center = bb.GetCenter();
    if (hovered || held) {
        auto button_extends = ImVec2(g.FontSize, g.FontSize) / 2.0f + ImVec2(1, 1);
        window->DrawList->AddRectFilled(center - button_extends, center + button_extends, col);
    }
    RenderArrow(bb.Min + g.Style.FramePadding, window->Collapsed ? ImGuiDir_Right : ImGuiDir_Down, 1.0f);

    // Switch to moving the window after mouse is moved beyond the initial drag threshold
    if (IsItemActive() && IsMouseDragging())
        StartMouseMovingWindow(window);

    return pressed;
}

bool ImGui::CustomSelectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg) {
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
        // FIXME-OPT: Avoid if vertically clipped.
        PopClipRect();

    ImGuiID id = window->GetID(label);
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x,
                size_arg.y != 0.0f ? size_arg.y : label_size.y + style.FramePadding.y);
    ImVec2 pos = window->DC.CursorPos;
    // pos.y += window->DC.CurrentLineTextBaseOffset;
    ImRect bb_inner(pos + style.FramePadding, pos + size);

    // Fill horizontal space.
    ImVec2 window_padding = window->WindowPadding;
    float max_x = (flags & ImGuiSelectableFlags_SpanAllColumns)
                      ? GetWindowContentRegionMax().x
                      : GetContentRegionMax().x;
    float w_draw = ImMax(label_size.x, window->Pos.x + max_x - window_padding.x - window->DC.CursorPos.x);
    ImVec2 size_draw((size_arg.x != 0 && !(flags & ImGuiSelectableFlags_DrawFillAvailWidth)) ? size_arg.x : w_draw,
                     size_arg.y != 0.0f ? size_arg.y : size.y + style.FramePadding.y);
    ImRect bb(pos, pos + size_draw);
    if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_DrawFillAvailWidth))
        bb.Max.x += window_padding.x;

    // Selectables are tightly packed together, we extend the box to cover spacing between selectable.
    float spacing_L = (float) (int) (style.ItemSpacing.x * 0.5f);
    float spacing_U = (float) (int) (style.ItemSpacing.y * 0.5f);
    float spacing_R = style.ItemSpacing.x - spacing_L;
    float spacing_D = style.ItemSpacing.y - spacing_U;
    ItemSize(bb);
    bb.Min.x -= spacing_L;
    // bb.Min.y -= spacing_U;
    bb.Max.x += spacing_R;
    // bb.Max.y += spacing_D;
    if (!ItemAdd(bb, (flags & ImGuiSelectableFlags_Disabled) ? 0 : id)) {
        if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet)
            PushColumnClipRect();
        return false;
    }

    // We use NoHoldingActiveID on menus so user can click and _hold_ on a menu then drag to browse child entries
    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_NoHoldingActiveID) button_flags |= ImGuiButtonFlags_NoHoldingActiveID;
    if (flags & ImGuiSelectableFlags_PressedOnClick) button_flags |= ImGuiButtonFlags_PressedOnClick;
    if (flags & ImGuiSelectableFlags_PressedOnRelease) button_flags |= ImGuiButtonFlags_PressedOnRelease;
    if (flags & ImGuiSelectableFlags_Disabled) button_flags |= ImGuiButtonFlags_Disabled;
    if (flags & ImGuiSelectableFlags_AllowDoubleClick)
        button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);
    if (flags & ImGuiSelectableFlags_Disabled)
        selected = false;

    // Hovering selectable with mouse updates NavId accordingly so navigation can be resumed with gamepad/keyboard (this doesn't happen on most widgets)
    if (pressed || hovered)
        if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent) {
            g.NavDisableHighlight = true;
            SetNavID(id, window->DC.NavLayerCurrent);
        }
    if (pressed)
        MarkItemEdited(id);

    // Render
    if (hovered || selected) {
        const ImGuiWindow* window_to_highlight = g.NavWindowingTarget ? g.NavWindowingTarget : g.NavWindow;
        const bool title_bar_is_highlight = window_to_highlight && window->RootWindowForTitleBarHighlight ==
                                            window_to_highlight->RootWindowForTitleBarHighlight;
        const ImU32 col = GetColorU32((held && hovered)
                                          ? ImGuiCol_ButtonActive
                                          : hovered
                                          ? ImGuiCol_ButtonHovered
                                          : title_bar_is_highlight
                                          ? ImGuiCol_Button
                                          : ImGuiCol_Header);
        RenderFrame(bb.Min, bb.Max, col, false, 0.0f);
        RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);
    }

    if ((flags & ImGuiSelectableFlags_SpanAllColumns) && window->DC.ColumnsSet) {
        PushColumnClipRect();
        bb.Max.x -= (GetContentRegionMax().x - max_x);
    }

    if (flags & ImGuiSelectableFlags_Disabled) PushStyleColor(ImGuiCol_Text, g.Style.Colors[ImGuiCol_TextDisabled]);
    RenderTextClipped(bb_inner.Min, bb.Max, label, NULL, &label_size, ImVec2(0.0f, 0.0f));
    if (flags & ImGuiSelectableFlags_Disabled) PopStyleColor();

    // Automatically close popups
    if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(
            window->DC.ItemFlags & ImGuiItemFlags_SelectableDontClosePopup))
        CloseCurrentPopup();
    return pressed;
}

bool ImGui::CustomSelectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg) {
    if (CustomSelectable(label, *p_selected, flags, size_arg)) {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}
