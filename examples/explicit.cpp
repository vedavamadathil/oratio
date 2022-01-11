#include "nabu.hpp"

using namespace nabu;

std::string source = R"(identifier identifier 456)";

using myident = rules::skipper <rules::identifier>;
using mynumber = rules::skipper <int>;

int to_int(const std::string &s) {return std::atoi(s.c_str());}

mk_token(myident, 1, "[a-z]+");
mk_overloaded_token(mynumber, 2, "[0-9]+", int, to_int);

lexlist_next(myident, mynumber);

int main()
{
	// ret rptr = rules::kstar <myrule> ::value(&sf);
	// std::cout << getrv(rptr).json() << std::endl;

	/* parser::lexicon lptr;
	lptr = parser::lexer <rules::skipper <rules::identifier>> (&sf);
	std::cout << "Code [1]: " << lptr->str() << std::endl;
	lptr = parser::lexer <rules::skipper <int>> (&sf);
	std::cout << "Code [2]: " << lptr->str() << std::endl; */

	/* std::cout << typeid(parser::LexList <myident> ::next).name() << std::endl;
	std::cout << typeid(parser::LexList <mynumber> ::next).name() << std::endl;
	std::cout << typeid(parser::LexList <void> ::next).name() << std::endl; */

	/* parser::Queue queue = parser::lexq <myident> (&sf);

	parser::Queue cpy = queue;
	
	std::cout << "Queue items:\n";
	while (!cpy.empty()) {
		parser::lexicon lptr = cpy.front();
		cpy.pop_front();
		std::cout << "\tlexicon: " << lptr->str() << std::endl;
	}

	std::cout << "Queue size = " << queue.size() << std::endl;
	parser::lexicon lptr = parser::rd::grammar <myident, mynumber> ::value(queue);
	std::cout << "Code: " << lptr.get() << " -> " << queue.size() << std::endl;
	// std::cout << "\tlptr = " << lptr->str() << std::endl;
	
	lptr = parser::rd::grammar <myident, myident, mynumber> ::value(queue);
	std::cout << "Code: " << lptr.get() << " -> " << queue.size() << std::endl;
	std::cout << "\tlptr = " << lptr->str() << std::endl; */

	/* std::cout << "Source: " << source << std::endl;
	std::cout << "Regex: " << concat <myident> () << std::endl;

	std::regex re = compile <myident> ();

	std::sregex_iterator begin(source.begin(), source.end(), re);
	std::sregex_iterator end;

	std::cout << "Matches:\n";
	for (auto it = begin; it != end; it++) {
		std::smatch m = *it;
		parser::lexicon lptr = match <myident> (it);
		std::cout << "\t\tlptr = " << lptr->str() << std::endl;
	} */

	parser::Queue q = parser::lexq <myident> (source);
	while (!q.empty()) {
		parser::lexicon lptr = q.front();
		q.pop_front();

		std::cout << "Lexicon: " << lptr->str() << std::endl;
	}
}
