#include <iostream>

#include "nabu.hpp"
#include "common.hpp"

using namespace std;
using namespace nabu;

// Literals constants and rules
static const char walrus[] = ":=";

using lbrace = space_lit <'{'>;
using option = space_lit <'|'>;

//////////////////////
// Rule definitions //
//////////////////////

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

template <> struct nabu::rule <term_star> : public seqrule <skipper_no_nl <identifier>, lit <'*'>> {
	static ret *value(Feeder *fd) {
		ret *rptr = _value(fd);
		if (rptr) {
			ReturnVector rvec = getrv(rptr);
			return rvec[0];
		}

		return nullptr;
	}
};

template <> struct nabu::rule <term_plus> : public seqrule <skipper_no_nl <identifier>, lit <'+'>> {
	static ret *value(Feeder *fd) {
		ret *rptr = _value(fd);
		if (rptr) {
			ReturnVector rvec = getrv(rptr);
			return rvec[0];
		}

		return nullptr;
	}
};

struct term {};

// Rule tag prefix
const char *prefix = "nbg_";

template <> struct nabu::rule <term> : public multirule <
		term_star,
		term_plus,
		skipper_no_nl <identifier>,
		skipper_no_nl <cchar>,
		skipper_no_nl <cstr>
	> {
	
	static ret *value(Feeder *fd) {
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;

		std::string rule_tag;	// Tag
		std::string rule_expr;	// Actual rule

		switch (mr.first) {
		case 0:
			rule_tag = prefix + get <std::string> (mr.second);
			rule_expr = "kstar <" + rule_tag + ">";
			break;
		case 1:
			rule_tag = prefix + get <std::string> (mr.second);
			rule_expr = "kplus <" + rule_tag + ">";
			break;
		case 2:
			rule_tag = prefix + get <std::string> (mr.second);
			rule_expr = rule_tag;
			break;
		case 3:
			rule_expr = std::string("lit <\'") + get <char> (mr.second) + "\'>";
			break;
		case 4:
			// TODO: deal with static const char[] caveat
			rule_expr = "str <\"" + get <std::string> (mr.second) + "\">";
			break;
		default:
			break;
		}

		/* if (!rule_tag.empty())
			std::cout << "struct " << rule_tag << " {};" << std::endl; */

		return new Tret <std::string> (rule_expr);
	}
};


struct term_expr {};

template <> struct nabu::rule <term_expr> : public kplus <term> {
	static ret *value(Feeder *fd) {
		ret *rptr = kplus <term> ::value(fd);
		if (!rptr)
			return nullptr;
		
		std::string combined = "seqrule <";

		// Should always be a list of strings
		// TODO: optimize one rule expressions
		ReturnVector rvec = getrv(rptr);
		for (size_t i = 0; i < rvec.size(); i++) {
			combined += get <std::string> (rvec[i]);
			if (i < rvec.size() - 1)
				combined += ", ";
		}

		// Complete the rule and return it
		combined += ">";
		return new Tret <std::string> (combined);
	}
};

struct basic_expression {};
struct custom_expression {};

template <> struct nabu::rule <basic_expression> : public rule <term_expr> {};
template <> struct nabu::rule <custom_expression> : public seqrule <term_expr, custom_enclosure> {};

struct expression {};
struct option_expression {};

template <> struct nabu::rule <option_expression> : public seqrule <option, expression> {};

template <> struct nabu::rule <expression> : public multirule <
		custom_expression,
		basic_expression
	> {
	
	// Enable indexing of returns
	static ret *value(Feeder *fd) {
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;
		return new Tret <mt_ret> (mr);
	}
};

struct statement {};
struct option_list {};

template <> struct nabu::rule <option_list> : public kstar <skipper <option_expression>> {};

template <> struct nabu::rule <statement> : public seqrule <identifier, defined, expression, option_list> {
	static ret *value(Feeder *fd) {
		ret *rptr = _value(fd);
		if (!rptr)
			return nullptr;

		ReturnVector rvec = getrv(rptr);
		mt_ret mr = get <mt_ret> (rvec[2]);

		std::string rule_tag = prefix + get <std::string> (rvec[0]);
		std::string rule_expr;
		if (mr.first == 0) {
			// Custom expression
			ReturnVector crv = getrv(mr.second);
			rule_expr = format(
				sources::custom_expression,
				rule_tag,
				get <std::string> (crv[0]),
				get <std::string> (crv[1])
			);
		} else if (mr.first == 1) {
			// Basic expression
			rule_expr = format(
				sources::basic_expression,
				rule_tag,
				get <std::string> (mr.second)
			);
		}

		std::cout << rule_expr << std::endl;

		// Return NABU_SUCCESS
		return new Tret <std::string> (rule_expr);
	}
};

struct statement_list {};

template <> struct nabu::rule <statement_list> : public kstar <skipper <statement>> {};

// Sources
StringFeeder sf = R"(
paren := '(' expression ')'
number := k8 {body123}
	| digit*
	| identifier digit+ {body456}
)";

// Main function
int main()
{
	ret *rptr = rule <statement_list> ::value(&sf);
	// ReturnVector rvec = getrv(rptr);
	// std::cout << rvec.json() << std::endl;
}