//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "gpu_interface/debug.hpp"
#include "gpu_interface/vao.hpp"
#include "gpu_interface/shader_program.hpp"
#include "geometry/geometry.hpp"
#include "gpu_interface/vbo.hpp"
#include "gpu_interface/element_buffer.hpp"

namespace tostf
{
    namespace gl
    {
        inline void init_default_setup() {
            set_clear_color(1, 1, 1, 1);
            enable(cap::depth_test);
            clear_all();
        }
    }

    inline std::vector<VAO_attachment> create_vbos_from_geometry(const std::shared_ptr<Geometry>& geom,
                                                                 std::array<unsigned, 3> ids = {0, 1, 2}) {
        std::vector<VAO_attachment> vbos;
        if (geom->has_vertices()) {
            vbos.emplace_back(std::make_shared<VBO>(geom->vertices, 4), ids.at(0));
        }
        if (geom->has_normals()) {
            vbos.emplace_back(std::make_shared<VBO>(geom->normals, 4), ids.at(1));
        }
        if (geom->has_uv_coords()) {
            vbos.emplace_back(std::make_shared<VBO>(geom->uv_coords, 2), ids.at(2));
        }
        return vbos;
    }

    inline std::shared_ptr<Element_buffer> create_ebo_from_geometry(const std::shared_ptr<Geometry>& geom) {
        if (!geom->has_indices()) { }
        auto ebo = std::make_shared<Element_buffer>(geom->indices);
        return ebo;
    }
}
