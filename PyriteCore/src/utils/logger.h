#pragma once

#include <format>
#include <iomanip>
#include <sstream>
#include <iostream>

#include "Math.h"
#include "StringUtils.h"

inline std::ostream &operator<<(std::ostream &ss, const vec2& v)
{
	return ss << std::format("(%.2f, %.2f)", v.x, v.y);
}
inline std::ostream &operator<<(std::ostream &ss, const vec3& v)
{
	return ss << std::format("(%.2f, %.2f, %.2f)", v.x, v.y, v.z);
}
inline std::ostream &operator<<(std::ostream &ss, const vec4& v)
{
	return ss << std::format("(%.2f, %.2f, %.2f, %.2f)", v.x, v.y, v.z, v.w);
}
inline std::ostream &operator<<(std::ostream &ss, const std::wstring& s)
{
	return ss << pyr::widestring2string(s);
}

class Logger
{
public:
	enum Verbosity {
		VERBOSE,
		INFO,
		WARN,
		FATAL,
	};

	explicit Logger(std::string_view name, Verbosity verbosity = INFO)
		: m_name(name)
		, m_verbosity(verbosity)
	{}

	template<class ...Args>
	void log(Verbosity verbosity, Args&&... args)
	{
		if (verbosity < m_verbosity)
			return;

		static std::stringstream ss;
		ss.str("");
		(ss << ... << std::forward<Args>(args));
		ss << '\n';
		std::cout << ss.str();
		OutputDebugStringA(ss.str().c_str());
	}

	template<class ...Args>
	void logf(Verbosity verbosity, std::string_view format, Args&&... args)
    {
	    if (verbosity < m_verbosity)
	      return;

		std::string res = std::vformat(
			std::string(format) + "\n",
			std::make_format_args(std::forward<Args>(args)...));

		std::cout << res;
	    OutputDebugStringA(res.c_str());
    }


    static std::string concat(auto&&... args)
	{
        std::stringstream ss;
	    (ss << ... << std::forward<decltype(args)>(args));
	    return ss.str().c_str();
	}

private:
	std::string m_name;
	Verbosity m_verbosity;
};

#define PYR_DEFINELOG(logname, verbosity) Logger PyrLogger_##logname{ #logname, Logger::verbosity }
#define PYR_DECLARELOG(logname) extern Logger PyrLogger_##logname
#define PYR_LOG(logger, verbosity, ...) PyrLogger_##logger.log(Logger::verbosity, __VA_ARGS__)
#define PYR_LOGF(logger, verbosity, format, ...) PyrLogger_##logger.logf(Logger::verbosity, format, __VA_ARGS__)
