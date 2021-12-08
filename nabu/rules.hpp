#ifndef RULES_H_
#define RULES_H_

// Standard headers
#include <iostream>
#include <set>

// Nabu library headers
#include "nabu.hpp"

// Local headers
#include "common.hpp"

// Set of rule tags
extern std::set <std::string> tags;

// Adding to this set
inline void add_tag(const std::string &tag)
{
	if (!tag.empty())
		tags.insert(tag);
}

// Set the first rule
extern std::string first_tag;

void set_first(const std::string &tag)
{
	if (first_tag.empty())
		first_tag = tag;
}

// Sources
extern std::vector <std::string> code;

// Adding sources
inline void add_source(const std::string &source)
{
	code.push_back(source);
}

// Literals constants and rules
extern const char walrus_str[];

using lbrace = nabu::space_lit <'{'>;
using option = nabu::space_lit <'|'>;

//////////////////////
// Rule definitions //
//////////////////////

struct defined {};

template <> struct nabu::rule <defined> : public rule <str <walrus_str>> {};

// TODO: macro for these cycles?
struct custom_enclosure {};

// TODO: maybe check for returns?
template <> struct nabu::rule <custom_enclosure> : public seqrule <lbrace, delim_str <'}'>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (rptr)
			return getrv(rptr)[1];
		return nullptr;
	}
};

struct term_star {};
struct term_plus {};

template <> struct nabu::rule <term_star> : public seqrule <skipper_no_nl <identifier>, lit <'*'>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (rptr) {
			ReturnVector rvec = getrv(rptr);
			return rvec[0];
		}

		return nullptr;
	}
};

template <> struct nabu::rule <term_plus> : public seqrule <skipper_no_nl <identifier>, lit <'+'>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (rptr) {
			ReturnVector rvec = getrv(rptr);
			return rvec[0];
		}

		return nullptr;
	}
};

struct term {};

// Rule tag prefix
extern const char *prefix;

template <> struct nabu::rule <term> : public multirule <
		term_star,
		term_plus,
		skipper_no_nl <identifier>,
		skipper_no_nl <cchar>,
		skipper_no_nl <cstr>
	> {
	
	static ret value(Feeder *fd) {
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

		// Add rule to set
		add_tag(rule_tag);

		return ret(new Tret <std::string> (rule_expr));
	}
};

struct term_expr {};

template <> struct nabu::rule <term_expr> : public kplus <term> {
	static ret value(Feeder *fd) {
		ret rptr = kplus <term> ::value(fd);
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
		return ret(new Tret <std::string> (combined));
	}
};

struct basic_expression {};
struct custom_expression {};

template <> struct nabu::rule <basic_expression> : public rule <term_expr> {};
template <> struct nabu::rule <custom_expression> : public seqrule <term_expr, custom_enclosure> {};

struct expression {};
struct option_expression {};

template <> struct nabu::rule <option_expression> : public seqrule <option, expression> {
	// TODO: fast method to return first element
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		ReturnVector rvec = getrv(rptr);
		return rvec[1];
	}
};

template <> struct nabu::rule <expression> : public multirule <
		custom_expression,
		basic_expression
	> {
	
	// Enable indexing of returns
	// TODO: fast method for this?
	static ret value(Feeder *fd) {
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;
		return ret(new Tret <mt_ret> (mr));
	}
};

struct statement {};
struct option_list {};

template <> struct nabu::rule <option_list> : public kstar <skipper <option_expression>> {};

template <> class nabu::rule <statement> : public seqrule <identifier, defined, expression, option_list> {
	static std::string mk_rule(std::string rule_tag, mt_ret mr) {
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

		return rule_expr;
	}

	static void mk_optns(const std::string &rule_tag, const std::vector <mt_ret> &sub_rules) {
		for (size_t i = 0; i < sub_rules.size(); i++) {
			std::string optn = rule_tag + "_optn_" + std::to_string(i);
			std::string rule_expr = mk_rule(optn, sub_rules[i]);

			// Add rule to set
			add_source(rule_expr);
			add_tag(optn);
		}
	}
public:
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		ReturnVector rvec = getrv(rptr);
		
		// Get the rule tag
		std::string rule_tag = prefix + get <std::string> (rvec[0]);

		// Get the expression
		mt_ret mr = get <mt_ret> (rvec[2]);

		// If there are options, separate each rule
		ReturnVector optns = getrv(rvec[3]);

		// Generate the rule
		std::string rule_expr;
		if (optns) {
			std::vector <mt_ret> sub_rules {mr};
			for (size_t i = 0; i < optns.size(); i++) {
				mt_ret opt = get <mt_ret> (optns[i]);
				sub_rules.push_back(opt);
			}

			// Create the sub-rules
			mk_optns(rule_tag, sub_rules);

			// String the rule
			rule_expr = "template <> struct nabu::rule <" + rule_tag + "> : public multirule <";
			for (size_t i = 0; i < sub_rules.size(); i++) {
				rule_expr += rule_tag + "_optn_" + std::to_string(i);
				if (i < sub_rules.size() - 1)
					rule_expr += ", ";
			}
			
			rule_expr += "> {};";
		} else {
			rule_expr = mk_rule(rule_tag, mr);
		}

		// Add rule to set
		add_source(rule_expr);
		add_tag(rule_tag);
		set_first(rule_tag);
		return rptr;
	}
};

// Preprocessor directives
extern const char entry_str[];
extern const char noentry_str[];
extern const char source_str[];
extern const char rules_str[];
extern const char nojson_str[];
extern const char project_str[];

// Main rule and language name
//	by default, the main rule
//	is the first rule in the set
extern std::string main_rule;
extern std::string lang_name;
extern bool no_main_rule;
extern bool no_json;

using prechar = nabu::space_lit <'@'>;

template <const char *str>
struct pre_dir {};

template <const char *s>
struct nabu::rule <pre_dir <s>> : public seqrule <prechar, str <s>> {};

struct pre_entry {};
struct pre_noentry {};
struct pre_source {};
struct pre_rules {};
struct pre_nojson {};
struct pre_project {};

template <> struct nabu::rule <pre_entry> : public seqrule <
		pre_dir <entry_str>,
		identifier
	> {

	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		ReturnVector rvec = getrv(rptr);
		main_rule = get <std::string> (rvec[1]);
		return ret(new Tret <std::string> ("@entry " + main_rule));
	}
};

template <> struct nabu::rule <pre_noentry> : public seqrule <pre_dir <noentry_str>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		no_main_rule = true;
		return ret(new Tret <std::string> ("@noentry"));
	}
};

template <> struct nabu::rule <pre_source> : public seqrule <
		pre_dir <source_str>,
		delim_str <'@', false>
	> {

	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		// Add the source
		ReturnVector rvec = getrv(rptr);
		std::string source = get <std::string> (rvec[1]);
		add_source(source);

		return ret(new Tret <std::string> ("@source"));
	}
};

template <> struct nabu::rule <pre_rules> : public seqrule <pre_dir <rules_str>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		return ret(new Tret <std::string> ("@rules"));
	}
};

template <> struct nabu::rule <pre_nojson> : public seqrule <pre_dir <nojson_str>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		no_json = true;
		return ret(new Tret <std::string> ("@nojson"));
	}
};

template <> struct nabu::rule <pre_project> : public seqrule <
		pre_dir <project_str>,
		identifier
	> {

	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		ReturnVector rvec = getrv(rptr);
		lang_name = get <std::string> (rvec[1]);
		return ret(new Tret <std::string> ("@project " + lang_name));
	}	
};

struct preprocessor {};
template <>
struct nabu::rule <preprocessor> : public multirule <
		pre_entry,
		pre_noentry,
		pre_source,
		pre_rules,
		pre_nojson,
		pre_project
		// TODO: add another generic preprocessor directive rule for error handling
	> {};

// Unit penultimate rule
struct unit {};

template <> struct nabu::rule <unit> : public multirule <statement, preprocessor> {};

// Statement list (ultimate grammar)
struct statement_list {};

template <> struct nabu::rule <statement_list> : public kstar <skipper <unit>> {};

#endif