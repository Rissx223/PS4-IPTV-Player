// ============================================================================
//  json.h - Minimal, dependency-free JSON parser (C++).
//
//  Purpose-built for parsing Xtream Codes API responses (objects, arrays,
//  strings, numbers, booleans, null). Not a full validator: it is lenient and
//  aims to extract fields robustly from real-world panel output. Xtream panels
//  often return numeric ids as either JSON numbers or quoted strings, so
//  JsonValue::asString() coerces numbers to text for convenience.
// ============================================================================
#ifndef PS4_IPTV_JSON_H
#define PS4_IPTV_JSON_H

#include <string>
#include <vector>
#include <memory>

class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;

    bool                                                   boolValue = false;
    double                                                 numberValue = 0.0;
    std::string                                            stringValue;
    std::vector<JsonValue>                                 arrayValue;
    std::vector<std::pair<std::string, JsonValue>>         objectValue;

    bool isNull()   const { return type == Type::Null; }
    bool isArray()  const { return type == Type::Array; }
    bool isObject() const { return type == Type::Object; }

    // Object lookup; returns a static Null value when absent.
    const JsonValue &operator[](const std::string &key) const;
    // Array access; returns a static Null value when out of range.
    const JsonValue &at(size_t i) const;
    size_t size() const;

    // Coercions that tolerate string/number ambiguity from panels.
    std::string asString() const;
    long        asLong(long fallback = 0) const;
    bool        asBool(bool fallback = false) const;
};

// Parse a JSON document. Returns false on hard syntax errors (out is Null).
bool json_parse(const std::string &text, JsonValue &out);

#endif // PS4_IPTV_JSON_H
