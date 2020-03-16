//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "geometry/geometry.hpp"

namespace tostf {
    struct Halfedge {
        unsigned id = 0;
        unsigned vertex_ref = 0;
        int opposite = -1;
        int next() const {
            return 3 * (id / 3) + (id + 1) % 3;
        }
        int prev() const {
            return 3 * (id / 3) + (id + 2) % 3;
        }
    };

    struct Halfedge_vertex {
        int halfedge_ref = -2;
    };


    class Halfedge_mesh {
    public:
        explicit Halfedge_mesh(std::shared_ptr<Geometry> geometry);
        void build();
        std::vector<glm::vec4> calculate_normals() const;
        std::vector<glm::vec4> find_hard_edge_candidates() const;
        Mesh_stats calculate_mesh_stats();
        std::shared_ptr<Geometry> get_geometry() const;
        Geometry_view compute_one_ring_neighborhood(unsigned vertex_id);
        std::vector<unsigned> compute_one_ring_neighborhood_faces(unsigned vertex_id);
        void find_boundaries();
        int find_adjacent_boundary_candidate(unsigned next_vertex);
        std::vector<std::vector<unsigned>> get_boundaries();
    private:
        std::shared_ptr<Geometry> _geometry;
        std::vector<Halfedge> _halfedges;
        std::vector<Halfedge_vertex> _vertices;
        std::vector<std::vector<unsigned>> _halfedge_map;
        std::optional<std::vector<std::vector<unsigned>>> _boundaries;
    };
}
