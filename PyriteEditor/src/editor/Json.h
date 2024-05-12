#pragma once

#include <array>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <filesystem>

#include "utils/debug.h"

/*
 * Generic JSON parser, there is little (no) error logging but the
 * parser is still safe. By discarding error reporting facilities
 * the parser can be made blazingly fast.
 *
 * JsonObject can be copied but copy should be avoided whenever
 * possible. Deep json copy is not cheap.
 */

class JsonObject;
class JsonValue;

namespace json {

using number_type = double;
using array_type = std::vector<JsonValue>;
using variant_type = std::variant<double, JsonObject, array_type, std::string, bool>;
// set this flag on an output stream to write pretty json
// boolalpha is used as a workarround because we cannot create new stream flags. We handle "true" and "false" oursleves anyway
static constexpr auto pretty_format = std::boolalpha;

JsonValue parse(const std::string &text);
JsonValue parseFile(const std::filesystem::path &filepath);
JsonValue parse(std::istream &is);

}

class JsonObject {
public:
  using array_type = json::array_type;
  using number_type = json::number_type;

  number_type getNumber(const std::string &name) const { return get<number_type>(name); }
  int getInt(const std::string &name) const { return static_cast<int>(getNumber(name)); }
  float getFloat(const std::string &name) const { return static_cast<float>(getNumber(name)); }
  double getDouble(const std::string &name) const { return static_cast<double>(getNumber(name)); }
  const std::string &getString(const std::string &name) const { return get<std::string>(name); }
  bool getBool(const std::string &name) const { return get<bool>(name); }
  const JsonObject &getObject(const std::string &name) const { return get<JsonObject>(name); }
  const array_type &getArray(const std::string &name) const { return get<array_type>(name); }
  template<class T>
  const T &get(const std::string &name) const;

  const JsonValue &operator[](const std::string &name) const;

  void set(const std::string &name, JsonValue &&val);

  const std::unordered_map<std::string, JsonValue> &getFields() const { return m_fields; }
  bool hasField(const std::string &name) const { return m_fields.contains(name); }

private:
  std::unordered_map<std::string, JsonValue> m_fields;
};

class JsonValue {
public:
  using array_type = json::array_type;
  using number_type = json::number_type;
  using variant_type = json::variant_type;

  template<class T>
    requires std::is_same_v<T, JsonObject>
          or std::is_same_v<T, array_type>
  JsonValue(T &&val)
    : m_value(std::forward<T>(val))
  {}

  template<class T>
    requires std::is_same_v<T, std::string>
  JsonValue(T copiedVal)
    : m_value(std::move(copiedVal))
  {}
  
  template<class T>
    requires std::is_floating_point_v<T>
          or std::is_integral_v<T>
          /* bool is an integral type */
  JsonValue(T numVal)
    : m_value(static_cast<number_type>(numVal))
  {}

  number_type asNumber() const { return std::get<number_type>(m_value); }
  const std::string &asString() const { return std::get<std::string>(m_value); }
  const JsonObject &asObject() const { return std::get<JsonObject>(m_value); }
  const array_type &asArray() const { return std::get<array_type>(m_value); }
  bool asBool() const { return std::get<bool>(m_value); }
  template<class T>
  const T &as() const { return std::get<T>(m_value); }

  const variant_type &getVariant() const { return m_value; }

  const JsonValue &operator[](const std::string& fieldName) const { return asObject()[fieldName]; }

private:
  variant_type m_value;
};

struct json_parser_error : std::runtime_error {
  explicit json_parser_error(const std::string& message) : std::runtime_error(message) {}
  json_parser_error(const std::string &message, char problematicChar) : json_parser_error(message + ", got: " + std::to_string(problematicChar)) {}
};



std::ostream &operator<<(std::ostream &os, const JsonValue &val);
inline std::istream &operator>>(std::istream &is, JsonValue &val) { val = json::parse(is); return is; }

inline void JsonObject::set(const std::string& name, JsonValue&& val)
{ m_fields.emplace(name, std::move(val)); }

template <class T>
const T& JsonObject::get(const std::string& name) const
{
#ifdef PYR_ISDEBUG
  PYR_ENSURE(m_fields.contains(name), "Missing field in json: ", name);
  try {
    return m_fields.at(name).as<T>();
  } catch (const std::bad_variant_access&) {
    throw json_parser_error("Invalid type for json field " + name + ", expected " + typeid(T).name());
  }
#else
  return m_fields.at(name).as<T>();
#endif
}

inline const JsonValue& JsonObject::operator[](const std::string &name) const
{
#ifdef PYR_ISDEBUG
  PYR_ENSURE(m_fields.contains(name), "Missing field in json: ", name);
  return m_fields.at(name);
#else
  return m_fields.at(name);
#endif
}