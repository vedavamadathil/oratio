#ifndef RULES_H_
#define RULES_H_

// Standard headers
#include <iostream>
#include <map>
#include <set>

// Debugging nabu rules
// #define NABU_DEBUG_RULES

// Nabu library headers
#include "nabu.hpp"

// Local headers
#include "common.hpp"

// Color constants
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define WARNING "\033[93;1m"
#define ERROR "\033[91;1m"

// TODO: automate conversion from left recursion
// to right recursion, and give a NOTE

// State singleton
struct State {
	// Set of rule tags
	std::set <std::string>		tags;

	// Unresolved symbols (maps to line number)
	std::map <std::string, size_t>	unresolved;

	// Rule specialization code
	std::vector <std::string>	code;

	// Main rule and language name
	//	by default, the main rule
	//	is the first rule in the set
	std::string			main_rule;
	std::string			lang_name;
	std::string			first_tag;

	// Flags
	bool				no_main_rule = false;
	bool				no_json = false;
	bool				print_json = false;

	// Number of warnings and errors
	size_t				warnings = 0;
	size_t				errors = 0;
	
	// Add tag and unresolved
	void push_symbol(const std::string &symbol, size_t line) {
		tags.insert(symbol);
		if (tags.count(symbol) == 0)
			unresolved[symbol] = line;
	}

	// Attempt to resolve a symbol
	void resolve_symbol(const std::string &symbol) {
		if (tags.count(symbol))
			unresolved.erase(symbol);
	}
};

extern State state;

// TODO: make these methods of the State struct

// Adding to this set
inline void add_tag(const std::string &tag)
{
	if (!tag.empty())
		state.tags.insert(tag);
}

// Set the first rule
void set_first(const std::string &tag)
{
	if (state.first_tag.empty())
		state.first_tag = tag;
}

// Adding sources
inline void add_source(const std::string &source)
{
	state.code.push_back(source);
}

// Print errors
template <class ... Args>
inline void error(nabu::Feeder *fd, const std::string &str, Args ... args)
{
	std::string fmt = "%s%s:%d:%d: %serror:%s " + str;
	fprintf(stderr, fmt.c_str(), BOLD, fd->source().c_str(),
		fd->line(), fd->col() - 1, ERROR, RESET, args...);
	exit(-1);
}

template <class ... Args>
inline void warn_no_col(const std::string &source, size_t line, const std::string &str, Args ... args)
{
	std::string fmt = "%s%s:%d: %swarning:%s " + str;
	fprintf(stderr, fmt.c_str(), BOLD, source.c_str(),
		line, WARNING, RESET, args...);
}

// Literals constants and rules
extern const char walrus_str[];

// Predefined rules
extern const char int_str[];
extern const char double_str[];
extern const char identifier_str[];
extern const char word_str[];

// Using declarations
using lbrace = nabu::space_lit <'{'>;
using option = nabu::space_lit <'|'>;
using equals = nabu::space_lit <'='>;

// Trap structures
struct equal_trap {};
struct equal_trap_statement {};

// Error handling rules
template <> struct nabu::rule <equal_trap> : public rule <equals> {
	static ret value(Feeder *fd) {
		if (rule <equals> ::value(fd)) {
			error(fd, "%s", "use \':=\' instead of \'=\'\n");
		}

		return nullptr;
	}
};

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

// Predefined rules
#define mk_predefined(name)				\
	struct name##_rule {};				\
							\
	template <> struct nabu::rule <name##_rule>	\
		: public rule <str <name##_str>> {};

mk_predefined(int);
mk_predefined(double);
mk_predefined(identifier);
mk_predefined(word);

// Overarching rule for predefined
struct predefined {};

template <> struct nabu::rule <predefined> : public multirule <
		int_rule,
		double_rule,
		identifier_rule,
		word_rule
	> {

	// Index constants
	enum {
		INT_RULE,
		DOUBLE_RULE,
		IDENTIFIER_RULE,
		WORD_RULE
	};
	
	// Return the corresponding nabu rule
	static ret value(Feeder *fd) {
		// Run the multirule
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;
		
		// Macros for cases and returns?
		switch (mr.first) {
		case INT_RULE:
			return ret(new Tret <std::string> ("int"));
		case DOUBLE_RULE:
			return ret(new Tret <std::string> ("double"));
		case IDENTIFIER_RULE:
			return ret(new Tret <std::string> ("nabu::identifier"));
		case WORD_RULE:
			return ret(new Tret <std::string> ("nabu::word"));
		default:
			break;
		}

		// TODO: should have a fatal error
		return nullptr;
	}
};

// Singlet term
struct term_singlet {};

template <> struct nabu::rule <term_singlet> : public multirule <
		skipper_no_nl <predefined>,
		skipper_no_nl <identifier>,
		skipper_no_nl <cchar>,
		skipper_no_nl <cstr>
	> {

	// Index constants
	enum {
		PREDEFINED,
		IDENTIFIER,
		C_CHAR,
		C_STR
	};

	static ret value(Feeder *fd) {
		// Its safe to get the line here because
		//	the skippers will not consume any
		//	newlines
		size_t line = fd->line();

		// Run the multirule
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;

		std::string rule_tag;	// Tag
		std::string rule_expr;	// Actual rule

		// TODO: enum for cases
		switch (mr.first) {
		case PREDEFINED:
			rule_expr = get <std::string> (mr.second);
			break;
		case IDENTIFIER:
			rule_tag = get <std::string> (mr.second);
			rule_expr = state.lang_name + "::" + rule_tag;
			break;
		case C_CHAR:
			rule_expr = std::string("lit <\'") + get <char> (mr.second) + "\'>";
			break;
		case C_STR:
			// TODO: deal with static const char[] caveat
			rule_expr = "str <\"" + get <std::string> (mr.second) + "\">";
			break;
		default:
			// Throw internal error
			break;
		}

		// Add rule to set
		if (!rule_tag.empty())
			state.push_symbol(rule_tag, line);
		
		// Return the constructed sub-expression
		return ret(new Tret <std::string> (rule_expr));
	}
};

// General term
struct term {};
struct term_prime {};

// TODO: term star and term plus using right recursion (convert from left)
template <> struct nabu::rule <term> : public seqrule <
		term_singlet,
		term_prime
	> {
	
	// Index constants
	enum {
		KSTAR,
		KPLUS,
		EPSILON
	};

	enum {
		PREDEFINED,
		IDENTIFIER,
		C_CHAR,
		C_STR
	};

	static ret value(Feeder *fd) {
		// Run the rule
		ret rptr = _value(fd, false);
		if (!rptr)
			return nullptr;

		// Convert to ReturnVector
		ReturnVector rvec = getrv(rptr);

		// Check kstar or kplus
		std::string rule_expr = get <std::string> (rvec[0]);

		switch (get <int> (rvec[1])) {
		case KSTAR:
			rule_expr = "kstar <" + rule_expr + ">";
			break;
		case KPLUS:
			rule_expr = "kplus <" + rule_expr + ">";
			break;
		default:
			break;
		}
		
		// Return the constructed sub-expression
		return ret(new Tret <std::string> (rule_expr));
	}
};

template <> struct nabu::rule <term_star> : public seqrule <skipper_no_nl <lit <'*'>>, term_prime> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd, false);
		if (rptr) {
			ReturnVector rvec = getrv(rptr);
			return rvec[0];
		}

		return nullptr;
	}
};

// TODO: trap extra + or * for error (nested + or * makes no sense)
template <> struct nabu::rule <term_plus> : public seqrule <skipper_no_nl <lit <'+'>>, term_prime> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd, false);
		if (rptr) {
			ReturnVector rvec = getrv(rptr);
			return rvec[0];
		}

		return nullptr;
	}
};

// Rule to prevent left recursion
template <> struct nabu::rule <term_prime> : public multirule <
		term_star,
		term_plus,
		nabu::epsilon
	> {

	// Return only the index
	static ret value(Feeder *fd) {
		// Run the multirule
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;
		return ret(new Tret <int> (mr.first));
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

template <> struct nabu::rule <option_list> : public kstar <skipper <option_expression>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;
		return rptr;
	}
};

template <> class nabu::rule <statement> : public seqrule <identifier, defined, expression, option_list> {
	static std::string mk_rule(std::string rule_tag, mt_ret mr) {
		std::string rule_expr;
		if (mr.first == 0) {
			// Custom expression
			ReturnVector crv = getrv(mr.second);
			rule_expr = format(
				sources::custom_expression,
				state.lang_name + "::" + rule_tag,
				get <std::string> (crv[0]),
				get <std::string> (crv[1])
			);
		} else if (mr.first == 1) {
			// Basic expression
			rule_expr = format(
				sources::basic_expression,
				state.lang_name + "::" + rule_tag,
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
		std::string rule_tag = get <std::string> (rvec[0]);

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
			rule_expr = "template <> struct nabu::rule <"
				+ state.lang_name + "::" + rule_tag + "> : public multirule <";
			for (size_t i = 0; i < sub_rules.size(); i++) {
				rule_expr += state.lang_name + "::" + rule_tag
					+ "_optn_" + std::to_string(i);
				if (i < sub_rules.size() - 1)
					rule_expr += ", ";
			}

			rule_expr += "> {};";
		} else {
			rule_expr = mk_rule(rule_tag, mr);
		}

		// Add rule to set
		add_source(rule_expr);
		set_first(rule_tag);

		// Resolve a symbol only if it is defined
		std::cout << "Defining statement: " << rule_tag << std::endl;
		state.push_symbol(rule_tag, -1);
		state.resolve_symbol(rule_tag);
		for (const auto &str : state.unresolved)
			std::cout << "\tunresolved: " << str.first << std::endl;

		return rptr;
	}
};

template <> struct nabu::rule <equal_trap_statement> : public seqrule <
		identifier,
		equal_trap,
		expression,
		option_list
	> {};

// Preprocessor directives
extern const char entry_str[];
extern const char noentry_str[];
extern const char source_str[];
extern const char rules_str[];
extern const char nojson_str[];
extern const char project_str[];

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
		state.main_rule = get <std::string> (rvec[1]);
		return ret(new Tret <std::string> ("@entry " + state.main_rule));
	}
};

template <> struct nabu::rule <pre_noentry> : public seqrule <pre_dir <noentry_str>> {
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		state.no_main_rule = true;
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

		state.no_json = true;
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
		state.lang_name = get <std::string> (rvec[1]);
		return ret(new Tret <std::string> ("@project " + state.first_tag));
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

template <> struct nabu::rule <unit> : public multirule <
		statement,
		preprocessor,
		// Error handling
		equal_trap_statement
	> {};

// Statement list (ultimate grammar)
struct statement_list {};

template <> struct nabu::rule <statement_list> : public kstar <skipper <unit>> {};

// Set names
set_name(int_rule, int_rule);
set_name(double_rule, double_rule);
set_name(identifier_rule, identifier_rule);
set_name(word_rule, word_rule);
set_name(predefined, predefined);

set_name(term_star, term_star);
set_name(term_plus, term_plus);

set_name(term, term);
set_name(term_expr, term_expr);
set_name(custom_enclosure, custom_enclosure);
set_name(custom_expression, custom_expression);
set_name(basic_expression, basic_expression);

set_name(defined, defined);
set_name(expression, expression);
set_name(option_list, option_list);

set_name(pre_entry, entry);
set_name(pre_noentry, noentry);
set_name(pre_nojson, nojson);
set_name(pre_source, source);
set_name(pre_rules, rules);
set_name(pre_project, project);

set_name(preprocessor, preprocessor);
set_name(statement, statement);
set_name(equal_trap, equal_trap);

set_name(unit, unit);

#endif
