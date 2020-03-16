//
// Alexander Scheid-Rehder
// alexanderb@scheid-rehder.de
// https://www.alexsr.de
// https://github.com/alexsr
//

#pragma once

#include <vector>
#include <algorithm>
#include <stdexcept>
#include <map>

namespace tostf
{
    template <typename T>
    std::vector<T> sub_vector(const std::vector<T>& v, size_t length, size_t offset = 0) {
        if (v.begin() + offset + length > v.end()) {
            throw std::runtime_error{"Subvector out of range of vector."};
        }
        return std::vector<T>(v.begin() + offset, v.begin() + offset + length);
    }

    template <typename T>
    std::vector<std::vector<T>> split_vector(std::vector<T> v, std::vector<int> split_ids) {
        std::sort(split_ids.begin(), split_ids.end());
        std::vector<std::vector<T>> res(split_ids.size() + 1);
        int i = 0;
        for (auto split : split_ids) {
            if (split > static_cast<int>(v.size())) {
                break;
            }
            res.at(i) = std::vector<T>(std::make_move_iterator(v.begin()),
                                       std::make_move_iterator(v.begin() + split));
            v.erase(v.begin(), v.begin() + split);
            i++;
        }
        res.at(i) = std::move(v);
        return res;
    }

    template <typename T>
    void remove_items_by_id(std::vector<T>& v, std::vector<unsigned int> ids) {
        std::sort(ids.begin(), ids.end(), std::greater<>());
        for (auto& d : ids) {
            v.erase(v.begin() + d);
        }
    }

    template <typename K, typename V>
    std::map<K, V> merge_maps(const std::map<K, V>& left, const std::map<K, V>& right) {
        std::map<K, V> result;
        auto it_left = left.begin();
        auto it_right = right.begin();
        while (it_left != left.end() && it_right != right.end()) {
            if (it_left->first < it_right->first) {
                ++it_left;
            }
            else if (it_right->first < it_left->first) {
                ++it_right;
            }
            else {
                result.insert_or_assign(it_left->first, it_left->second);
                ++it_left;
                ++it_right;
            }
        }
        return result;
    }
}
