//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#include "glm_helper.hpp"
#include <execution>
#include <vector>

namespace tostf
{
    namespace math
    {
        glm::vec4 calc_center(const std::vector<glm::vec4>& points);
        glm::vec3 calc_normal(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c);
        glm::vec3 calc_normalized_normal(const glm::vec4& a, const glm::vec4& b, const glm::vec4& c);

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T dist(const T& a, const T& b) {
            return abs(a - b);
        }

        template <typename T, glm::length_t N, glm::precision P>
        T dist(const glm::vec<N, T, P>& a, const glm::vec<N, T, P>& b) {
            return glm::distance(a, b);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || is_glm_vec_v<T>>>
        T nearest(const T& p, const T& a, const T& b) {
            if (dist(p, a) < dist(p, b)) {
                return a;
            }
            return b;
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T nearest(const T& p, const std::vector<T>& values) {
            return std::reduce(std::execution::par, values.begin(), values.end(),
                               std::numeric_limits<T>::max(),
                               [&p](const T& a, const T& b) {
                                   return nearest(p, a, b);
                               });
        }

        template <typename T, glm::length_t N, glm::precision P>
        glm::vec<N, T, P> nearest(const glm::vec<N, T, P>& p, const std::vector<glm::vec<N, T, P>>& values) {
            return std::reduce(std::execution::par, values.begin(), values.end(),
                               glm::vec<N, T, P>(std::numeric_limits<T>::max()),
                               [&p](const glm::vec<N, T, P>& a, const glm::vec<N, T, P>& b) {
                                   return nearest(p, a, b);
                               });
        }


        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T nearest_seq(const T& p, const std::vector<T>& values) {
            auto min_dist = std::numeric_limits<T>::max();
            T nearest_element;
            for (auto& v : values) {
                const auto d = dist(p, v);
                if (d < min_dist) {
                    min_dist = d;
                    nearest_element = v;
                }
            }
            return nearest_element;
        }

        template <typename T, glm::length_t N, glm::precision P>
        glm::vec<N, T, P> nearest_seq(const glm::vec<N, T, P>& p, const std::vector<glm::vec<N, T, P>>& values) {
            auto min_dist = std::numeric_limits<T>::max();
            glm::vec<N, T, P> nearest_element;
            for (auto& v : values) {
                const auto d = dist(p, v);
                if (d < min_dist) {
                    min_dist = d;
                    nearest_element = v;
                }
            }
            return nearest_element;
        }

        template <typename T, glm::length_t N, glm::precision P>
        unsigned nearest_id_seq(const glm::vec<N, T, P>& p, const std::vector<glm::vec<N, T, P>>& values) {
            auto min_dist = std::numeric_limits<T>::max();
            unsigned nearest_element = 0;
            for (unsigned i = 0; i < values.size(); i++) {
                const auto d = dist(p, values.at(i));
                if (d < min_dist) {
                    min_dist = d;
                    nearest_element = i;
                }
            }
            return nearest_element;
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T min_dist(const T& p, const std::vector<T>& values) {
            return dist(p, nearest(p, values));
        }

        template <typename T, glm::length_t N, glm::precision P>
        T min_dist(const glm::vec<N, T, P>& p, const std::vector<glm::vec<N, T, P>>& values) {
            return dist(p, nearest(p, values));
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T min_dist_seq(const T& p, const std::vector<T>& values) {
            auto min_dist = std::numeric_limits<T>::max();
            for (auto& v : values) {
                const auto d = dist(p, v);
                if (d < min_dist) {
                    min_dist = d;
                }
            }
            return min_dist;
        }

        template <typename T, glm::length_t N, glm::precision P>
        T min_dist_seq(const glm::vec<N, T, P>& p, const std::vector<glm::vec<N, T, P>>& values) {
            auto min_dist = std::numeric_limits<T>::max();
            for (auto& v : values) {
                const auto d = dist(p, v);
                if (d < min_dist) {
                    min_dist = d;
                }
            }
            return min_dist;
        }
    }
}
