//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "math/glm_helper.hpp"
#include <vector>
#include <optional>
#include <memory>

namespace tostf
{
    template <typename T>
    struct Geometric_stats {
        T avg = T(0);
        T min = T(std::numeric_limits<T>::max());
        T max = T(std::numeric_limits<T>::lowest());
    };

    struct Mesh_stats {
        Geometric_stats<float> area{};
        Geometric_stats<float> edge_length{};
        float volume = 0.0f;
    };

    struct Bounding_box {
        glm::vec4 min{FLT_MAX};
        glm::vec4 max{-FLT_MAX};
    };

    Bounding_box calculate_bounding_box(const std::vector<glm::dvec4>& vertices);
    Bounding_box calculate_bounding_box(const std::vector<glm::vec4>& vertices);

    struct Faces_info {
        std::vector<glm::vec4> face_normals;
        std::vector<glm::vec4> face_centers;
    };

    struct Geometry {
        bool has_vertices() const;
        bool has_normals() const;
        bool has_uv_coords() const;
        bool has_indices() const;
        size_t count() const;
        unsigned get_attribute_count() const;
        Bounding_box get_bounding_box();
        void recalc_bounding_box();
        void move_to_center();
        Faces_info retrieve_faces_info();
        std::vector<glm::vec4> vertices;
        std::vector<glm::vec4> normals;
        std::vector<glm::vec2> uv_coords;
        std::vector<unsigned> indices;
    protected:
        std::optional<Bounding_box> _bounding_box;
        std::optional<Faces_info> _faces_info;
    };

    struct Geometry_view {
        Geometry_view() = default;
        Geometry_view(std::shared_ptr<Geometry> geom, std::vector<unsigned> in_indices);
        std::shared_ptr<Geometry> geometry;
        std::vector<unsigned> indices;
    };
}
