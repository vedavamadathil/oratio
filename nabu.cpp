#include <iostream>
#include <sstream>
#include <set>

#include "nabu.hpp"

using namespace std;
using namespace nabu;

// Sources
StringFeeder sf = R"(
integer := digit {
	std::string first = get <std::string> (_val);
}
)";

// Literals constants and rules
static const char walrus[] = ":=";

using lbrace = space_lit <'{'>;
using rbrace = space_lit <'}'>;

// Grammar structures
struct defined {};
struct custom_enclosure {};

struct basic_assignment {};
struct custom_assignment {};

struct nabu_start {};

// New rules
template <> struct rule <defined> : public rule <str <walrus>> {};
template <> struct rule <custom_enclosure> : public seqrule <lbrace, delim_str <'}'>> {
	static ret *value(Feeder *fd) {
		ret *rptr = _value(fd);
		if (rptr)
			return getrv(rptr)[1];
		return nullptr;
	}
};

template <> struct rule <basic_assignment> : public seqrule <identifier, defined, identifier> {};
template <> struct rule <custom_assignment> : public seqrule <identifier, defined, identifier, custom_enclosure> {};

// String formatting
std::string format(const std::string &source, const std::vector <std::string> &args)
{
	ostringstream oss;
	istringstream iss(source);

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

///////////////////////////////
// Source generation methods //
///////////////////////////////

// Static source templates
constexpr const char *basic_assignment_source = R"(
template <> struct rule <%s> : public rule <%s> {};
)";

constexpr const char *custom_assignment_source = R"(
template <>
struct rule <@1> : public rule <@2> {
	static ret *value(Feeder *fd) {
		// Predefined values
		ret *_val = rule <@1> ::value(fd);

		// User source
		@3
	}
}
)";

// ruleA := ruleB as simple inheritance
inline std::string inheritance(const std::string &ruleA,
		const std::string ruleB)
{
	return format(
		basic_assignment_source,
		ruleA.c_str(),
		ruleB.c_str()
	);
}

inline std::string inheritance(const std::string &ruleA,
		const std::string &ruleB,
		const std::string function)
{
	return format(
		custom_assignment_source,
		ruleA.c_str(),
		ruleB.c_str(),
		function.c_str()
	);
}

// Parsers
class NabuParser : public Parser <nabu_start> {
	// Members
	std::set <std::string> _rules;
public:
	// Constructor
	NabuParser(Feeder *fd) : Parser <nabu_start> (fd) {}

	// Overloading grammars
	template <class T>
	ret *grammar() {
		return Parser <nabu_start> ::grammar <T> ();
	}
};

// Overloading grammars
template <>
ret *NabuParser::grammar <basic_assignment> ()
{
	ret *rptr = rule <basic_assignment> ::value(feeder);
	if (!rptr)
		return nullptr;
	
	// Continue
	ReturnVector rvec = getrv(rptr);
	std::string first = get <std::string> (rvec[0]);
	std::string second = get <std::string> (rvec[2]);

	std::cout << inheritance(first, second) << "\n";

	return NABU_SUCCESS;
}

template <>
ret *NabuParser::grammar <custom_assignment> ()
{
	ret *rptr = rule <custom_assignment> ::value(feeder);
	if (!rptr)
		return nullptr;

	// Continue
	ReturnVector rvec = getrv(rptr);
	std::string source = inheritance(
		get <std::string> (rvec[0]),
		get <std::string> (rvec[2]),
		get <std::string> (rvec[3])
	);

	std::cout << source << std::endl;

	return NABU_SUCCESS;
}

// Main function
int main()
{
	NabuParser nbpr(&sf);
	nbpr.grammar <custom_assignment> ();
}