//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "plane_fitting.hpp"
#include "utility/logging.hpp"
#include "utility/vector.hpp"
#include <Eigen/Eigenvalues>
#include "math/glm_eigen_interop.hpp"

tostf::Plane::Plane(const glm::vec4 in_center, const glm::vec4 in_normal) : center(in_center), normal(in_normal) {}

tostf::Plane::Plane(const glm::vec4 in_center, const glm::vec4 in_normal, const glm::vec4 in_tangent,
                    const glm::vec4 in_bitangent)
    : center(in_center), normal(in_normal), tangent(in_tangent), bitangent(in_bitangent) {}

std::vector<glm::vec4> tostf::Plane::to_points() const {
    std::vector<glm::vec4> res{
        center, center + normal,
        center + tangent + bitangent,
        center + tangent - bitangent,
        center - tangent + bitangent,
        center - tangent - bitangent
    };
    return res;
}

bool tostf::Plane::is_on_plane(const glm::vec4 p, const float bias) const {
    return abs(dot(center - p, normal)) <= bias;
}

tostf::Plane tostf::find_plane(const std::vector<glm::vec4>& points) {
    auto plane_verts = to_eig_point_mat(points);
    const auto center = plane_verts.rowwise().mean();
    auto centered_points = plane_verts.colwise() - center;
    const auto svd_setting = Eigen::ComputeFullU;
    auto svd = centered_points.jacobiSvd(svd_setting);
    auto u = svd.matrixU();
    return {
        to_glm(center), normalize(to_glm(u.col(2), 0)), normalize(to_glm(u.col(0), 0)), normalize(to_glm(u.col(1), 0))
    };
}

void tostf::remove_outliers(std::vector<glm::vec4>& points, const glm::vec4 center, const float bias) {
    float avg_dist = 0;
    std::vector<float> dists;
    for (auto& p : points) {
        auto dist = distance(center, p);
        avg_dist += dist;
        dists.push_back(dist);
    }
    std::vector<unsigned int> del_points;
    avg_dist /= static_cast<float>(points.size());
    for (int i = 0; i < static_cast<int>(points.size()); i++) {
        if (dists.at(i) > avg_dist + bias) {
            del_points.push_back(i);
        }
    }
    remove_items_by_id(points, del_points);
}
