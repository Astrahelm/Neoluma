#include "Toml.hpp"
#include "../../HelperFunctions.hpp"

namespace Toml {
    // ==== Helper utilities ====
    TomlValue& getOrCreate(TomlTable& table, const std::string& key) {
        for (auto& [k, v] : table) {
            if (k == key) {
                if (v.type != TomlType::Table) v.type = TomlType::Table;
                return v;
            }
        }
        TomlValue val;
        val.type = TomlType::Table;
        val.value = TomlTable{};
        table.emplace_back(key, std::move(val));
        return table.back().second;
    }

    TomlValue& TomlValue::operator[](const std::string& key) {
        TomlTable& table = std::get<TomlTable>(value);
        for (auto& [k, v] : table) {
            if (k == key) return v;
        }
        table.emplace_back(key, TomlValue{});
        return table.back().second;
    }

    // ===== Main parsing =====
    TomlValue parseValue(const std::string& text) {
        std::string s = trim(text);
        if (s.empty()) return TomlValue("");

        // arrays
        if (s.front() == '[' && s.back() == ']') {
            TomlArray arr;
            std::string inside = s.substr(1, s.size() - 2);
            std::stringstream ss(inside);
            std::string item;
            while (std::getline(ss, item, ',')) {
                arr.push_back(parseValue(item));
            }
            return TomlValue(arr);
        }

        // string
        if (s.front() == '"' && s.back() == '"') {
            return TomlValue(s.substr(1, s.size() - 2));
        }

        // boolean
        if (s == "true") return TomlValue(true);
        if (s == "false") return TomlValue(false);

        // integer
        size_t pos;
        int64_t i = std::stoll(s, &pos);
        if (pos == s.size()) return TomlValue(i);

        // float
        double d = std::stod(s, &pos);
        if (pos == s.size()) return TomlValue(d);

        // fallback as string
        return TomlValue(s);
    }

    TomlTable parseToml(std::istream& in) {
        TomlTable root;
        TomlTable* current = &root;

        std::string line;
        while (std::getline(in, line)) {
            // remove comments
            auto pos = line.find('#');
            if (pos != std::string::npos) line = line.substr(0, pos);

            line = trim(line);
            if (line.empty()) continue;

            // parse table headers into tomltable
            if (line.front() == '[' && line.back() == ']') {
                auto parts = split(line.substr(1, line.size() - 2), '.');
                current = &root;
                for (auto& part : parts) {
                    TomlValue& v = getOrCreate(*current, part);
                    v.type = TomlType::Table;
                    current = &std::get<TomlTable>(v.value);
                }
            }
            else {
                auto eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = trim(line.substr(0, eq));
                std::string val = trim(line.substr(eq + 1));
                TomlValue& v = getOrCreate(*current, key);
                v = parseValue(val);
            }
        }

        return root;
    }

    std::string serializeValue(const TomlValue& val) {
        switch (val.type) {
            case TomlType::String:  return "\"" + std::get<std::string>(val.value) + "\"";
            case TomlType::Integer: return std::to_string(std::get<int64_t>(val.value));
            case TomlType::Float:   return std::to_string(std::get<double>(val.value));
            case TomlType::Boolean: return std::get<bool>(val.value) ? "true" : "false";
            case TomlType::Array: {
                const auto& arr = std::get<TomlArray>(val.value);
                std::string res = "[";
                for (size_t i = 0; i < arr.size(); i++) {
                    res += serializeValue(arr[i]);
                    if (i + 1 < arr.size()) res += ", ";
                }
                return res + "]";
            }
            case TomlType::Table: return "";
            case TomlType::Comment: return "#" + std::get<std::string>(val.value);
        }
        return "";
    }

    void serializeTable(std::ostream& out, const TomlTable& table, const std::string& parent) {
        // values
        for (auto& [key, val] : table) {
            if (val.type != TomlType::Table) {
                out << key << " = " << serializeValue(val) << "\n";
            }
        }
        // subtables
        for (auto& [key, val] : table) {
            if (val.type == TomlType::Table) {
                std::string full = parent.empty() ? key : parent + "." + key;
                out << "\n[" << full << "]\n";
                serializeTable(out, std::get<TomlTable>(val.value), full);
            }
        }
    }

    std::string serialize(const TomlTable& root) {
        std::ostringstream out;
        serializeTable(out, root);
        return out.str();
    }

    TomlValue& Table::operator[](const std::string& key) {
        for (auto& [k, v] : data)
            if (k == key) return v;
        data.emplace_back(key, TomlValue{});
        return data.back().second;
    }

    const TomlValue& Table::operator[](const std::string& key) const {
        for (const auto& [k, v] : data)
            if (k == key) return v;
        throw std::out_of_range(std::format("[NeolumaLibs/Toml::Table] Key '{}' not found", key));
    }

    TomlTable& Table::get() { return data; }
}
