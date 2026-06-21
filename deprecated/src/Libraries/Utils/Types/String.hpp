#pragma once

#include <string>
#include <string_view>
#include <cstddef>
#include <format>
#include <sstream>
#include <stdexcept>
#include <cctype>

#include "Array.hpp"

class String {
    std::string str;

    template<typename T>
    String anyToStr(T&& v) {
        std::ostringstream oss;
        oss << std::forward<T>(v);
        return String(oss.str());
    }

    String formatStrVec(const std::string& value, const Array<String>& args) {
        String out;
        size_t argCount = 0;
        for (size_t i = 0; i < value.size(); i++) {
            char c = value[i];
            if (c == '{') {
                if (i + 1 >= value.size()) throw std::runtime_error("[Neoluma/String] Invalid '{'");
                char next = value[i + 1];
                if (next == '{') { out += '{'; i++; }
                else {
                    size_t j = i + 1;

                    // {} case
                    if (value[j] == '}') {
                        if (argCount >= args.len()) throw std::runtime_error("[Neoluma/String] Not enough format arguments");
                        out += args[argCount++];
                        i = j;
                    }
                    // {number} case
                    else if (isDigit(value[j])) {
                        size_t index = 0;

                        while (j < value.size() && isDigit(value[j])) {
                            index = index * 10 + (value[j] - '0');
                            j++;
                        }

                        if (j >= value.size() || value[j] != '}') throw std::runtime_error("[Neoluma/String] Invalid positional format");
                        if (index >= args.len()) throw std::runtime_error("[Neoluma/String] Positional argument out of range");

                        out += args[index];
                        i = j;
                    }
                    else throw std::runtime_error("[Neoluma/String] Invalid '{'");
                }
            }
            else if (c == '}') {
                if (i + 1 < value.size() && value[i + 1] == '}') { out += '}'; i++; }
                else throw std::runtime_error("[Neoluma/String] Invalid '}'");
            }
            else out += c;
        }
        if (argCount < args.len()) throw std::runtime_error("[Neoluma/String] Too many format arguments");
        return out;
    }

    bool isDigit(char c) { return c >= '0' && c <= '9'; }
public:
    String();
    String(const char* value);
    String(const std::string& value);
    String(std::string_view value);
    // TODO for later: fix this, i absolutely hate how this looks
    template <typename... Args>
    String(const std::string& value, Args&&... args) {
        Array<String> collectedArgs = { anyToStr(args)... };
        formatStrVec(value, collectedArgs);
    }
    String(const std::string& value, const Array<String>& args) { formatStrVec(value, args); }

    String& operator=(const std::string& value);
    String& operator=(std::string&& value);
    String& operator=(const char* value);
    String& operator+=(const std::string& value);
    String& operator+=(const char* value);
    String& operator+=(char value);
    bool operator==(const std::string& value) const;
    bool operator!=(const std::string& value) const;
    char& operator[](std::size_t index);
    const char& operator[](std::size_t index) const;
    operator const std::string&() const;
    operator std::string&();

    std::size_t len() const;
    bool empty() const;
    void clear();

    void trim();
    Array<String> split(char delimiter) const;
};

namespace std {
    template<>
    struct hash<String> {
        size_t operator()(const String& value) const noexcept { return hash<std::string>()(value); }
    };
}