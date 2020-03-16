//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include "glm_helper.hpp"
#include <Eigen/Core>

namespace tostf
{
    inline std::vector<Eigen::Vector3f> to_eig_vector(const std::vector<glm::vec4>& v) {
        std::vector<Eigen::Vector3f> res(v.size());
        for (int i = 0; i < static_cast<int>(v.size()); i++) {
            res.at(i) = Eigen::Vector3f(v.at(i).x, v.at(i).y, v.at(i).z);
        }
        return res;
    }

    inline Eigen::Matrix3Xf to_eig_point_mat(const std::vector<glm::vec4>& v) {
        Eigen::Matrix3Xf res(3, v.size());
        for (int i = 0; i < static_cast<int>(v.size()); i++) {
            res.col(i) = Eigen::Vector3f(v.at(i).x, v.at(i).y, v.at(i).z);
        }
        return res;
    }

    inline std::vector<glm::vec4> to_glm_vector(const Eigen::Matrix3Xf& m) {
        std::vector<glm::vec4> res(m.cols());
        for (int i = 0; i < m.cols(); i++) {
            res.at(i) = glm::vec4(m.col(i)[0], m.col(i)[1], m.col(i)[2], 1.0f);
        }
        return res;
    }

    inline glm::vec4 to_glm(const Eigen::Vector3f& v, const float w = 1.0f) {
        return glm::vec4(v[0], v[1], v[2], w);
    }
}
