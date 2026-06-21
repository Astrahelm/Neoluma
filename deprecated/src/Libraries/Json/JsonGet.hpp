#pragma once
#include "Json.hpp"

namespace json {

    // -------- Safe getters --------
    inline const Value* get(const Value& v, const std::string& key) {
        if (!v.isObject()) return nullptr;
        return v.find(key);
    }

    inline const Value* get(const Value& v, size_t index) {
        if (!v.isArray()) return nullptr;
        if (index >= v.asArray().size()) return nullptr;
        return &v.asArray()[index];
    }

    inline std::string getString(const Value& v, const std::string& key, std::string def = {}) {
        auto* p = get(v, key);
        return (p && p->isString()) ? p->asString() : def;
    }

    inline std::int64_t getInt(const Value& v, const std::string& key, std::int64_t def = 0) {
        auto* p = get(v, key);
        if (!p) return def;
        if (p->isInt()) return p->asInt();
        if (p->isDouble()) return (std::int64_t)p->asDouble();
        return def;
    }

    inline bool getBool(const Value& v, const std::string& key, bool def = false) {
        auto* p = get(v, key);
        return (p && p->isBool()) ? p->asBool() : def;
    }

} // namespace json
