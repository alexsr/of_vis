//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "geometry/geometry.hpp"
#include "data_processing/plane_fitting.hpp"
#include "gpu_interface/vao.hpp"
#include "preprocessing/inlet_detection.hpp"

namespace tostf
{
    struct Aneurysm_renderer {
        explicit Aneurysm_renderer();
        Aneurysm_renderer(std::vector<VAO_attachment> in_vbos, std::shared_ptr<Element_buffer> in_ebo);
        void update_vbos(const std::vector<VAO_attachment>& in_vbos);
        void update_ebo(const std::shared_ptr<Element_buffer>& in_ebo);
        std::unique_ptr<VAO> vao;
        std::vector<VAO_attachment> vbos;
        std::shared_ptr<Element_buffer> ebo;
    };

    class Aneurysm_viewer {
    public:
        explicit Aneurysm_viewer(std::shared_ptr<Geometry> geometry);
        void reset_views();
        void toggle_all();
        void toggle_wall();
        void toggle_selection(unsigned i);
        void toggle_all_inlets();
        void update_renderer(Aneurysm_renderer& renderer);
        void add_inlet(const Inlet& inlet);
        void update_aneurysm_indices(const std::vector<unsigned>& new_indices);
        void set_from_auto_detect(const std::vector<Inlet>& in_inlets);
        void remove_inlet(unsigned i);
        bool full_mesh_visible() const;
        bool all_inlets_visible() const;
        size_t get_view_count() const;
        void select_all();
        Geometry_view aneurysm_view;
        std::vector<Geometry_view> inlet_views;
        std::vector<bool> selected_inlets;
        bool wall_selected = true;
    private:
        bool _selection_changed = false;
        std::shared_ptr<Geometry> _geometry;
    };
}
