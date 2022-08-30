#ifndef LEXER_H_
#define LEXER_H_

#define NABU_DEBUG_PARSER

#include <nabu.hpp>

////////////////////////
// Lexicon converters //
////////////////////////

inline std::string to_string(const std::string &s) {
	return s;
}

///////////////////////
// Lexical strutures //
///////////////////////

// Values and types
struct identifier {};
struct str {};

// TODO: Is the string -> string any use?
auto_mk_overloaded_token(identifier, "[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);
auto_mk_overloaded_token(str, "\".*\"", std::string, to_string);

// Structural tokens
struct macro {};
struct fargs {};
struct fbody {};

// TODO: mk_token should create a struct, auto_token is using the struct
auto_mk_overloaded_token(macro, "@[a-zA-Z_][a-zA-Z0-9_-]*", std::string, to_string);
auto_mk_overloaded_token(fargs, "\\(.*\\)", std::string, to_string);
auto_mk_overloaded_token(fbody, "\\{", std::string, to_string);

// Operators
struct action {};
struct walrus {};
struct optional {};

auto_mk_token(action, "[=][>]");
auto_mk_token(walrus, ":=");
auto_mk_token(optional, "\\|")

// Miscenallenous
struct comment {};
struct newline {};

auto_mk_token(newline, "\n");

//////////////////
// Lexing prior //
//////////////////

lexlist_next(identifier, str);
lexlist_next(str, macro);

lexlist_next(macro, fargs);
lexlist_next(fargs, fbody);
lexlist_next(fbody, action);

lexlist_next(action, walrus);
lexlist_next(walrus, optional);
lexlist_next(optional, newline);

// Aliases
using nabu::ArgParser;

using nabu::parser::rd::alias;
using nabu::parser::rd::repeat;
using nabu::parser::rd::option;
using nabu::parser::rd::grammar;
using nabu::parser::rd::grammar_action;

#endif
