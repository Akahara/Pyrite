#include "json.h"

#include <fstream>
#include <iostream>
#include <numeric>

namespace jsonimpl {

static char peekNonSpace(std::istream &is)
{
  char c = static_cast<char>(is.peek());
  while (is && (c == ' ' || c == '\t' || c == '\n' || c == '\r'))
    c = (is.get(), static_cast<char>(is.peek()));
  return c;
}

static char getNonSpace(std::istream &is)
{
  peekNonSpace(is);
  return static_cast<char>(is.get());
}

static void skipText(const char *text, std::istream &is)
{
  for(const char *c = text; *c; c++) {
    if (char cc = static_cast<char>(is.get()); cc != *c)
      throw json_parser_error("Expected " + std::string(text), cc);
  }
}

struct json_fmt_flags {
  json_fmt_flags() = default;

  explicit json_fmt_flags(std::ostream& os)
    : os(&os)
    , flags(os.flags())
  {
    os.flags(std::ios_base::dec | std::ios_base::boolalpha);
  }

  ~json_fmt_flags()
  {
    os->flags(flags);
  }

  std::ostream* os{};
  std::ios_base::fmtflags flags{};
  bool prettyPrint{};
  bool forcePrettyPrint{};
};

static size_t jsonDeepLength(const JsonValue& val, size_t maxDeepness)
{
  return std::visit(jsonimpl::Overloaded{
    [&](const JsonObject &j) -> size_t
    {
      return std::accumulate(j.getFields().begin(), j.getFields().end(), 1, [&](size_t acc, auto& v)
      { return acc >= maxDeepness ? acc : jsonDeepLength(v.second, maxDeepness - acc) + acc; });
    },
    [&](const std::vector<JsonValue> &a) -> size_t
    {
      return std::accumulate(a.begin(), a.end(), 1, [&](size_t acc, auto& v)
      { return acc >= maxDeepness ? acc : jsonDeepLength(v, maxDeepness - acc) + acc; });
    },
    [&](const std::string &s) -> size_t
    {
      return 1 + s.length() / 10;
    },
    [&](double x) -> size_t
    {
      return 1;
    },
    [&](bool b) -> size_t
    {
      return 1;
    }
  }, val.getVariant());
}

static std::ostream& prettyPrintJson(std::ostream &os, const JsonValue &val, json_fmt_flags& flags, int depth)
{
  static constexpr size_t minPrettyPrintDeepness = 3;
  bool doPrettyPrint = flags.forcePrettyPrint || (flags.prettyPrint && jsonDeepLength(val, minPrettyPrintDeepness) >= minPrettyPrintDeepness);
  auto printLine = [&](int d) {
    if (!doPrettyPrint) return;
    constexpr int maxDepth = 10;
    constexpr char spaces[maxDepth*2+1] = "                    ";
    os << '\n';
    while (d > 0) {
      os.write(spaces, std::min(d*2, maxDepth));
      d -= maxDepth;
    }
  };

  std::visit(Overloaded{
    [&](const JsonObject &j)
    {
      os << '{';
      printLine(depth+1);
      size_t i = 0;
      std::vector<std::string> fieldNames;
      fieldNames.reserve(j.getFields().size());
      std::ranges::transform(j.getFields(), std::back_inserter(fieldNames), [](auto &kv) { return kv.first; });
      std::ranges::sort(fieldNames);
      for(std::string &name : fieldNames) {
        os << std::quoted(name);
        os << ": ";
        prettyPrintJson(os, j.getFields().at(name), flags, depth+1);
        if (++i != j.getFields().size()) {
          os << ",";
          printLine(depth+1);
        }
      }
      printLine(depth);
      os << '}';
    },
    [&](const std::vector<JsonValue> &a)
    {
      os << '[';
      printLine(depth+1);
      size_t i = 0;
      for(const auto &fieldVal : a) {
        prettyPrintJson(os, fieldVal, flags, depth+1);
        if (++i != a.size()) {
          os << ", ";
          printLine(depth+1);
        }
      }
      printLine(depth);
      os << ']';
    },
    [&](const std::string &s)
    {
      os << std::quoted(s);
    },
    [&](double x)
    {
      os << x;
    },
    [&](bool b) {
      os << b;
    }
  }, val.getVariant());

  return os;
}

static std::istream& loadJson(std::istream &is, JsonValue &v)
{
  char c = peekNonSpace(is);

  if(c == '{') {
    is.get();
    c = peekNonSpace(is);
    JsonObject object;
    if (c == '}') {
      is.get();
      v = std::move(object);
    } else {
      while(true) {
        c = peekNonSpace(is);
        std::string fieldName;
        is >> std::quoted(fieldName);
        if ((c = getNonSpace(is)) != ':') throw json_parser_error("Expected ':' for property value", c);
        if (JsonValue field; is >> field)
          object.set(fieldName, std::move(field));
        c = getNonSpace(is);
        if (c == ',')
          continue;
        if (c == '}')
          break;
        throw json_parser_error("Expected '}' or ',' in object", c);
      }
    }

  } else if(c == '[') {
    is.get();
    JsonArray arrayValues;
    c = peekNonSpace(is);
    if (c == ']') {
      is.get();
      v = std::move(arrayValues);
      return is;
    }
    while(true) {
      JsonValue field;
      is >> field;
      arrayValues.push_back(std::move(field));
      c = getNonSpace(is);
      if (c == ',')
        continue;
      if (c == ']')
        break;
      throw json_parser_error("Expected ']' or ',' in array", c);
    }
    v = std::move(arrayValues);

  } else if(c >= '0' && c <= '9' || c == '-') {
    if (json::number_type x; is >> x)
      v = x;

  } else if (c == '"') {
    if (std::string text; is >> std::quoted(text))
      v = std::move(text);

  } else if (c == 't') {
    skipText("true", is);
    v = true;

  } else if (c == 'f') {
    skipText("false", is);
    v = false;

  } else {
    throw json_parser_error("Unknown json type from begining char " + std::to_string(c));
  }

  if (!is)
    throw json_parser_error("EOF reached while reading json");
  return is;
}

}

namespace json
{

JsonValue parse(const std::string &text)
{
  std::stringstream ss{ text };
  JsonValue v;
  ss >> v;
  return v;
}

JsonValue parse(const std::filesystem::path &filepath)
{
  std::ifstream is{ filepath };
  if (!is) throw json_parser_error("Could not open json file");
  JsonValue v;
  is >> v;
  return v;
}

}


std::ostream& operator<<(std::ostream &os, const JsonValue &val)
{
  jsonimpl::json_fmt_flags flags{ os };
  return jsonimpl::prettyPrintJson(os, val, flags, 0);
}

std::ostream& operator<<(std::ostream &os, json::pretty_print val)
{
  jsonimpl::json_fmt_flags flags{ os };
  flags.prettyPrint = true;
  return jsonimpl::prettyPrintJson(os, val.val, flags, 0);
}

std::ostream& operator<<(std::ostream &os, json::force_pretty_print val)
{
  jsonimpl::json_fmt_flags flags{ os };
  flags.forcePrettyPrint = true;
  return jsonimpl::prettyPrintJson(os, val.val, flags, 0);
}

std::istream& operator>>(std::istream &is, JsonValue &v)
{
  return jsonimpl::loadJson(is, v);
}


int main() {
  JsonValue o;
  o["ab"] = 3;
  o["ac"]["d"] = *o["ab"];
  o["aa"][0] = "5";
  for (size_t i = 0; i < 3; i++)
    o["som"][i] = "some very long string";

  std::cout << json::pretty_print(o);
  std::cout << json::force_pretty_print(o);

  std::cin.get();
}