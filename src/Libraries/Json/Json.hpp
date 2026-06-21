/*
 * Json is an internal Neoluma library that allows to read or write JSON data (with comments)

Quick usage

1) Parse JSON / JSONC
--------------------
json::Value root = json::parse(text);
// or
json::Value root = json::parseFile("config.jsonc");

Throws json::ParseError on invalid input (never loops or hangs).

2) Read values (safe pattern)
-----------------------------
if (root.isObject()) {
    std::string name = root["name"].isString()
        ? root["name"].asString()
        : "default";

    int w = (root["window"].isObject() && root["window"]["w"].isInt())
        ? (int)root["window"]["w"].asInt()
        : 800;
}

3) Modify / create values
-------------------------
root["enabled"] = true;
root["count"] = (std::int64_t)42;
root["list"] = json::Array{ 1, 2, 3 };

json::Object obj;
obj.emplace_back("a", 1);
obj.emplace_back("b", "text");
root["sub"] = obj;

4) Comments
-----------
root.comments_before.push_back("Root comment");
root["enabled"].comments_before.push_back("Toggle feature");
root["enabled"].comments_after = "do not touch";

5) Write back
-------------
std::string out = json::stringify(root, {
    .pretty = true,
    .emit_comments = true
});

// or
json::writeFile("out.jsonc", root);

Notes:
- Supports // and /.* *./ comments (without dots)
- Supports trailing commas
- Supports single-quoted strings
- Duplicate keys: last one wins
- Always throws on invalid JSON (no infinite loops)
*/

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <stdexcept>
#include <algorithm>

#include "HelperFunctions.hpp" // provides: trim, split, readFile

namespace json {

// ---------- Error ----------
struct ParseError : std::runtime_error {
    size_t offset{}, line{}, col{};
    ParseError(const std::string& msg, size_t off, size_t ln, size_t cl)
        : std::runtime_error(msg), offset(off), line(ln), col(cl) {}
};

// ---------- Value ----------
struct Value;

using Array  = std::vector<Value>;
using Object = std::vector<std::pair<std::string, Value>>;

struct Value {
    // Comment strings attached to this node
    std::vector<std::string> comments_before;
    std::string comments_after;

    using Storage = std::variant<std::monostate, bool, std::int64_t, double, std::string, Array, Object>;
    Storage data;

    // Constructors
    Value() : data(std::monostate{}) {}
    Value(std::nullptr_t) : data(std::monostate{}) {}
    Value(bool b) : data(b) {}
    Value(std::int64_t i) : data(i) {}
    Value(double d) : data(d) {}
    Value(std::string s) : data(std::move(s)) {}
    Value(const char* s) : data(std::string(s)) {}
    Value(Array a) : data(std::move(a)) {}
    Value(Object o) : data(std::move(o)) {}

    // Type checks
    bool isNull()   const { return std::holds_alternative<std::monostate>(data); }
    bool isBool()   const { return std::holds_alternative<bool>(data); }
    bool isInt()    const { return std::holds_alternative<std::int64_t>(data); }
    bool isDouble() const { return std::holds_alternative<double>(data); }
    bool isNumber() const { return isInt() || isDouble(); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isArray()  const { return std::holds_alternative<Array>(data); }
    bool isObject() const { return std::holds_alternative<Object>(data); }

    // Access (throws std::bad_variant_access on mismatch)
    bool&              asBool()   { return std::get<bool>(data); }
    std::int64_t&      asInt()    { return std::get<std::int64_t>(data); }
    double&            asDouble() { return std::get<double>(data); }
    std::string&       asString() { return std::get<std::string>(data); }
    Array&             asArray()  { return std::get<Array>(data); }
    Object&            asObject() { return std::get<Object>(data); }

    const bool&        asBool()   const { return std::get<bool>(data); }
    const std::int64_t&asInt()    const { return std::get<std::int64_t>(data); }
    const double&      asDouble() const { return std::get<double>(data); }
    const std::string& asString() const { return std::get<std::string>(data); }
    const Array&       asArray()  const { return std::get<Array>(data); }
    const Object&      asObject() const { return std::get<Object>(data); }

    // Ordered object helpers
    Value*       find(const std::string& key);
    const Value* find(const std::string& key) const;
    bool         has(const std::string& key) const;

    // Convenience indexing
    Value& operator[](const std::string& key);             // creates if missing
    const Value& operator[](const std::string& key) const; // returns null Value if missing

    // Array indexing
    Value& at(size_t i);
    const Value& at(size_t i) const;
};

// ---------- Options ----------
struct ParseOptions {
    bool allow_comments = true;
    bool allow_trailing_commas = true;   // [1,2,] { "a":1, }
    bool allow_bom = true;               // UTF-8 BOM
    bool allow_single_quotes = true;
    bool duplicate_keys_last_wins = true;
};

struct StringifyOptions {
    bool pretty = true;
    int indent = 2;
    bool emit_comments = true;
    bool escape_non_ascii = false; // if true, escape real Unicode as \uXXXX / surrogate pairs
    bool sort_keys = false;        // optional stable output
};

// ---------- API ----------
Value parse(std::string_view text, const ParseOptions& opt = {});
Value parseFile(const std::string& filePath, const ParseOptions& opt = {});

std::string stringify(const Value& v, const StringifyOptions& opt = {});
void writeFile(const std::string& filePath, const Value& v, const StringifyOptions& opt = {});

} // namespace json
