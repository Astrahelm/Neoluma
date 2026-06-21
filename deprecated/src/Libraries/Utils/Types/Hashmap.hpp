#pragma once

#include <unordered_map>

template<typename K, typename V>
class HashMap {
    std::unordered_map<K, V> map;
public:
    HashMap() = default;

    void set(K key, V value) { map[key] = value; }
    V get(K key) const { return map.at(key); }
    const V& get(const K& key) const { return map.at(key); }
    V& operator[](K key) { return map[key]; }

    bool contains(K key) const { return map.find(key) != map.end(); }
    bool empty() const { return map.empty(); }
    void remove(const K& key) { map.erase(key); }
    void clear() { map.clear(); }
    size_t len() const { return map.size(); }
};
