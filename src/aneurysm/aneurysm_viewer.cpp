//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "aneurysm_viewer.hpp"
#include <utility>
#include "core/gpu_interface.hpp"

tostf::Aneurysm_renderer::Aneurysm_renderer() : vao(std::make_unique<VAO>()) {}

tostf::Aneurysm_renderer::Aneurysm_renderer(std::vector<VAO_attachment> in_vbos,
                                            std::shared_ptr<Element_buffer> in_ebo)
    : vao(std::make_unique<VAO>()), vbos(std::move(in_vbos)), ebo(std::move(in_ebo)) {}

void tostf::Aneurysm_renderer::update_vbos(const std::vector<VAO_attachment>& in_vbos) {
    vbos = in_vbos;
    vao->attach_vbos(vbos, true);
}

void tostf::Aneurysm_renderer::update_ebo(const std::shared_ptr<Element_buffer>& in_ebo) {
    ebo = in_ebo;
    vao->attach_ebo(ebo);
}

tostf::Aneurysm_viewer::Aneurysm_viewer(std::shared_ptr<Geometry> geometry)
    : _geometry(std::move(geometry)) {
    aneurysm_view.geometry = _geometry;
}

void tostf::Aneurysm_viewer::reset_views() {
    inlet_views.clear();
    selected_inlets.clear();
}

void tostf::Aneurysm_viewer::toggle_all() {
    if (!full_mesh_visible()) {
        std::fill(selected_inlets.begin(), selected_inlets.end(), true);
        wall_selected = true;
    }
    else {
        std::fill(selected_inlets.begin(), selected_inlets.end(), false);
        wall_selected = false;
    }
    _selection_changed = true;
}

void tostf::Aneurysm_viewer::toggle_wall() {
    wall_selected = !wall_selected;
    _selection_changed = true;
}

void tostf::Aneurysm_viewer::toggle_selection(const unsigned i) {
    selected_inlets.at(i) = !selected_inlets.at(i);
    _selection_changed = true;
}

void tostf::Aneurysm_viewer::toggle_all_inlets() {
    const auto vis = !all_inlets_visible();
    std::fill(selected_inlets.begin(), selected_inlets.end(), vis);
    _selection_changed = true;
}

void tostf::Aneurysm_viewer::update_renderer(Aneurysm_renderer& renderer) {
    if (_selection_changed) {
        _selection_changed = false;
        if (full_mesh_visible() || inlet_views.empty()) {
            renderer.update_ebo(std::make_shared<Element_buffer>(_geometry->indices));
        }
        else {
            std::vector<unsigned> indices;
            if (wall_selected) {
                indices.insert(indices.end(), aneurysm_view.indices.begin(),
                               aneurysm_view.indices.end());
            }
            for (unsigned i = 0; i < inlet_views.size(); i++) {
                if (selected_inlets.at(i)) {
                    indices.insert(indices.end(), inlet_views.at(i).indices.begin(),
                                   inlet_views.at(i).indices.end());
                }
            }
            renderer.update_ebo(std::make_shared<Element_buffer>(indices));
        }
    }
}

void tostf::Aneurysm_viewer::add_inlet(const Inlet& inlet) {
    std::vector<unsigned> indices(inlet.indices.size() * 3);
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(indices.size() / 3); i++) {
        indices.at(i * 3) = inlet.indices.at(i).at(0);
        indices.at(i * 3 + 1) = inlet.indices.at(i).at(1);
        indices.at(i * 3 + 2) = inlet.indices.at(i).at(2);
    }
    inlet_views.emplace_back(_geometry, indices);
    selected_inlets.emplace_back(true);
}

void tostf::Aneurysm_viewer::update_aneurysm_indices(const std::vector<unsigned>& new_indices) {
    aneurysm_view.indices = new_indices;
    if (wall_selected) {
        _selection_changed = true;
    }
}

void tostf::Aneurysm_viewer::set_from_auto_detect(const std::vector<Inlet>& in_inlets) {
    inlet_views.clear();
    inlet_views.resize(in_inlets.size());
    selected_inlets.resize(in_inlets.size());
    if (!selected_inlets.empty()) {
        for (int i = 0; i < static_cast<int>(in_inlets.size()); ++i) {
            std::vector<unsigned> indices(in_inlets.at(i).indices.size() * 3);
#pragma omp parallel for
            for (int j = 0; j < static_cast<int>(indices.size() / 3); j++) {
                indices.at(i * 3) = in_inlets.at(i).indices.at(j).at(0);
                indices.at(i * 3 + 1) = in_inlets.at(i).indices.at(j).at(1);
                indices.at(i * 3 + 2) = in_inlets.at(i).indices.at(j).at(2);
            }
            inlet_views.at(i) = {_geometry, indices};
        }
    }
    select_all();
}

void tostf::Aneurysm_viewer::remove_inlet(const unsigned i) {
    inlet_views.erase(inlet_views.begin() + i);
    selected_inlets.erase(selected_inlets.begin() + i);
    _selection_changed = true;
}

bool tostf::Aneurysm_viewer::full_mesh_visible() const {
    return all_inlets_visible() && wall_selected;
}

bool tostf::Aneurysm_viewer::all_inlets_visible() const {
    bool all_inlets = true;
    for (auto v : selected_inlets) {
        all_inlets = all_inlets && v;
    }
    return all_inlets;
}

size_t tostf::Aneurysm_viewer::get_view_count() const {
    return inlet_views.size();
}

void tostf::Aneurysm_viewer::select_all() {
    std::fill(selected_inlets.begin(), selected_inlets.end(), true);
    wall_selected = true;
}
