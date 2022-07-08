#pragma once

#include <cstddef>
#include <vector>
#include <map>
#include <mutex>

template<typename Key, typename Value>
class ConcurrentMap {
public:
    //static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> l;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count);

    explicit ConcurrentMap(const std::map<Key, Value>& solid, size_t bucket_count);

    explicit ConcurrentMap(const std::map<Key, Value>& solid);

    Access operator[](const Key& key);

    void Erase(const Key& key);

    std::map<Key, Value> BuildOrdinaryMap();

private:
    std::vector<std::map<Key, Value>> map_array_;
    std::vector<std::mutex> m_vector_;
    size_t arr_size_;

    void Swap(ConcurrentMap& other);
};

template<typename Key, typename Value>
ConcurrentMap<Key, Value>::ConcurrentMap(size_t bucket_count) : map_array_(bucket_count),
                                                                m_vector_(bucket_count),
                                                                arr_size_(bucket_count) {
}

template<typename Key, typename Value>
ConcurrentMap<Key, Value>::ConcurrentMap(const std::map<Key, Value>& solid, size_t bucket_count) {
    ConcurrentMap temp(bucket_count);

    for (const auto& [key, val]: solid) {
        temp[key].ref_to_value = std::move(val);
    }

    Swap(temp);
}

template<typename Key, typename Value>
ConcurrentMap<Key, Value>::ConcurrentMap(const std::map<Key, Value>& solid) {
    ConcurrentMap temp(solid.size() / 10);

    for (const auto& [key, val]: solid) {
        temp[key].ref_to_value = val;
    }

    Swap(temp);
}

template<typename Key, typename Value>
typename ConcurrentMap<Key, Value>::Access ConcurrentMap<Key, Value>::operator[](const Key& key) {
    const int index = key % arr_size_;

    return {std::lock_guard(m_vector_[index]), map_array_[index][key]};
}

template<typename Key, typename Value>
void ConcurrentMap<Key, Value>::Erase(const Key& key) {
    const int index = key % arr_size_;
    std::lock_guard l(m_vector_[index]);
    map_array_[index].erase(key);
}

template<typename Key, typename Value>
std::map<Key, Value> ConcurrentMap<Key, Value>::BuildOrdinaryMap() {
    std::map<Key, Value> ret_map;

    for (size_t i = 0; i < arr_size_; ++i) {
        std::lock_guard l(m_vector_[i]);
        std::map<Key, Value> temp(map_array_[i].begin(), map_array_[i].end());
        ret_map.merge(move(temp));
    }

    return ret_map;
}

template<typename Key, typename Value>
void ConcurrentMap<Key, Value>::Swap(ConcurrentMap& other) {
    std::swap(map_array_, other.map_array_);
    std::swap(m_vector_, other.m_vector_);
    std::swap(arr_size_, other.arr_size_);
}