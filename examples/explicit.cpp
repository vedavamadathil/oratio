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

using myident = rules::skipper <rules::identifier>;
using mynumber = rules::skipper <int>;

inline int to_int(const std::string &s) {
	return std::atoi(s.c_str());
}

inline std::string to_string(const std::string &s) {
	return s;
}

auto_mk_overloaded_token(myident, "[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);
auto_mk_overloaded_token(mynumber, "[0-9]+", int, to_int);

struct macro {};

auto_mk_overloaded_token(macro, "@[a-zA-Z_][a-zA-Z0-9_]*", std::string, to_string);

struct warlus {};

auto_mk_token(warlus, ":=");

struct option {};

auto_mk_token(option, "\\|")

lexlist_next(myident, mynumber);
lexlist_next(mynumber, macro);
lexlist_next(macro, warlus);
lexlist_next(warlus, option);

// auto_mk_token(int, "[0-9]+");

int main()
{
	std::cout << "Source: " << source << std::endl;
	parser::Queue q = parser::lexq <myident> (source);
	while (!q.empty()) {
		parser::lexicon lptr = q.front();
		q.pop_front();

		std::cout << "Lexicon: " << lptr->str() << std::endl;
	}

	/* parser::lexicon lptr;
	lptr = parser::rd::grammar <myident, myident, mynumber> ::value(q);

	std::cout << "lptr = " << lptr << " -> "
		<< (lptr ? lptr->str() : "Null") << std::endl; */
}
