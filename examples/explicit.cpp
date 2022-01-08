#include "nabu.hpp"

using namespace nabu;

StringFeeder sf = R"(identifier 456)";

using myident = rules::skipper <rules::identifier>;
using mynumber = rules::skipper <int>;

LexList_next(myident, mynumber);

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

	parser::Queue queue = parser::lexq <myident> (&sf);
	
	std::cout << "Queue items:\n";
	while (!queue.empty()) {
		parser::lexicon lptr = queue.front();
		queue.pop();
		std::cout << "\tlexicon: " << lptr->str() << std::endl;
	}
}