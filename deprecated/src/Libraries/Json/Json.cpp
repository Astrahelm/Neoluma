#include "Json.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <limits>
#include <cmath>
#include <cstdlib>

namespace json {

// --------- Value helpers ----------
Value* Value::find(const std::string& key) {
    if (!isObject()) return nullptr;
    auto& obj = std::get<Object>(data);
    for (auto& [k, v] : obj) if (k == key) return &v;
    return nullptr;
}
const Value* Value::find(const std::string& key) const {
    if (!isObject()) return nullptr;
    auto& obj = std::get<Object>(data);
    for (auto& [k, v] : obj) if (k == key) return &v;
    return nullptr;
}
bool Value::has(const std::string& key) const { return find(key) != nullptr; }

Value& Value::operator[](const std::string& key) {
    if (!isObject()) data = Object{};
    auto& obj = std::get<Object>(data);
    for (auto& [k, v] : obj) {
        if (k == key) return v;
    }
    obj.emplace_back(key, Value{});
    return obj.back().second;
}

const Value& Value::operator[](const std::string& key) const {
    static const Value kNull{};
    auto* p = find(key);
    return p ? *p : kNull;
}

Value& Value::at(size_t i) {
    auto& arr = std::get<Array>(data);
    if (i >= arr.size()) throw std::out_of_range("json::Value::at array index out of range");
    return arr[i];
}
const Value& Value::at(size_t i) const {
    auto& arr = std::get<Array>(data);
    if (i >= arr.size()) throw std::out_of_range("json::Value::at array index out of range");
    return arr[i];
}

// --------- UTF-8 decode (for correct unicode escaping) ----------
static bool utf8DecodeOne(std::string_view s, size_t& i, uint32_t& cp) {
    auto byte = [&](size_t j)->uint8_t { return (uint8_t)s[j]; };
    if (i >= s.size()) return false;

    uint8_t b0 = byte(i);
    if (b0 < 0x80) { cp = b0; i += 1; return true; }

    if ((b0 & 0xE0) == 0xC0) {
        if (i + 1 >= s.size()) return false;
        uint8_t b1 = byte(i+1);
        if ((b1 & 0xC0) != 0x80) return false;
        uint32_t x = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        if (x < 0x80) return false; // overlong
        cp = x; i += 2; return true;
    }

    if ((b0 & 0xF0) == 0xE0) {
        if (i + 2 >= s.size()) return false;
        uint8_t b1 = byte(i+1), b2 = byte(i+2);
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return false;
        uint32_t x = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        if (x < 0x800) return false; // overlong
        if (x >= 0xD800 && x <= 0xDFFF) return false; // surrogate invalid in UTF-8
        cp = x; i += 3; return true;
    }

    if ((b0 & 0xF8) == 0xF0) {
        if (i + 3 >= s.size()) return false;
        uint8_t b1 = byte(i+1), b2 = byte(i+2), b3 = byte(i+3);
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return false;
        uint32_t x = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        if (x < 0x10000 || x > 0x10FFFF) return false;
        cp = x; i += 4; return true;
    }

    return false;
}

static void appendHex4(std::string& out, uint16_t v) {
    static const char* hex = "0123456789ABCDEF";
    out.push_back(hex[(v >> 12) & 0xF]);
    out.push_back(hex[(v >>  8) & 0xF]);
    out.push_back(hex[(v >>  4) & 0xF]);
    out.push_back(hex[(v >>  0) & 0xF]);
}

// --------- Stringify escape ----------
static void appendEscaped(std::string& out, std::string_view s, bool escape_non_ascii) {
    auto emitCpAsEscapes = [&](uint32_t cp) {
        if (cp <= 0xFFFF) {
            out += "\\u";
            appendHex4(out, (uint16_t)cp);
        } else {
            cp -= 0x10000;
            uint16_t hi = 0xD800 + ((cp >> 10) & 0x3FF);
            uint16_t lo = 0xDC00 + (cp & 0x3FF);
            out += "\\u"; appendHex4(out, hi);
            out += "\\u"; appendHex4(out, lo);
        }
    };

    for (size_t i = 0; i < s.size();) {
        unsigned char c = (unsigned char)s[i];

        if (c < 0x80) {
            switch (c) {
                case '\"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) { out += "\\u"; appendHex4(out, (uint16_t)c); }
                    else out.push_back((char)c);
            }
            ++i;
            continue;
        }

        if (!escape_non_ascii) {
            // keep UTF-8 bytes as-is (most compatible modern behavior)
            out.push_back((char)s[i]);
            ++i;
            continue;
        }

        uint32_t cp = 0;
        size_t save = i;
        if (!utf8DecodeOne(s, i, cp)) {
            // invalid sequence -> escape raw byte
            i = save + 1;
            out += "\\u";
            appendHex4(out, (uint16_t)(uint8_t)s[save]);
            continue;
        }
        emitCpAsEscapes(cp);
    }
}

static void indent(std::string& out, int n) {
    out.append((size_t)n, ' ');
}

// --------- Parser ----------
class Parser {
public:
    Parser(std::string_view s, ParseOptions opt)
        : src(s), options(opt) {
        if (options.allow_bom && src.size() >= 3 &&
            (unsigned char)src[0] == 0xEF &&
            (unsigned char)src[1] == 0xBB &&
            (unsigned char)src[2] == 0xBF) {
            pos = 3;
        }
    }

    Value parseRoot() {
        skipWsAndCommentsCollect(pending_before);
        Value v = parseValue();

        // If we collected comments before root, attach
        if (!pending_before.empty()) {
            v.comments_before.insert(v.comments_before.begin(), pending_before.begin(), pending_before.end());
            pending_before.clear();
        }

        skipWsAndCommentsCollect(pending_before);
        if (pos != src.size()) error("Unexpected trailing characters");
        return v;
    }

private:
    std::string_view src;
    ParseOptions options{};
    size_t pos{0};
    size_t line{1};
    size_t col{1};

    // comments collected between separators and next value
    std::vector<std::string> pending_before;

    [[noreturn]] void error(const std::string& msg) const {
        throw ParseError(msg, pos, line, col);
    }

    char peek() const { return (pos < src.size()) ? src[pos] : '\0'; }

    char get() {
        if (pos >= src.size()) return '\0';
        char c = src[pos++];
        if (c == '\n') { line++; col = 1; }
        else { col++; }
        return c;
    }

    bool consume(char c) {
        if (peek() == c) { get(); return true; }
        return false;
    }

    void expect(char c, const char* msg) {
        if (!consume(c)) error(msg);
    }

    static bool isLineBreak(char c) { return c == '\n' || c == '\r'; }

    void skipWhitespaceOnly() {
        while (true) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { get(); continue; }
            break;
        }
    }

    void skipWsAndCommentsCollect(std::vector<std::string>& outComments) {
        while (true) {
            skipWhitespaceOnly();
            if (!options.allow_comments) return;

            // line comment //
            if (peek() == '/' && pos + 1 < src.size() && src[pos + 1] == '/') {
                get(); get();
                std::string text;
                while (pos < src.size() && !isLineBreak(peek())) text.push_back(get());
                outComments.push_back(trim(text));
                continue;
            }

            // block comment /* ... */
            if (peek() == '/' && pos + 1 < src.size() && src[pos + 1] == '*') {
                get(); get();
                std::string text;
                while (true) {
                    if (pos >= src.size()) error("Unterminated block comment");
                    if (peek() == '*' && pos + 1 < src.size() && src[pos + 1] == '/') {
                        get(); get();
                        break;
                    }
                    text.push_back(get());
                }
                outComments.push_back(trim(text));
                continue;
            }

            return;
        }
    }

    bool startsWith(std::string_view w) const {
        return src.substr(pos, w.size()) == w;
    }

    Value parseValue() {
        // collect leading comments for this value
        std::vector<std::string> leading;
        skipWsAndCommentsCollect(leading);

        char c = peek();
        Value v;

        if (c == '{') v = Value(parseObject());
        else if (c == '[') v = Value(parseArray());
        else if (c == '"' || (options.allow_single_quotes && c == '\'')) v = Value(parseString());
        else if (c == '-' || (c >= '0' && c <= '9')) v = parseNumber();
        else if (startsWith("true"))  { pos += 4; col += 4; v = Value(true); }
        else if (startsWith("false")) { pos += 5; col += 5; v = Value(false); }
        else if (startsWith("null"))  { pos += 4; col += 4; v = Value(nullptr); }
        else error("Expected a JSON value");

        v.comments_before = std::move(leading);

        // try to attach trailing comment
        size_t savePos = pos, saveLine = line, saveCol = col;

        auto skipSpacesTabs = [&]() {
            while (peek() == ' ' || peek() == '\t') get();
        };

        skipSpacesTabs();

        if (options.allow_comments && peek() == '/' && pos + 1 < src.size()) {
            if (src[pos + 1] == '/') {
                get(); get();
                std::string text;
                while (pos < src.size() && !isLineBreak(peek())) text.push_back(get());
                v.comments_after = trim(text);
            } else if (src[pos + 1] == '*') {
                get(); get();
                std::string text;
                while (true) {
                    if (pos >= src.size()) error("Unterminated block comment");
                    if (peek() == '*' && pos + 1 < src.size() && src[pos + 1] == '/') {
                        get(); get();
                        break;
                    }
                    text.push_back(get());
                }
                v.comments_after = trim(text);
            } else {
                pos = savePos; line = saveLine; col = saveCol;
            }
        } else {
            pos = savePos; line = saveLine; col = saveCol;
        }

        return v;
    }

    std::string parseString() {
        char quote = peek();
        if (quote != '"' && quote != '\'') error("Expected string quote");
        if (quote == '\'' && !options.allow_single_quotes) error("Single-quoted strings are disabled");
        get(); // consume quote

        std::string out;
        while (true) {
            if (pos >= src.size()) error("Unterminated string");
            char c = get();
            if (c == quote) break;

            if (c == '\\') {
                if (pos >= src.size()) error("Bad escape");
                char e = get();
                switch (e) {
                    case '"': out.push_back('"'); break;
                    case '\'': out.push_back('\''); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        auto hex = [&](char h)->int{
                            if (h >= '0' && h <= '9') return h - '0';
                            if (h >= 'a' && h <= 'f') return 10 + (h - 'a');
                            if (h >= 'A' && h <= 'F') return 10 + (h - 'A');
                            return -1;
                        };
                        if (pos + 4 > src.size()) error("Invalid \\u escape");
                        int v0 = hex(get()), v1 = hex(get()), v2 = hex(get()), v3 = hex(get());
                        if (v0 < 0 || v1 < 0 || v2 < 0 || v3 < 0) error("Invalid \\u escape");
                        uint32_t cp = (uint32_t)((v0<<12) | (v1<<8) | (v2<<4) | v3);

                        // surrogate pair handling
                        if (cp >= 0xD800 && cp <= 0xDBFF) {
                            size_t pSave = pos; size_t lSave = line; size_t cSave = col;
                            if (peek() == '\\' && pos+1 < src.size() && src[pos+1] == 'u') {
                                get(); get();
                                if (pos + 4 > src.size()) error("Invalid \\u escape");
                                int w0 = hex(get()), w1 = hex(get()), w2 = hex(get()), w3 = hex(get());
                                if (w0 < 0 || w1 < 0 || w2 < 0 || w3 < 0) error("Invalid \\u escape");
                                uint32_t lo = (uint32_t)((w0<<12) | (w1<<8) | (w2<<4) | w3);
                                if (lo >= 0xDC00 && lo <= 0xDFFF) {
                                    uint32_t hi = cp - 0xD800;
                                    lo -= 0xDC00;
                                    cp = 0x10000 + ((hi << 10) | lo);
                                } else {
                                    pos = pSave; line = lSave; col = cSave;
                                }
                            }
                        }

                        // encode UTF-8
                        if (cp <= 0x7F) out.push_back((char)cp);
                        else if (cp <= 0x7FF) {
                            out.push_back((char)(0xC0 | ((cp >> 6) & 0x1F)));
                            out.push_back((char)(0x80 | (cp & 0x3F)));
                        } else if (cp <= 0xFFFF) {
                            out.push_back((char)(0xE0 | ((cp >> 12) & 0x0F)));
                            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                            out.push_back((char)(0x80 | (cp & 0x3F)));
                        } else {
                            out.push_back((char)(0xF0 | ((cp >> 18) & 0x07)));
                            out.push_back((char)(0x80 | ((cp >> 12) & 0x3F)));
                            out.push_back((char)(0x80 | ((cp >> 6) & 0x3F)));
                            out.push_back((char)(0x80 | (cp & 0x3F)));
                        }
                        break;
                    }
                    default:
                        error("Unknown escape sequence");
                }
            } else {
                if ((unsigned char)c < 0x20) error("Control character in string");
                out.push_back(c);
            }
        }
        return out;
    }

    Value parseNumber() {
        size_t start = pos;
        if (peek() == '-') get();

        if (peek() == '0') {
            get();
        } else {
            if (!(peek() >= '1' && peek() <= '9')) error("Invalid number");
            while (peek() >= '0' && peek() <= '9') get();
        }

        bool isFloat = false;
        if (peek() == '.') {
            isFloat = true;
            get();
            if (!(peek() >= '0' && peek() <= '9')) error("Invalid fraction");
            while (peek() >= '0' && peek() <= '9') get();
        }

        if (peek() == 'e' || peek() == 'E') {
            isFloat = true;
            get();
            if (peek() == '+' || peek() == '-') get();
            if (!(peek() >= '0' && peek() <= '9')) error("Invalid exponent");
            while (peek() >= '0' && peek() <= '9') get();
        }

        std::string_view sv = src.substr(start, pos - start);

        if (!isFloat) {
            std::int64_t val = 0;
            bool neg = false;
            size_t i = 0;
            if (!sv.empty() && sv[0] == '-') { neg = true; i = 1; }
            for (; i < sv.size(); ++i) {
                char c = sv[i];
                if (c < '0' || c > '9') error("Invalid integer");
                int digit = c - '0';
                if (!neg) {
                    if (val > (std::numeric_limits<std::int64_t>::max() - digit) / 10) {
                        double d = std::strtod(std::string(sv).c_str(), nullptr);
                        if (!std::isfinite(d)) error("Non-finite number not allowed");
                        return Value(d);
                    }
                    val = val * 10 + digit;
                } else {
                    if (val < (std::numeric_limits<std::int64_t>::min() + digit) / 10) {
                        double d = std::strtod(std::string(sv).c_str(), nullptr);
                        if (!std::isfinite(d)) error("Non-finite number not allowed");
                        return Value(d);
                    }
                    val = val * 10 - digit;
                }
            }
            return Value(val);
        }

        double d = std::strtod(std::string(sv).c_str(), nullptr);
        if (!std::isfinite(d)) error("Non-finite number not allowed");
        return Value(d);
    }

    Array parseArray() {
        expect('[', "Expected '['");
        Array arr;

        skipWsAndCommentsCollect(pending_before);
        if (consume(']')) return arr;

        while (true) {
            Value v = parseValue();
            if (!pending_before.empty()) {
                v.comments_before.insert(v.comments_before.begin(), pending_before.begin(), pending_before.end());
                pending_before.clear();
            }
            arr.push_back(std::move(v));

            skipWsAndCommentsCollect(pending_before);

            if (consume(',')) {
                skipWsAndCommentsCollect(pending_before);
                if (options.allow_trailing_commas && peek() == ']') { get(); break; }
                continue;
            }

            if (consume(']')) break;
            error("Expected ',' or ']'");
        }

        return arr;
    }

    Object parseObject() {
        expect('{', "Expected '{'");
        Object obj;

        skipWsAndCommentsCollect(pending_before);
        if (consume('}')) return obj;

        while (true) {
            skipWsAndCommentsCollect(pending_before);

            char c = peek();
            if (!(c == '"' || (options.allow_single_quotes && c == '\'')))
                error("Object keys must be strings");

            std::string key = parseString();

            skipWsAndCommentsCollect(pending_before);
            expect(':', "Expected ':' after key");

            Value val = parseValue();
            if (!pending_before.empty()) {
                val.comments_before.insert(val.comments_before.begin(), pending_before.begin(), pending_before.end());
                pending_before.clear();
            }

            // duplicate key policy: last-wins by default
            bool replaced = false;
            for (auto& [k, existing] : obj) {
                if (k == key) {
                    if (!options.duplicate_keys_last_wins) {
                        error("Duplicate key in object: " + key);
                    }
                    existing = std::move(val);
                    replaced = true;
                    break;
                }
            }
            if (!replaced) obj.emplace_back(std::move(key), std::move(val));

            skipWsAndCommentsCollect(pending_before);

            if (consume(',')) {
                skipWsAndCommentsCollect(pending_before);
                if (options.allow_trailing_commas && peek() == '}') { get(); break; }
                continue;
            }

            if (consume('}')) break;
            error("Expected ',' or '}'");
        }

        return obj;
    }
};

// --------- Stringify ----------
static void emitCommentsBefore(std::string& out, const Value& v, const StringifyOptions& opt, int depth) {
    if (!opt.emit_comments) return;
    for (const auto& c : v.comments_before) {
        if (opt.pretty) indent(out, depth);
        out += "// ";
        out += c;
        out += opt.pretty ? "\n" : "";
    }
}

static void emitCommentAfter(std::string& out, const Value& v, const StringifyOptions& opt) {
    if (!opt.emit_comments) return;
    if (!v.comments_after.empty()) {
        out += opt.pretty ? " " : "";
        out += "/* ";
        out += v.comments_after;
        out += " */";
    }
}

static void stringifyImpl(std::string& out, const Value& v, const StringifyOptions& opt, int depth);

static void stringifyArray(std::string& out, const Array& arr, const StringifyOptions& opt, int depth) {
    out += "[";
    if (arr.empty()) { out += "]"; return; }

    if (opt.pretty) out += "\n";
    for (size_t i = 0; i < arr.size(); ++i) {
        const auto& el = arr[i];
        emitCommentsBefore(out, el, opt, depth + opt.indent);
        if (opt.pretty) indent(out, depth + opt.indent);

        stringifyImpl(out, el, opt, depth + opt.indent);
        emitCommentAfter(out, el, opt);

        if (i + 1 < arr.size()) out += ",";
        out += opt.pretty ? "\n" : "";
    }
    if (opt.pretty) indent(out, depth);
    out += "]";
}

static void stringifyObject(std::string& out, const Object& obj, const StringifyOptions& opt, int depth) {
    out += "{";
    if (obj.empty()) { out += "}"; return; }

    if (opt.pretty) out += "\n";

    std::vector<size_t> idx(obj.size());
    for (size_t i = 0; i < obj.size(); ++i) idx[i] = i;

    if (opt.sort_keys) {
        std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b){
            return obj[a].first < obj[b].first;
        });
    }

    for (size_t n = 0; n < idx.size(); ++n) {
        const auto& [k, val] = obj[idx[n]];

        emitCommentsBefore(out, val, opt, depth + opt.indent);
        if (opt.pretty) indent(out, depth + opt.indent);

        out += "\"";
        appendEscaped(out, k, opt.escape_non_ascii);
        out += "\":";
        out += opt.pretty ? " " : "";

        stringifyImpl(out, val, opt, depth + opt.indent);
        emitCommentAfter(out, val, opt);

        if (n + 1 < idx.size()) out += ",";
        out += opt.pretty ? "\n" : "";
    }

    if (opt.pretty) indent(out, depth);
    out += "}";
}

static void stringifyImpl(std::string& out, const Value& v, const StringifyOptions& opt, int depth) {
    if (v.isNull()) { out += "null"; return; }
    if (v.isBool()) { out += (v.asBool() ? "true" : "false"); return; }
    if (v.isInt()) { out += std::to_string(v.asInt()); return; }
    if (v.isDouble()) {
        std::ostringstream oss;
        oss.setf(std::ios::fmtflags(0), std::ios::floatfield);
        oss << std::setprecision(17) << v.asDouble();
        out += oss.str();
        return;
    }
    if (v.isString()) {
        out += "\"";
        appendEscaped(out, v.asString(), opt.escape_non_ascii);
        out += "\"";
        return;
    }
    if (v.isArray())  { stringifyArray(out, v.asArray(), opt, depth); return; }
    if (v.isObject()) { stringifyObject(out, v.asObject(), opt, depth); return; }
}

// --------- Public API ----------
Value parse(std::string_view text, const ParseOptions& opt) {
    Parser p(text, opt);
    return p.parseRoot();
}

Value parseFile(const std::string& filePath, const ParseOptions& opt) {
    return parse(readFile(filePath), opt);
}

std::string stringify(const Value& v, const StringifyOptions& opt) {
    std::string out;
    emitCommentsBefore(out, v, opt, 0);
    stringifyImpl(out, v, opt, 0);
    emitCommentAfter(out, v, opt);
    if (opt.pretty) out += "\n";
    return out;
}

void writeFile(const std::string& filePath, const Value& v, const StringifyOptions& opt) {
    std::ofstream f(filePath, std::ios::binary);
    if (!f) throw std::runtime_error("json::writeFile: cannot open file for writing: " + filePath);
    auto s = stringify(v, opt);
    f.write(s.data(), (std::streamsize)s.size());
    if (!f) throw std::runtime_error("json::writeFile: write failed: " + filePath);
}

} // namespace json
