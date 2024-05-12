#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class JsonObject;
class JsonValue;
using JsonArray = std::vector<JsonValue>;
class JsonView;

namespace json {

using number_type = double;
using variant_type = std::variant<double, JsonObject, JsonArray, std::string, bool>;

template<class T>
concept is_json_type = requires(variant_type v) { { std::get<T>(v) }; };
template<class T>
concept is_json_key = std::is_integral_v<T> || std::is_convertible_v<T, std::string>;

//// Format flag that can be passed to any output stream before writing a json value,
//// if the flag is set, json will be printed using new lines and indentation
//// The difference between pretty_print and force_pretty_print is that without 'force'
//// newlines are not added for small objects ({ "a": 3 } will take a single line)
//// Note that all other format flags are ignored while printing json
//// ie. std::cout << json::pretty_print_json << myJsonValue;
//static constexpr auto pretty_print_json = std::showpos;
//static constexpr auto pretty_print_json_flag = std::ios::showpos;
//static constexpr auto force_pretty_print_json = std::skipws;
//static constexpr auto force_pretty_print_json_flag = std::ios::skipws;

struct pretty_print {
  const JsonValue& val;
  explicit pretty_print(JsonValue& val) : val(val) {}
};
struct force_pretty_print {
  const JsonValue& val;
  explicit force_pretty_print(JsonValue& val) : val(val) {}
};

JsonValue parse(const std::string &text);
JsonValue parse(const std::filesystem::path &filepath);

}

struct json_parser_error : std::runtime_error {
  explicit json_parser_error(const std::string& message) : std::runtime_error(message) {}
  json_parser_error(const std::string &message, char problematicChar) : json_parser_error(message + ", got: " + std::to_string(problematicChar)) {}
};

struct json_use_error : std::runtime_error {
  explicit json_use_error(const std::string& message) : std::runtime_error(message) {}
};

class JsonObject {
public:
  using number_type = json::number_type;

  template<class T> requires json::is_json_type<T>
  const T& get(const std::string& name) const;
  void set(const std::string& name, JsonValue&& val);

  const std::unordered_map<std::string, JsonValue> &getFields() const { return m_fields; }
  bool hasField(const std::string& name) const { return m_fields.contains(name); }
  const JsonValue& getField(const std::string& name) const { return m_fields.at(name); }
  JsonValue& getField(const std::string& name) { return m_fields.at(name); }

private:
  std::unordered_map<std::string, JsonValue> m_fields;
};

class JsonValue {
public:
  using number_type = json::number_type;
  using variant_type = json::variant_type;

  template<class T> requires std::is_same_v<T, JsonObject> or std::is_same_v<T, JsonArray>
  JsonValue(T &&jsonVal) : m_value(std::forward<T>(jsonVal)) {}
  JsonValue(std::string stringVal) : m_value(std::move(stringVal)) {}
  JsonValue(const char* stringVal) : m_value(std::string(stringVal)) {}
  template<class T> requires (std::is_floating_point_v<T> or std::is_integral_v<T>) and !std::is_same_v<T, bool>
  JsonValue(T numVal) : m_value(static_cast<number_type>(numVal)) {}
  JsonValue(bool boolVal) : m_value(boolVal) {}
  JsonValue() : m_value(JsonObject{}) {}

  template<class T> requires json::is_json_type<T>
  const T& as() const { return const_cast<JsonValue&>(*this).as<T>(); }
  template<class T> requires json::is_json_type<T>
  T &as();

  template<class T> requires json::is_json_key<T>
  JsonView operator[](T name);

  const variant_type &getVariant() const { return m_value; }

private:
  variant_type m_value;
};

class JsonView {
public:
  using json_key = std::variant<std::string, size_t>;

  JsonView(JsonValue& object, std::string key);

  template<class T> requires std::is_integral_v<T>
  JsonView(JsonValue& object, T key);

  JsonView(const JsonView&) = delete;
  JsonView(JsonView&&) = delete;
  JsonView& operator=(const JsonView&) = delete;
  JsonView& operator=(JsonView&&) = delete;

  JsonView& operator=(JsonValue&& val);
  JsonView& operator=(const JsonValue& val) { return *this = JsonValue{ val }; }

  template<class T> requires json::is_json_type<T>
  operator const T&() const;

  operator const JsonValue&() const;
  const JsonValue& operator*() const { return static_cast<const JsonValue&>(*this); }

  template<class T> requires json::is_json_key<T>
  JsonView operator[](T key);

private:
  JsonValue& m_object;
  json_key m_field;
};

std::ostream& operator<<(std::ostream& os, const JsonValue& val);
std::ostream& operator<<(std::ostream& os, json::pretty_print val);
std::ostream& operator<<(std::ostream& os, json::force_pretty_print val);
std::istream& operator>>(std::istream& is, JsonValue& val);

#include "json.inl"