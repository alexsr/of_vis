//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <vector>
#include "math/vector_math.hpp"
#include "math/statistics.hpp"

namespace tostf
{
    template <typename T, typename = void>
    struct Dataset_stats {};

    template<typename T>
    struct Dataset_stats<T, std::enable_if_t<std::is_arithmetic_v<T>>> {
        T mean;
        T variance;
        T standard_deviation;
    };

    template<typename T, glm::length_t N, glm::precision P>
    struct Dataset_stats<glm::vec<N, T, P>> {
        glm::vec<N, T, P> mean;
        T variance;
        T standard_deviation;
    };

    template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || is_glm_vec_v<T>>>
    Dataset_stats<T> analyze_dataset(const std::vector<T>& dataset) {
        Dataset_stats<T> stats;
        stats.mean = math::mean(dataset);
        stats.variance = math::variance_seq(dataset, stats.mean);
        stats.standard_deviation = glm::sqrt(stats.variance);
        return stats;
    }
}
