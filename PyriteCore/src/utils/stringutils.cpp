#include "stringutils.h"

#include <codecvt>

namespace pyr
{

std::string widestring2string(const std::wstring& string)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.to_bytes(string);
}

std::wstring string2widestring(const std::string& string)
{
	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	return converter.from_bytes(string);
}

}
