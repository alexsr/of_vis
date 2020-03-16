//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <vector>
#include <algorithm>
#include <random>
#include "math/vector_math.hpp"
#include "math/statistics.hpp"
#include "utility/vector.hpp"

namespace tostf
{
    template <typename T>
    struct Cluster {
        T center;
        std::vector<T> data_points;
        void remove_outlier();
    };

    struct Cluster_assignment {
        Cluster_assignment() = default;
        explicit Cluster_assignment(int id);
        int data_id = -1;
        int cluster_id = -1;
    };

    std::vector<Cluster_assignment> init_cluster_assignments(int data_size);
    void sort_cluster_assignments(std::vector<Cluster_assignment>::iterator begin,
                                  std::vector<Cluster_assignment>::iterator end);

    template <typename T>
    std::vector<T> find_k_means_seeds(const std::vector<T>& v, int k);

    template <typename T>
    std::vector<Cluster<T>> k_means_pp(const std::vector<T>& v, int k, int max_iterations = 100);

    template <typename T>
    std::vector<Cluster<T>> k_means(const std::vector<T>& v, std::vector<T> centers,
                                    int k, int max_iterations = 100);
}

template <typename T>
void tostf::Cluster<T>::remove_outlier() {
    std::vector<unsigned int> outlier;
    auto standard_deviation = glm::sqrt(math::variance(data_points));
    for (int i = 0; i < static_cast<int>(data_points.size()); i++) {
        if (math::dist(data_points.at(i), center) > standard_deviation) {
            outlier.push_back(i);
        }
    }
    remove_items_by_id(data_points, outlier);
}

template <typename T>
std::vector<T> tostf::find_k_means_seeds(const std::vector<T>& v, const int k) {
    if (static_cast<int>(v.size()) < k) {
        throw std::runtime_error{"Cannot find k seeds in vector smaller than k."};
    }
    std::random_device rd;
    std::mt19937 rng(rd());
    const std::uniform_int_distribution<int> uni(0, static_cast<unsigned>(v.size() - 1));

    std::vector<T> centers{v.at(uni(rng))};
    std::vector<float> weight(v.size());
    for (int c = 1; c < k; c++) {
        for (int i = 0; i < static_cast<int>(v.size()); i++) {
            float min_dist_to_center = static_cast<float>(math::min_dist(v.at(i), centers));
            weight.at(i) = min_dist_to_center * min_dist_to_center;
        }
        const std::discrete_distribution<> d(weight.begin(), weight.end());
        centers.push_back(v.at(d(rng)));
    }
    return centers;
}

template <typename T>
std::vector<tostf::Cluster<T>> tostf::k_means_pp(const std::vector<T>& v, const int k,
                                                 const int max_iterations) {
    auto centers = find_k_means_seeds(v, k);
    return k_means(v, centers, k, max_iterations);
}

template <typename T>
std::vector<tostf::Cluster<T>> tostf::k_means(const std::vector<T>& v, std::vector<T> centers,
                                              const int k, const int max_iterations) {
    if (k < 2) {
        return {{math::mean(v), v}};
    }
    auto cluster_assignments = init_cluster_assignments(static_cast<int>(v.size()));
    std::vector<int> cluster_sizes(k, 0);
    for (int iteration = 0; iteration < max_iterations; iteration++) {
        cluster_sizes = std::vector<int>(k, 0);
        int assignments = 0;
        std::vector<T> new_centers(k, T(0));
#pragma omp parallel for
        for (int i = 0; i < static_cast<int>(v.size()); i++) {
            auto point = v.at(i);
            int min_c_id = -1;
            auto min_dist = FLT_MAX;
            for (int j = 0; j < k; j++) {
                const auto dist = distance(centers.at(j), point);
                if (dist < min_dist) {
                    min_c_id = j;
                    min_dist = dist;
                }
            }
            if (cluster_assignments.at(i).cluster_id != min_c_id) {
                assignments++;
                cluster_assignments.at(i).cluster_id = min_c_id;
            }
            new_centers.at(min_c_id) += point;
            cluster_sizes.at(min_c_id) += 1;
        }
#pragma omp parallel for
        for (int c = 0; c < k; c++) {
            auto center = new_centers.at(c) / static_cast<float>(cluster_sizes.at(c));
            centers.at(c) = math::nearest(center, v);
        }
    }
    sort_cluster_assignments(cluster_assignments.begin(), cluster_assignments.end());
    std::vector<T> v_copy(v.size());
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(v.size()); i++) {
        v_copy.at(i) = v.at(cluster_assignments.at(i).data_id);
    }
    std::vector<Cluster<T>> res(k);
    const auto split_ids = sub_vector(cluster_sizes, cluster_sizes.size() - 1);
    auto split_res = split_vector(v_copy, split_ids);
#pragma omp parallel for
    for (int i = 0; i < k; i++) {
        res.at(i).data_points = std::move(split_res.at(i));
        res.at(i).center = centers.at(i);
    }
    return res;
}
