#ifndef LEXER_H_
#define LEXER_H_

#include <nabu.hpp>

struct identifier {};
struct macro {};
struct warlus {};
struct option {};
struct newline {};

inline std::string to_string(const std::string &s) {
	return s;
}

// TODO: mk_token should create a struct, auto_token is using the struct
auto_mk_overloaded_token(identifier, "[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);
auto_mk_overloaded_token(macro, "@[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);

auto_mk_token(warlus, ":=");
auto_mk_token(option, "\\|")
auto_mk_token(newline, "\n");

lexlist_next(identifier, macro);
lexlist_next(macro, warlus);
lexlist_next(warlus, option);
lexlist_next(option, newline);

#endif
