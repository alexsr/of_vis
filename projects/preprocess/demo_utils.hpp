#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <geometry/mesh_handling.hpp>
#include <core/gpu_interface.hpp>
#include <core/display.hpp>
#include <file/file_handling.hpp>
#include <utility/logging.hpp>
#include <utility/event_profiler.hpp>
#include <gpu_interface/storage_buffer.hpp>
#include <gpu_interface/fbo.hpp>
#include <display/tostf_imgui_widgets.hpp>
#include <display/view.hpp>
#include <mesh_processing/halfedge_mesh.hpp>
#include <mesh_processing/tri_face_mesh.hpp>
#include <utility/random.hpp>
#include "imgui/imgui_internal.h"
#include "core/shader_settings.hpp"
#include "math/distribution.hpp"
#include "assimp/postprocess.h"
#include "aneurysm/aneurysm_viewer.hpp"
#include "mesh_processing/structured_mesh.hpp"
#include <preprocessing/inlet_detection.hpp>
#include <preprocessing/mesh_deformation.hpp>
#include <preprocessing/openfoam_exporter.hpp>
#include "iconfont/IconsMaterialDesignIcons_c.h"
#include "glm/gtx/component_wise.hpp"
#include "visualization/vis_utilities.hpp"
#include "gpu_interface/debug.hpp"
#include "foam_processing/foam_loader.hpp"
#include "geometry/mesh_handling.hpp"
#include "visualization/control.hpp"
#include "visualization/visualization.hpp"
#include <cstdio>
#include <thread>

inline std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

enum class popups : int {
    none,
    wrong_mesh_path,
    no_mesh,
    export_path_issue,
    no_inlet_selected,
    export_success
};

enum class preprocessing_steps {
    open_mesh,
    find_inlets,
    create_deformed,
    export_case,
    start_simulation,
    count,
    select_aneurysm,
};

inline std::string step_to_window_title(const preprocessing_steps step) {
    switch (step) {
    case preprocessing_steps::find_inlets:
        return "Preprocessing: Detect inlets";
    case preprocessing_steps::select_aneurysm:
        return "Preprocessing: Select aneurysm";
    case preprocessing_steps::create_deformed:
        return "Preprocessing: Create deformed meshes";
    case preprocessing_steps::export_case:
        return "Preprocessing: Export OpenFOAM case";
    case preprocessing_steps::start_simulation:
        return "Start simulation";
    case preprocessing_steps::open_mesh:
    default:
        return "Preprocessing: Select vessel mesh";
    }
}

inline std::string step_to_sidebar_title(const preprocessing_steps step) {
    switch (step) {
    case preprocessing_steps::find_inlets:
        return "Inlet detection controls";
    case preprocessing_steps::select_aneurysm:
        return "Select aneurysm";
    case preprocessing_steps::create_deformed:
        return "Deformation settings";
    case preprocessing_steps::export_case:
        return "OpenFOAM case settings";
    case preprocessing_steps::start_simulation:
        return "Start simulation";
    case preprocessing_steps::open_mesh:
    default:
        return "Mesh information";
    }
}

struct Proprocessing_settings {
    preprocessing_steps step = preprocessing_steps::open_mesh;
    popups popup = popups::none;
    bool night_mode = false;
    bool inlet_manual_mode = false;
    bool nav_blocked = false;
    std::array<bool, static_cast<int>(preprocessing_steps::count)> step_status{};

    inline void reset() {
        for (auto& s : step_status) {
            s = false;
        }
        inlet_manual_mode = false;
        nav_blocked = false;
        popup = popups::none;
        step = preprocessing_steps::open_mesh;
    }

    inline void go_to_step(unsigned step_id) {
        if (step_id < static_cast<int>(preprocessing_steps::count)) {
            if (is_step_finished(static_cast<preprocessing_steps>(step_id))) {
                step = static_cast<preprocessing_steps>(step_id);
            }
            else if (step_id > 0 && is_step_finished(static_cast<preprocessing_steps>(step_id - 1))) {
                step = static_cast<preprocessing_steps>(step_id);
            }
        }
    }

    inline void proceed() {
        if (is_current_step_finished() && !nav_blocked) {
            step = static_cast<preprocessing_steps>(static_cast<int>(step) + 1);
        }
    }

    inline void force_proceed() {
        finish_step(step);
        step = static_cast<preprocessing_steps>(static_cast<int>(step) + 1);
    }

    inline void back() {
        if (!nav_blocked) {
            step = static_cast<preprocessing_steps>(static_cast<int>(step) - 1);
        }
    }

    inline bool is_current_step_finished() {
        return step_status.at(static_cast<int>(step));
    }

    inline bool is_step_finished(preprocessing_steps s) {
        return step_status.at(static_cast<int>(s));
    }

    inline void finish_step() {
        step_status.at(static_cast<int>(step)) = true;
    }

    inline void finish_step(preprocessing_steps s) {
        step_status.at(static_cast<int>(s)) = true;
    }

    inline void reset_step() {
        step_status.at(static_cast<int>(step)) = true;
    }

    inline void reset_step(preprocessing_steps s) {
        step_status.at(static_cast<int>(s)) = false;
    }
};

enum class mesh_scale : int {
    km,
    m,
    dm,
    cm,
    mm,
    um,
    nm
};

struct Mesh_info {
    std::filesystem::path path;
    tostf::Mesh_stats stats{};
    tostf::Bounding_box bounding_box{};
    mesh_scale scale = mesh_scale::m;

    inline float get_scale() {
        switch (scale) {
        case mesh_scale::km:
            return 1000.0f;
        case mesh_scale::m:
            return 1.0f;
        case mesh_scale::dm:
            return 0.1f;
        case mesh_scale::cm:
            return 0.01f;
        case mesh_scale::mm:
            return 0.001f;
        case mesh_scale::um:
            return 0.000001f;
        case mesh_scale::nm:
            return 0.000000001f;
        }
        return 1.0f;
    }
};

struct Split_mesh {
    inline void reset() {
        active = -3;
        inlets.clear();
        std::fill(color_data.begin(), color_data.end(), aneurysm_color);
    }

    inline void set_inlets(std::vector<tostf::Inlet> in_inlets) {
        inlets = std::move(in_inlets);
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(inlets.size()); i++) {
            for (int idx = 0; idx < static_cast<int>(inlets.at(i).indices.size()); idx++) {
                color_data.at(inlets.at(i).indices.at(idx).at(0)) = inlets.at(i).color;
                color_data.at(inlets.at(i).indices.at(idx).at(1)) = inlets.at(i).color;
                color_data.at(inlets.at(i).indices.at(idx).at(2)) = inlets.at(i).color;
            }
        }
    }

    inline void add_inlet(const tostf::Inlet& inlet) {
        inlets.push_back(inlet);
#pragma omp parallel for
        for (int idx = 0; idx < static_cast<int>(inlet.indices.size()); idx++) {
            color_data.at(inlet.indices.at(idx).at(0)) = inlet.color;
            color_data.at(inlet.indices.at(idx).at(1)) = inlet.color;
            color_data.at(inlet.indices.at(idx).at(2)) = inlet.color;
        }
    }

    inline void remove_inlet(int id) {
        if (id >= 0 && id < static_cast<int>(inlets.size())) {
            const auto& inlet = inlets.at(id);
#pragma omp parallel for
            for (int idx = 0; idx < static_cast<int>(inlet.indices.size()); idx++) {
                color_data.at(inlet.indices.at(idx).at(0)) = aneurysm_color;
                color_data.at(inlet.indices.at(idx).at(1)) = aneurysm_color;
                color_data.at(inlet.indices.at(idx).at(2)) = aneurysm_color;
            }
        }
        inlets.erase(inlets.begin() + id);
    }

    inline void update_color(const tostf::Inlet & inlet) {
#pragma omp parallel for
        for (int idx = 0; idx < static_cast<int>(inlet.indices.size()); idx++) {
            color_data.at(inlet.indices.at(idx).at(0)) = inlet.color;
            color_data.at(inlet.indices.at(idx).at(1)) = inlet.color;
            color_data.at(inlet.indices.at(idx).at(2)) = inlet.color;
        }
    }

    std::vector<tostf::Inlet> inlets;
    std::vector<glm::vec4> color_data;
    glm::vec4 aneurysm_color{ 1.0f };
    int active = -3; // -3 = full_mesh, -2 = all_inlets, -1 = wall
};

struct Step_results {
    Mesh_info mesh_info;
    Split_mesh split_mesh;
    std::unique_ptr<tostf::Texture2D> reference_picture = nullptr;
    std::vector<tostf::Deformed_mesh> deformed;
    float min_dist_to_ref{};
    float max_dist_to_ref{};
};

struct Imgui_window_settings {
    float sidebar_width = 300.0f;
    float panel_height = 200.0f;
    int deform_pic_size = 80;
    float deform_bar_height = 160.0f;
    //bool see_deform_results;
};

struct Deformation_control {
    int iterations = 100;
    int current = 0;
    float gauss_variance = 2.0f;
    float rayleigh_variance = 1.0f;
    float rayleigh_shift = 1.0f;
    float scale = 1.0f;

    inline void stop() {
        current = 0;
    }

    inline float progress() const {
        return static_cast<float>(current) / iterations;
    }

    inline std::string progress_str() const {
        std::stringstream str;
        str << current << "/" << iterations;
        return str.str();
    }
};

struct Deformation_settings {
    Deformation_settings() {
        update_curve();
    }

    float value_variance_in_percent = 0.2f;
    int deformation_count = 1;
    bool append_deforms = false;
    bool preserve_inlets = false;
    std::array<float, 100> deform_curve{};
    Deformation_control control_reference;
    std::vector<Deformation_control> controls;
    int active_view = -1;
    bool was_run = false;
    std::future<void> future;

    inline void stop() {
        was_run = true;
        future = std::future<void>();
    }

    inline void reset() {
        controls.clear();
        controls.resize(deformation_count);
        if (value_variance_in_percent == 0.0f) {
            for (auto& c : controls) {
                c.iterations = control_reference.iterations;
                c.gauss_variance = control_reference.gauss_variance;
                c.rayleigh_variance = control_reference.rayleigh_variance;
                c.rayleigh_shift = control_reference.rayleigh_shift;
                c.scale = control_reference.scale;
            }
        }
        else {
            std::random_device rd{};
            std::mt19937 gen{ rd() };
            std::normal_distribution<float> iter_nd(static_cast<float>(control_reference.iterations),
                static_cast<float>(
                    value_variance_in_percent * control_reference.iterations));
            std::normal_distribution<float> gauss_nd(control_reference.gauss_variance,
                value_variance_in_percent * control_reference.gauss_variance);
            std::normal_distribution<float> rayleigh_variance_nd(control_reference.rayleigh_variance,
                value_variance_in_percent * control_reference.
                rayleigh_variance);
            std::normal_distribution<float> shift_nd(control_reference.rayleigh_shift,
                value_variance_in_percent * control_reference.rayleigh_shift);
            std::normal_distribution<float> scale_nd(control_reference.scale,
                value_variance_in_percent * control_reference.scale);
            for (auto& c : controls) {
                c.iterations = static_cast<int>(iter_nd(gen));
                c.gauss_variance = gauss_nd(gen);
                c.rayleigh_variance = rayleigh_variance_nd(gen);
                c.rayleigh_shift = shift_nd(gen);
                c.scale = scale_nd(gen);
            }
        }
    }

    inline void update_curve() {
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(deform_curve.size()); i++) {
            deform_curve.at(i) = control_reference.scale * tostf::math::calc_gauss_rayleigh(
                i / static_cast<float>(deform_curve.size()) * 5.0f,
                control_reference.gauss_variance,
                control_reference.rayleigh_variance, control_reference.rayleigh_shift);
        }
    }
};

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
