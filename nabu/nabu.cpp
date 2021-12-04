#include <cassert>
#include <iostream>
#include <set>
#include <sstream>

#include "nabu.hpp"

using namespace std;
using namespace nabu;

// Sources
StringFeeder sf = R"(
ruleA := Identifier12 _equals* '+' _9integer9+ "my literal" 
ruleB := ruleA semicolon ruleA

ruleA := Identifier12 _equals* '+' _9integer9+ "my literal" {
	body...
}

ruleB := ruleA semicolon ruleA
)";

// Literals constants and rules
static const char walrus[] = ":=";

using lbrace = space_lit <'{'>;
using option = space_lit <'|'>;

// Grammar structures

// TODO: remove
struct basic_assignment {};
struct custom_assignment {};

struct option_assignment {};

struct nabu_start {};

// New rules
struct defined {};

template <> struct nabu::rule <defined> : public rule <str <walrus>> {};

// TODO: macro for these cycles?
struct custom_enclosure {};

template <> struct nabu::rule <custom_enclosure> : public seqrule <lbrace, delim_str <'}'>> {
	static ret *value(Feeder *fd) {
		ret *rptr = _value(fd);
		if (rptr)
			return getrv(rptr)[1];
		return nullptr;
	}
};

struct term_star {};
struct term_plus {};

template <> struct nabu::rule <term_star> : public seqrule <skipper_no_nl <identifier>, lit <'*'>> {};
template <> struct nabu::rule <term_plus> : public seqrule <skipper_no_nl <identifier>, lit <'+'>> {};

struct term {};

template <> struct nabu::rule <term> : public multirule <
		term_star,
		term_plus,
		skipper_no_nl <identifier>,
		skipper_no_nl <cchar>,
		skipper_no_nl <cstr>
	> {};


struct term_expr {};

template <> struct nabu::rule <term_expr> : public kplus <term> {};

struct basic_statement {};
struct custom_statement {};

template <> struct nabu::rule <basic_statement> : public seqrule <identifier, defined, term_expr> {};
template <> struct nabu::rule <custom_statement> : public seqrule <identifier, defined, term_expr, custom_enclosure> {};

struct statement {};

template <> struct nabu::rule <statement> : public multirule <
		custom_statement,
		basic_statement
	> {};

struct statement_list {};

template <> struct nabu::rule <statement_list> : public kstar <skipper <statement>> {};

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
template <> struct rule <@1> : public rule <@2> {};
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
	std::cout << "rule: " << rule <statement_list> ::value(&sf)->str() << std::endl;
}