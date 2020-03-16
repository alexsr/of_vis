//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once
#include <numeric>
#include <execution>

namespace tostf
{
    namespace math
    {
        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T mean_seq(const std::vector<T>& values) {
            T res(0);
            for (auto& v : values) {
                res += v;
            }
            return res / static_cast<float>(values.size());
        }

        template <typename T, glm::length_t N, glm::precision P>
        glm::vec<N, T, P> mean_seq(const std::vector<glm::vec<N, T, P>>& values) {
            T res(0);
            for (auto& v : values) {
                res += v;
            }
            return res / static_cast<float>(values.size());
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T mean(const std::vector<T>& values) {
            return std::reduce(std::execution::par, values.begin(), values.end(), static_cast<T>(0))
                   / static_cast<float>(values.size());
        }

        template <typename T, glm::length_t N, glm::precision P>
        glm::vec<N, T, P> mean(const std::vector<glm::vec<N, T, P>>& values) {
            return std::reduce(std::execution::par, values.begin(), values.end(), glm::vec<N, T, P>(static_cast<T>(0)))
                   / static_cast<float>(values.size());
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T variance_seq(const std::vector<T>& values) {
            T mean = 0;
            for (auto& v : values) {
                mean += v;
            }
            mean = mean / static_cast<float>(values.size());
            return variance_seq(values, mean);
        }

        template <typename T, size_t N, glm::precision P>
        T variance_seq(const std::vector<glm::vec<N, T, P>>& values) {
            glm::vec<N, T, P> mean(0);
            for (auto& v : values) {
                mean += v;
            }
            mean = mean / static_cast<float>(values.size());
            return variance_seq(values, mean);
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T variance_seq(const std::vector<T>& values, T mean) {
            T res = 0;
            for (auto& v : values) {
                res += (v - mean) * (v - mean);
            }
            return res / static_cast<float>(values.size());
        }

        template <typename T, glm::length_t N, glm::precision P>
        T variance_seq(const std::vector<glm::vec<N, T, P>>& values, glm::vec<N, T, P> mean) {
            T res = 0;
            for (auto& v : values) {
                res += dot(v - mean, v - mean);
            }
            return res / static_cast<float>(values.size());
        }

        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
        T variance(const std::vector<T>& values) {
            T res = 0;
            T m = mean(values);
            for (auto& v : values) {
                res += (v - m) * (v - m);
            }
            return res / static_cast<float>(values.size());
        }

        template <typename T, glm::length_t N, glm::precision P>
        T variance(const std::vector<glm::vec<N, T, P>>& values) {
            T res = 0;
            glm::vec<N, T, P> m = mean(values);
            for (auto& v : values) {
                res += dot(v - m, v - m);
            }
            return res / static_cast<float>(values.size());
        }
    }
}
