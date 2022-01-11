#include "nabu.hpp"

using namespace nabu;

std::string source = R"(identifier identifier 456)";

using myident = rules::skipper <rules::identifier>;
using mynumber = rules::skipper <int>;

inline int to_int(const std::string &s) {
	return std::atoi(s.c_str());
}

inline std::string to_string(const std::string &s) {
	return s;
}

mk_overloaded_token(myident, 1, "[a-z]+", std::string, to_string);
mk_overloaded_token(mynumber, 2, "[0-9]+", int, to_int);

lexlist_next(myident, mynumber);

int main()
{
	parser::Queue q = parser::lexq <myident> (source);
	/* while (!q.empty()) {
		parser::lexicon lptr = q.front();
		q.pop_front();

		std::cout << "Lexicon: " << lptr->str() << std::endl;
	} */

	parser::lexicon lptr;
	lptr = parser::rd::grammar <myident, myident, mynumber> ::value(q);

	std::cout << "lptr = " << lptr << " -> "
		<< (lptr ? lptr->str() : "Null") << std::endl;
}
