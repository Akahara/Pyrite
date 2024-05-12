#pragma once

#include "json.h"

namespace jsonimpl
{

template<class ...Ts>
struct Overloaded : public Ts... {
  using Ts::operator()...;
};

}

inline void JsonObject::set(const std::string& name, JsonValue&& val)
{
  m_fields.emplace(name, std::move(val));
}

inline JsonView::JsonView(JsonValue& object, std::string key)
  : m_object(object)
  , m_field(std::move(key))
{}

template <class T> requires std::is_integral_v<T>
JsonView::JsonView(JsonValue& object, T key)
  : m_object(object)
  , m_field(static_cast<size_t>(key))
{}

inline JsonView& JsonView::operator=(JsonValue&& val)
{
  std::visit(jsonimpl::Overloaded{
    [&](const std::string& name)
    {
      m_object.as<JsonObject>().set(name, std::move(val));
    },
      [&](size_t index)
    {
      JsonArray& asArray = m_object.as<JsonArray>();
      if (asArray.size() > index)
        asArray[index] = std::move(val);
      else if (asArray.size() == index)
        asArray.push_back(std::move(val));
      else
        throw json_use_error("Tried to insert an item at index " + std::to_string(index) + " in array of size " + std::to_string(asArray.size()));
    }
  }, m_field);
  return *this;
}

static inline JsonValue v;

inline JsonView::operator const JsonValue&() const
{
  return std::visit(jsonimpl::Overloaded{
    [&](const std::string& name) -> const JsonValue&
    {
      return m_object.as<JsonObject>().getField(name);
    },
    [&](size_t index) -> const JsonValue&
    {
      JsonArray& asArray = m_object.as<JsonArray>();
      if (asArray.size() <= index)
        throw json_use_error(std::format("Index out of bounds: {} out of {}", index, asArray.size()));
      return asArray[index];
    }
  }, m_field);
}

template<class T> requires json::is_json_key<T>
JsonView JsonValue::operator[](T name)
{
  return { *this, std::move(name) };
}

template <class T> requires json::is_json_type<T>
JsonView::operator const T&() const
{
  return std::visit(jsonimpl::Overloaded{
    [&](const std::string& name)
    {
      return m_object.as<JsonObject>().get<T>(name);
    },
    [&](size_t index)
    {
      JsonArray& asArray = m_object.as<JsonArray>();
      if (asArray.size() <= index)
        throw json_use_error(std::format("Index out of bounds: {} out of {}", index, asArray.size()));
      return asArray[index].as<T>();
    }
  }, m_field);
}

template <class T> requires json::is_json_key<T>
JsonView JsonView::operator[](T key)
{
  return std::visit(jsonimpl::Overloaded{
    [&](const std::string& name)
    {
      JsonObject& asObject = m_object.as<JsonObject>();
      if (asObject.hasField(name))
        return JsonView{ asObject.getField(name), key };
      if constexpr (std::is_integral_v<T>)
        asObject.set(name, JsonArray{});
      else
        asObject.set(name, JsonObject{});
      return JsonView{ asObject.getField(name), key };
    },
    [&](size_t index)
    {
      JsonArray& asArray = m_object.as<JsonArray>();
      if (asArray.size() <= index)
        throw json_use_error(std::format("Index out of bounds: {} out of {}", index, asArray.size()));
      return JsonView{ asArray[index], key };
    }
  }, m_field);
}

template <class T> requires json::is_json_type<T>
const T& JsonObject::get(const std::string& name) const
{
  if (!m_fields.contains(name))
    throw json_use_error("Missing field in json: " + name);
  try {
    return m_fields.at(name).as<T>();
  } catch (const std::bad_variant_access&) {
    throw json_use_error("Invalid type for json field " + name + ", expected " + typeid(T).name());
  }
}

template <class T> requires json::is_json_type<T>
T& JsonValue::as()
{
  try {
    return std::get<T>(m_value);
  } catch (const std::bad_variant_access&) {
    throw json_use_error(std::format("Invalid type for json value, expected {}", typeid(T).name()));
  }
}

