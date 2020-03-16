//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#include "clustering.hpp"

tostf::Cluster_assignment::Cluster_assignment(const int id) : data_id(id) {}

std::vector<tostf::Cluster_assignment> tostf::init_cluster_assignments(const int data_size) {
    std::vector<Cluster_assignment> cluster_assignment(data_size);
#pragma omp parallel for
    for (int i = 0; i < data_size; i++) {
        cluster_assignment.at(i) = Cluster_assignment(i);
    }
    return cluster_assignment;
}

void tostf::sort_cluster_assignments(const std::vector<Cluster_assignment>::iterator begin,
                                     const std::vector<Cluster_assignment>::iterator end) {
    std::sort(begin, end,
              [](const Cluster_assignment& a, const Cluster_assignment& b) {
                  return a.cluster_id < b.cluster_id;
              });
}
