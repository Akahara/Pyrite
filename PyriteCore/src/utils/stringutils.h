#pragma once

#include <locale>

namespace pyr {

std::string widestring2string(const std::wstring &string);

std::wstring string2widestring(const std::string& string);
}
