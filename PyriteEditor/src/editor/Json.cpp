#include "json.h"

#include <fstream>

template<class ...Ts>
struct overloaded : public Ts... {
  using Ts::operator()...;
};

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

static void prettyPrintJson(std::ostream &os, const JsonValue &val, int depth) {
  auto printLine = [&](int d) {
    if (!(os.flags() & std::ios::boolalpha)) return;
    constexpr int maxDepth = 10;
    constexpr char spaces[maxDepth*2+1] = "                    ";
    os << '\n';
    while (d > 0) {
      os.write(spaces, std::min(d*2, maxDepth));
      d -= maxDepth;
    }
  };

  std::visit(overloaded{
    [&](double x) { os << x; },
    [&](const JsonObject &j) {
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
        prettyPrintJson(os, j.getFields().at(name), depth+1);
        if (++i != j.getFields().size()) {
          os << ",";
          printLine(depth+1);
        }
      }
      printLine(depth);
      os << '}';
    },
    [&](const std::vector<JsonValue> &a) {
      os << '[';
      printLine(depth+1);
      size_t i = 0;
      for(const auto &fieldVal : a) {
        prettyPrintJson(os, fieldVal, depth+1);
        if (++i != a.size()) {
          os << ", ";
          printLine(depth+1);
        }
      }
      printLine(depth);
      os << ']';
    },
    [&](const std::string &s) {
      os << std::quoted(s);
    },
    [&](bool b) {
      os << (b ? "true" : "false");
    }
  }, val.getVariant());
}

std::ostream& operator<<(std::ostream &os, const JsonValue &val)
{
  prettyPrintJson(os, val, 0);
  return os;
}

namespace json
{

JsonValue parse(const std::string &text)
{
  std::stringstream ss{ text };
  return parse(ss);
}

JsonValue parseFile(const std::filesystem::path &filepath)
{
  std::ifstream is{ filepath };
  if (!is) throw json_parser_error("Could not open json file");
  return parse(is);
}

JsonValue parse(std::istream &is)
{
  char c = peekNonSpace(is);

  if(c == '{') {
    is.get();
    c = peekNonSpace(is);
    JsonObject object;
    if (c == '}') {
      is.get();
      return object;
    }
    while(true) {
      c = peekNonSpace(is);
      std::string fieldName;
      if (c == '"') {
        is >> std::quoted(fieldName);
      } else {
        char nameBuf[256];
        is.get(nameBuf, sizeof(nameBuf), ':');
        fieldName = nameBuf;
        if(fieldName.contains(' ')) throw json_parser_error("Got space in property name: " + fieldName);
      }
      if ((c = getNonSpace(is)) != ':') throw json_parser_error("Expected ':' for property value", c);
      object.set(fieldName, parse(is));
      c = getNonSpace(is);
      if (c == ',')
        continue;
      if (c == '}')
        break;
      throw json_parser_error("Expected '}' or ',' in object", c);
    }
    return object;

  } else if(c == '[') {
    is.get();
    json::array_type arrayValues;
    c = peekNonSpace(is);
    if (c == ']') {
      is.get();
      return arrayValues;
    }
    while(true) {
      arrayValues.push_back(parse(is));
      c = getNonSpace(is);
      if (c == ',')
        continue;
      if (c == ']')
        break;
      throw json_parser_error("Expected ']' or ',' in array", c);
    }
    return arrayValues;
  } else if(c >= '0' && c <= '9' || c == '-') {
    json::number_type x;
    is >> x;
    return x;
  } else if (c == '"') {
    std::string text;
    is >> std::quoted(text);
    return text;
  } else if (c == 't') {
    skipText("true", is);
    return true;
  } else if (c == 'f') {
    skipText("false", is);
    return false;
  } else {
    throw json_parser_error("Unknown json type from begining char " + std::to_string(c));
  }
}

}