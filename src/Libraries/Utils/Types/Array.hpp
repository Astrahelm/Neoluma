#pragma once

#include <vector>
#include <utility>
#include <cstddef>
#include <initializer_list>

template<typename T>
class Array {
    std::vector<T> data;
public:
    Array() = default;
    Array(std::initializer_list<T> values) : data(values) {}

    void add(const T& value) { data.push_back(value); }
    void add(T&& value) { data.push_back(std::move(value)); }
    std::size_t len() const { return data.size(); }
    void clear() { data.clear(); }
    T& first() { return data[0]; }
    T& last() { return data[data.size() - 1]; }
    void reverse() { std::reverse(begin(), end()); }
    bool empty() const { return data.empty(); }
    T& operator[](std::size_t index) { return data[index]; }
    const T& operator[](std::size_t index) const { return data[index]; }

    // evil ass rape functions
    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }
    auto begin() { return data.begin(); }
    auto end() { return data.end(); }
};
