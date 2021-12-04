#include <iostream>

#include "nabu.hpp"
#include "common.hpp"

using namespace std;
using namespace nabu;

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

struct basic_expression {};
struct custom_expression {};

template <> struct nabu::rule <basic_expression> : public seqrule <term_expr> {};
template <> struct nabu::rule <custom_expression> : public seqrule <term_expr, custom_enclosure> {};

struct expression {};
struct option_expression {};

template <> struct nabu::rule <option_expression> : public seqrule <option, expression> {};

template <> struct nabu::rule <expression> : public multirule <
		custom_expression,
		basic_expression
	> {};

struct statement {};
struct option_list {};

template <> struct nabu::rule <option_list> : public kstar <skipper <option_expression>> {};
template <> struct nabu::rule <statement> : public seqrule <identifier, defined, expression, option_list> {};

struct statement_list {};

template <> struct nabu::rule <statement_list> : public kstar <skipper <statement>> {};

// Sources
StringFeeder sf = R"(
number := k8 {body123}
	| digit*
	| identifier digit+ {body456}

paren := '(' expression ')'
)";

// Main function
int main()
{
	ret *rptr = rule <statement_list> ::value(&sf);
	ReturnVector rvec = getrv(rptr);
	std::cout << rvec.json() << std::endl;
}