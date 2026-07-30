#include "json.h"
#include <cstdlib>
#include <cstdio>

static const JsonValue kNull;

const JsonValue &JsonValue::operator[](const std::string &key) const
{
    if (type == Type::Object) {
        for (const auto &kv : objectValue)
            if (kv.first == key)
                return kv.second;
    }
    return kNull;
}

const JsonValue &JsonValue::at(size_t i) const
{
    if (type == Type::Array && i < arrayValue.size())
        return arrayValue[i];
    return kNull;
}

size_t JsonValue::size() const
{
    if (type == Type::Array)  return arrayValue.size();
    if (type == Type::Object) return objectValue.size();
    return 0;
}

std::string JsonValue::asString() const
{
    switch (type) {
        case Type::String: return stringValue;
        case Type::Bool:   return boolValue ? "true" : "false";
        case Type::Number: {
            // Render integers without a trailing ".000000".
            double r = numberValue;
            char buf[64];
            if (r == (long long)r)
                snprintf(buf, sizeof(buf), "%lld", (long long)r);
            else
                snprintf(buf, sizeof(buf), "%g", r);
            return buf;
        }
        default: return std::string();
    }
}

long JsonValue::asLong(long fallback) const
{
    if (type == Type::Number) return (long)numberValue;
    if (type == Type::String) {
        if (stringValue.empty()) return fallback;
        char *end = nullptr;
        long v = strtol(stringValue.c_str(), &end, 10);
        if (end == stringValue.c_str()) return fallback;
        return v;
    }
    if (type == Type::Bool) return boolValue ? 1 : 0;
    return fallback;
}

bool JsonValue::asBool(bool fallback) const
{
    if (type == Type::Bool)   return boolValue;
    if (type == Type::Number) return numberValue != 0.0;
    if (type == Type::String) return stringValue == "true" || stringValue == "1";
    return fallback;
}

// --------------------------------------------------------------------------
//  Recursive descent parser
// --------------------------------------------------------------------------
namespace {

struct Parser {
    const char *p;
    const char *end;

    void skipWs() {
        while (p < end) {
            char c = *p;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                p++;
            else
                break;
        }
    }

    bool parseValue(JsonValue &out);

    bool parseString(std::string &s) {
        if (p >= end || *p != '"') return false;
        p++;
        s.clear();
        while (p < end) {
            char c = *p++;
            if (c == '"') return true;
            if (c == '\\') {
                if (p >= end) return false;
                char e = *p++;
                switch (e) {
                    case '"':  s.push_back('"');  break;
                    case '\\': s.push_back('\\'); break;
                    case '/':  s.push_back('/');  break;
                    case 'b':  s.push_back('\b'); break;
                    case 'f':  s.push_back('\f'); break;
                    case 'n':  s.push_back('\n'); break;
                    case 'r':  s.push_back('\r'); break;
                    case 't':  s.push_back('\t'); break;
                    case 'u': {
                        if (end - p < 4) return false;
                        unsigned code = 0;
                        for (int i = 0; i < 4; i++) {
                            char h = *p++;
                            code <<= 4;
                            if (h >= '0' && h <= '9') code |= (h - '0');
                            else if (h >= 'a' && h <= 'f') code |= (h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') code |= (h - 'A' + 10);
                            else return false;
                        }
                        // Encode BMP code point as UTF-8 (surrogate pairs are
                        // uncommon in panel data and rendered as-is).
                        if (code < 0x80) {
                            s.push_back((char)code);
                        } else if (code < 0x800) {
                            s.push_back((char)(0xC0 | (code >> 6)));
                            s.push_back((char)(0x80 | (code & 0x3F)));
                        } else {
                            s.push_back((char)(0xE0 | (code >> 12)));
                            s.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
                            s.push_back((char)(0x80 | (code & 0x3F)));
                        }
                        break;
                    }
                    default: return false;
                }
            } else {
                s.push_back(c);
            }
        }
        return false; // unterminated
    }

    bool parseNumber(JsonValue &out) {
        const char *start = p;
        if (p < end && (*p == '-' || *p == '+')) p++;
        bool any = false;
        while (p < end && ((*p >= '0' && *p <= '9') || *p == '.' ||
                           *p == 'e' || *p == 'E' || *p == '+' || *p == '-')) {
            p++;
            any = true;
        }
        if (!any) return false;
        std::string num(start, p - start);
        out.type = JsonValue::Type::Number;
        out.numberValue = strtod(num.c_str(), nullptr);
        return true;
    }

    bool literal(const char *lit) {
        size_t n = 0;
        while (lit[n]) n++;
        if ((size_t)(end - p) < n) return false;
        for (size_t i = 0; i < n; i++)
            if (p[i] != lit[i]) return false;
        p += n;
        return true;
    }
};

bool Parser::parseValue(JsonValue &out)
{
    skipWs();
    if (p >= end) return false;
    char c = *p;

    if (c == '"') {
        std::string s;
        if (!parseString(s)) return false;
        out.type = JsonValue::Type::String;
        out.stringValue = std::move(s);
        return true;
    }
    if (c == '{') {
        p++;
        out.type = JsonValue::Type::Object;
        skipWs();
        if (p < end && *p == '}') { p++; return true; }
        for (;;) {
            skipWs();
            std::string key;
            if (!parseString(key)) return false;
            skipWs();
            if (p >= end || *p != ':') return false;
            p++;
            JsonValue val;
            if (!parseValue(val)) return false;
            out.objectValue.emplace_back(std::move(key), std::move(val));
            skipWs();
            if (p >= end) return false;
            if (*p == ',') { p++; continue; }
            if (*p == '}') { p++; return true; }
            return false;
        }
    }
    if (c == '[') {
        p++;
        out.type = JsonValue::Type::Array;
        skipWs();
        if (p < end && *p == ']') { p++; return true; }
        for (;;) {
            JsonValue val;
            if (!parseValue(val)) return false;
            out.arrayValue.push_back(std::move(val));
            skipWs();
            if (p >= end) return false;
            if (*p == ',') { p++; continue; }
            if (*p == ']') { p++; return true; }
            return false;
        }
    }
    if (c == 't') { if (literal("true"))  { out.type = JsonValue::Type::Bool; out.boolValue = true;  return true; } return false; }
    if (c == 'f') { if (literal("false")) { out.type = JsonValue::Type::Bool; out.boolValue = false; return true; } return false; }
    if (c == 'n') { if (literal("null"))  { out.type = JsonValue::Type::Null; return true; } return false; }

    return parseNumber(out);
}

} // namespace

bool json_parse(const std::string &text, JsonValue &out)
{
    out = JsonValue();
    Parser parser;
    parser.p   = text.c_str();
    parser.end = text.c_str() + text.size();
    if (!parser.parseValue(out)) {
        out = JsonValue();
        return false;
    }
    return true;
}
