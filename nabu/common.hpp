#ifndef COMMON_H_
#define COMMON_H_

#include <string>
#include <vector>
#include <sstream>
#include <cassert>

///////////////////////
// String formatting //
///////////////////////

std::string format(const std::string &source, const std::vector <std::string> &args)
{
	std::ostringstream oss;
	std::istringstream iss(source);

	char c;
	while (!iss.eof() && (c = iss.get()) != EOF) {
		if (c == '@') {
			size_t i = 0;
			while (!iss.eof() && (c = iss.get()) != EOF && isdigit(c))
				i = 10 * i + (c - '0');
			
			// TODO: throw later
			assert(i > 0);
			oss << args[i - 1] << c;
		} else {
			oss << c;
		}
	}

	return oss.str();
}

template <class ... Args>
void _format_gather(std::vector <std::string> &args, std::string str, Args ... strs)
{
	args.push_back(str);
	_format_gather(args, strs...);
}

template <>
void _format_gather(std::vector <std::string> &args, std::string str)
{
	args.push_back(str);
}

template <class ... Args>
std::string format(const std::string &source, Args ... strs)
{
	std::vector <std::string> args;
	_format_gather(args, strs...);
	return format(source, args);
}

////////////////////////////////
// Source generation sources //
////////////////////////////////

namespace sources {

// Static source templates
constexpr const char *basic_expression = R"(template <> struct rule <@1> : public rule <@2> {};
)";

constexpr const char *custom_expression = R"(template <>
struct rule <@1> : public @2 {
	static ret *value(Feeder *fd) {
		// Predefined values
		ret *_val = rule <@1> ::value(fd);

		// User source
		@3
	}
};
)";

}

#endif