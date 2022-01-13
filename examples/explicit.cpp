#include <sstream>

#include "nabu.hpp"

using namespace nabu;

std::string read(const std::string &path)
{
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

std::string source = read("examples/example.nabu");

struct identifier {};
struct macro {};
struct warlus {};
struct option {};

inline int to_int(const std::string &s) {
	return std::atoi(s.c_str());
}

inline std::string to_string(const std::string &s) {
	return s;
}

// TODO: mk_token should create a struct, auto_token is using the struct
auto_mk_overloaded_token(identifier, "[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);
auto_mk_overloaded_token(macro, "@[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);

auto_mk_token(warlus, ":=");
auto_mk_token(option, "\\|")

lexlist_next(identifier, macro);
lexlist_next(macro, warlus);
lexlist_next(warlus, option);

#define nabu_tokid(T) token <T> ::id

template <>
struct parser::rd::grammar_action <macro> {
	static constexpr bool available = true;

	// Macro related variables
	static struct {
		std::string project;
		std::string entry;
	} state;

	// Macro handlers
	static void project(parser::Queue &q) {
		parser::lexicon lptr = q.front();
		if (lptr->id == nabu_tokid(identifier)) {
			state.project = get <std::string> (lptr);
			q.pop_front();
		}
	}

	static void entry(parser::Queue &q) {
		if (!parser::expect <identifier, std::string> (q, state.entry)) {
			std::cout << "Expected identifier after @entry!\n";
		}
	}

	// Grammar action
	static void action(parser::lexicon lptr, parser::Queue &q) {
		static std::unordered_map <
				std::string,
				void (*)(parser::Queue &)
			> handlers {
			{"@project", &project},
			{"@entry", &entry}
		};

		std::cout << "MACRO: " << lptr->str() << std::endl;

		std::string macro = get <std::string> (lptr);
		if (handlers.find(macro) == handlers.end()) {
			std::cout << "BAD MACRO!\n";
		}

		return handlers[macro](q);
	}
};

decltype(parser::rd::grammar_action <macro> ::state)
parser::rd::grammar_action <macro> ::state;

int main()
{
	using parser::rd::grammar;
	using parser::rd::option;

	using gmacro = grammar <macro>;

	std::cout << "Source: " << source << std::endl;
	parser::Queue q = parser::lexq <identifier> (source);
	/* while (!q.empty()) {
		parser::lexicon lptr = q.front();
		q.pop_front();

		std::cout << "Lexicon: " << lptr->str() << std::endl;
	} */

	parser::lexicon lptr;
	lptr = grammar <void> ::value(q);
	while ((lptr = gmacro::value(q)));
}
