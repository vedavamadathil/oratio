#ifndef RULES_H_
#define RULES_H_

// Standard headers
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>

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
	std::set <std::string>		resolved;	// Tags that have been resovled

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

	// Index of parenthesized expressions
	size_t				parenthesized = 0;

	// Index of str literals
	size_t				lindex = 0;

	// String literals
	// TODO: should recycle these using set
	std::vector <std::string>	literals;

	// Predefined symbols
	std::unordered_map <std::string, std::string> predefined {
		// C++ built-in
		{"int", "int"},
		{"double", "double"},
		{"char", "char"},

		// Nabu built-in
		{"identifier", "nabu::identifier"},
		{"word", "nabu::word"},
		{"space", "nabu::space"}
	};
	
	// Add tag and unresolved
	void push_symbol(const std::string &symbol, size_t line) {
		tags.insert(symbol);
		if (resolved.count(symbol) == 0)
			unresolved[symbol] = line;
	}

	// Attempt to resolve a symbol
	void resolve_symbol(const std::string &symbol) {
		// In case anything goes wrong
		if (tags.count(symbol))
			unresolved.erase(symbol);
		resolved.insert(symbol);
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

// Singlet term
struct term_singlet {};

template <> struct nabu::rule <term_singlet> : public multirule <
		skipper_no_nl <identifier>,
		skipper_no_nl <cchar>,
		skipper_no_nl <cstr>
	> {

	// Index constants
	enum {
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

		std::string value;
		std::string rule_tag;	// Tag
		std::string rule_expr;	// Actual rule

		// Is the term a string literal?
		size_t index = 0;

		// TODO: enum for cases
		switch (mr.first) {
		case IDENTIFIER:
			value = get <std::string> (mr.second);
			if (state.predefined.count(value)) {
				rule_expr = state.predefined[value];
			} else {
				rule_tag = get <std::string> (mr.second);
				rule_expr = state.lang_name + "::" + rule_tag;
			}

			break;
		case C_CHAR:
			rule_expr = std::string("lit <\'") + get <char> (mr.second) + "\'>";
			break;
		case C_STR:
			rule_expr = get <std::string> (mr.second);
			index = 1;
			break;
		default:
			// Throw internal error
			break;
		}

		// Add rule to set
		if (!rule_tag.empty())
			state.push_symbol(rule_tag, line);
		
		// Return the constructed sub-expression
		return ret(new Tret <mt_ret> ({
			index,
			ret(new Tret <std::string> (rule_expr))
		}));
	}
};

// General terms
struct full_term {};
struct simple_term {};
struct paren_term {};
struct term_prime {};
struct term_expr {};

// Term as a parenthesized expression
template <> struct nabu::rule <paren_term> : public seqrule <
		lit <'('>,
		term_expr,
		lit <')'>
	> {

	// Create a new sub-expression tag
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;
		
		// Return term expression
		return getrv(rptr)[1];
	}
};

// Simple term is either a simple term or a parenthesized experession
//	no need to specialize the value function because both return
//	a string
template <> struct nabu::rule <simple_term> : public multirule <
		term_singlet,
		paren_term
	> {

	// Return indexed value
	static ret value(Feeder *fd) {
		mt_ret mr = _value(fd);
		if (mr.first < 0)
			return nullptr;
		return ret(new Tret <mt_ret> (mr));
	}
};

// Full term is term will option * or +
template <> struct nabu::rule <full_term> : public seqrule <
		simple_term,
		term_prime
	> {

	// Index constants
	enum {
		KSTAR,
		KPLUS,
		EPSILON
	};

	static ret value(Feeder *fd) {
		// Run the rule
		ret rptr = _value(fd, false);
		if (!rptr)
			return nullptr;

		// Convert to ReturnVector
		return rptr;
	}
};

// Left recursion
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

template <> struct nabu::rule <term_expr> : public kplus <full_term> {};

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

template <> class nabu::rule <statement> : public seqrule <
		identifier,
		defined,
		expression,
		option_list
	> {
	
	// Index constants
	enum {
		CUSTOM = 0,
		BASIC = 1,

		SIMPLE = 0,
		PAREN = 1,

		KSTAR = 0,
		KPLUS = 1,
		EPSILON = 2,

		LITERAL = 1
	};

	static std::string mk_term(ret rptr) {
		// As a list
		ReturnVector rvec = getrv(rptr);
		// std::cout << "\trptr = " << rptr->str() << std::endl;

		// Get indexable value
		mt_ret mr = get <mt_ret> (rvec[0]);

		std::string term;
		if (mr.first == SIMPLE) {		// Simple term

			// Check for possible literal
			mt_ret plit = get <mt_ret> (mr.second);

			// Either value, set term to value
			term = get <std::string> (plit.second);
			if (plit.first == LITERAL) {
				state.literals.push_back(term);
				term = "str <str_lit_" + std::to_string(state.lindex++) + ">";
			}
		} else if (mr.first == PAREN) {		// Parenthesized term
			// TODO: make a named rule for this
			term = mk_term_expr(mr.second, true);
		} // Throw internal error

		// std::cout << "\tso far, term = " << term << std::endl;

		switch (get <int> (rvec[1])) {
		case KSTAR:
			term = "kstar <" + term + ">";
			break;
		case KPLUS:
			term = "kplus <" + term + ">";
			break;
		case EPSILON:
			break;
		}

		return term;
	}

	static std::string mk_term_expr(ret rptr, bool nested) {
		ReturnVector rvec = getrv(rptr);

		// If single element, return mk_term value
		if (rvec.size() == 1) {
			// Term
			std::string term = mk_term(rvec[0]);

			// If nested in parenthesis, return mk_term value
			if (nested)
				return term;
			
			// If not a seqrule, enclose in rule
			if (term.length() > 7 && term.substr(0, 7) == "seqrule")
				return term;
			
			return "rule <" + term + ">";
		}

		// Otherwise, create a seqrule
		std::string combined = "seqrule <";
		for (size_t i = 0; i < rvec.size(); i++) {
			combined += mk_term(rvec[i]);
			if (i < rvec.size() - 1)
				combined += ", ";
		}
		
		return combined + ">";
	}

	static std::string mk_expression(const std::string &rule_tag, ret rptr) {
		mt_ret mr = get <mt_ret> (rptr);

		// Actual expression
		std::string expr;
		
		if (mr.first == CUSTOM) {
			ReturnVector rvec = getrv(mr.second);
			expr = mk_term_expr(rvec[0], false);
		} else if (mr.first == BASIC) {
			expr = mk_term_expr(mr.second, false);
		}

		// Source code
		std::string code;
		if (mr.first == CUSTOM) {
			ReturnVector rvec = getrv(mr.second);
			code = format(
				sources::custom_expression,
				state.lang_name + "::" + rule_tag,
				expr,
				rvec[1]->str()
			);
		} else {
			code = format(
				sources::basic_expression,
				state.lang_name + "::" + rule_tag,
				expr
			);
		}

		// std::cout << "code = " << code << std::endl;

		// Add the code
		add_source(code);

		// Resolve a symbol only if it is defined
		state.push_symbol(rule_tag, -1);
		state.resolve_symbol(rule_tag);
		return code;
	}
public:
	static ret value(Feeder *fd) {
		ret rptr = _value(fd);
		if (!rptr)
			return nullptr;

		ReturnVector rvec = getrv(rptr);
		// std::cout << rvec.json() << std::endl;

		// Get the rule tag
		std::string rule_tag = get <std::string> (rvec[0]);

		// Get the expression
		mt_ret mr = get <mt_ret> (rvec[2]);

		// If there are options, separate each rule
		ReturnVector optns = getrv(rvec[3]);
		// std::cout << "optns: " << optns.json() << std::endl;

		// Add rule to set
		set_first(rule_tag);

		// Different things for options
		if (optns) {
			size_t i = 0;
			mk_expression(rule_tag + "_optn_" + std::to_string(i), rvec[2]);
			for (ret optn : optns)
				mk_expression(rule_tag + "_optn_" + std::to_string(++i), optn);
			
			// Create multirule code
			std::string code = "template <> struct nabu::rule <"
				+ state.lang_name + "::" + rule_tag
				+ "> : public multirule <\n\t\t";

			for (size_t j = 0; j <= i; j++) {
				code += state.lang_name + "::" + rule_tag
					+ "_optn_" + std::to_string(j);

				if (j < i)
					code += ",\n\t\t";
			}

			// Add end
			code += "\n\t> {};\n";

			// Add the source
			add_source(code);
		} else {
			mk_expression(rule_tag, rvec[2]);
		}

		// Resolve a symbol only if it is defined
		state.push_symbol(rule_tag, -1);
		state.resolve_symbol(rule_tag);
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
set_name(term_star, term_star);
set_name(term_plus, term_plus);

set_name(simple_term, simple_term);
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
