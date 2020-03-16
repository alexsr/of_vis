//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "utility/utility.hpp"
#include "mesh.hpp"
#include <map>

namespace tostf
{
    enum class mesh_flag {
        vertices = 1,
        normals = 2,
        uv_coords = 4,
        indices = 8,
        all = 127
    };

    constexpr mesh_flag operator|(const mesh_flag i1, const mesh_flag i2) {
        return static_cast<mesh_flag>(to_underlying_type(i1) | to_underlying_type(i2));
    }

    constexpr bool operator&(const mesh_flag i1, const mesh_flag i2) {
        return (to_underlying_type(i1) & to_underlying_type(i2)) != 0;
    }

    std::optional<std::shared_ptr<Mesh>> load_single_mesh(const std::filesystem::path& path, unsigned id = 0,
                                                          bool move_to_center = false, unsigned ai_pflags = 0,
                                                          mesh_flag mesh_load_flags = mesh_flag::all);

    std::vector<std::shared_ptr<Mesh>> load_meshes(const std::filesystem::path& path, bool move_to_center = false,
                                                   unsigned ai_pflags = 0, mesh_flag mesh_load_flags = mesh_flag::all);

    std::map<std::string, std::shared_ptr<Mesh>> load_named_meshes(const std::filesystem::path& path,
                                                                   bool move_to_center = false, unsigned ai_pflags = 0,
                                                                   mesh_flag mesh_load_flags = mesh_flag::all);

    std::string gen_geom_indices_export_str(const std::shared_ptr<Geometry>& geometry, unsigned index_offset);
    std::string gen_geom_data_export_str(const std::shared_ptr<Geometry>& geometry, int precision = 7);

    void export_geom_as_obj(const std::shared_ptr<Geometry>& geometry, const std::filesystem::path& path,
                            int precision = 7);

    std::string gen_geom_view_data_export_str(const Geometry_view& view, int precision = 7);
    std::string gen_geom_view_scaled_data_export_str(const Geometry_view& view, float scale, int precision = 7);
    std::string gen_geom_view_indices_export_str(const Geometry_view& view, unsigned index_offset = 0);
    std::string gen_geom_view_export_str(const Geometry_view& view, unsigned index_offset = 0, int precision = 7);
    std::string gen_geom_view_scaled_export_str(const Geometry_view& view, float scale, unsigned index_offset = 0,
                                                int precision = 7);
    void export_geom_view_as_obj(const Geometry_view& view, const std::filesystem::path& path,
                                 unsigned index_offset = 0, int precision = 7);
}
