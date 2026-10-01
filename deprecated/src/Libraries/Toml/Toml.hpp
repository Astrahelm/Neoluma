#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <utility>
#include <variant>
#include <cstdint>

namespace Toml {
    enum class TomlType { Comment, String, Integer, Float, Boolean, Array, Table };

    struct TomlValue;
    
    using TomlArray = std::vector<TomlValue>;
    using TomlTable = std::vector<std::pair<std::string, TomlValue>>;
    
    struct TomlValue {
        TomlType type;
        std::variant<std::string, int64_t, double, bool, TomlArray, TomlTable> value;

        TomlValue() = default;
        TomlValue(const std::string& str) : type(TomlType::String), value(str) {}
        TomlValue(const char* str) : type(TomlType::String), value(std::string(str)) {}
        TomlValue(bool b) : type(TomlType::Boolean), value(b) {}
        TomlValue(int64_t i) : type(TomlType::Integer), value(i) {}
        TomlValue(double f) : type(TomlType::Float), value(f) {}
        TomlValue(const TomlArray& arr) : type(TomlType::Array), value(arr) {}
        TomlValue(const TomlTable& tbl) : type(TomlType::Table), value(tbl) {}

        TomlValue& operator[](const std::string& key);
    };

    struct Table {
        TomlTable data;
        TomlValue& operator[](const std::string& key);
        const TomlValue& operator[](const std::string& key) const;
        TomlTable& get();
    };

    TomlValue parseValue(const std::string& text);
    TomlTable parseToml(std::istream& in);

    std::string serializeValue(const TomlValue& val);
    void serializeTable(std::ostream& out, const TomlTable& table, const std::string& parent = "");
    std::string serialize(const TomlTable& root);
};
