#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <filesystem>
#include <type_traits>
#include <variant>
#include <print>

// Alias of std::unique_ptr
template<typename T>
using MemoryPtr = std::unique_ptr<T>;

// Alias of std::variant
template<typename... T>
using Option = std::variant<T...>;

template<typename T, typename... Types>
constexpr bool optionIs(const Option<Types...>& value) {
    return std::holds_alternative<T>(value);
}

template<typename T, typename... Types>
constexpr T& getOption(Option<Types...>& value) {
    return std::get<T>(value);
}

template<typename T, typename... Types>
constexpr const T& getOption(const Option<Types...>& value) {
    return std::get<T>(value);
}

template<typename T, typename... Types>
constexpr T* tryGetOption(Option<Types...>* value) {
    return std::get_if<T>(value);
}

template<typename T, typename... Types>
constexpr const T* tryGetOption(const Option<Types...>* value) {
    return std::get_if<T>(value);
}

template<typename T, typename... Args>
MemoryPtr<T> makeMemoryPtr(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
MemoryPtr<T> makeSharedPtr(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename U>
MemoryPtr<T> as(MemoryPtr<U> ptr) {
    static_assert(std::is_base_of_v<U, T>, "[Neoluma/HelperFunctions] as<T>(ptr): T must derive from U");

    if constexpr (std::is_polymorphic_v<U>) {
        if (T* casted = dynamic_cast<T*>(ptr.get())) {
            ptr.release();
            return MemoryPtr<T>(casted);
        }
        return nullptr;
    } else {
        return MemoryPtr<T>(static_cast<T*>(ptr.release()));
    }
}

// Other
// Reads the file
std::string readFile(const std::string& filePath);

// turns \\ into / cuz screw how windows is made
std::string normalizePath(const std::string& path);
