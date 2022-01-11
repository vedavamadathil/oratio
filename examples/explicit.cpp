#include "nabu.hpp"

using namespace nabu;

std::string source = R"(identifier identifier 456)";

StringFeeder feeder(source);

using myident = rules::skipper <rules::identifier>;
using mynumber = rules::skipper <int>;

LexList_next(myident, mynumber);

set_id(myident, 1);
set_id(mynumber, 2);

#include <regex>

template <class T>
struct token {
	static constexpr int id = -1;
	static constexpr const char *regex = "";
};

#define mk_token(T, value, str)					\
	template <>						\
	struct token <T> {					\
		static constexpr int id = value;		\
		static constexpr const char *regex = str;	\
	};

mk_token(myident, 1, "[a-z]+");
mk_token(mynumber, 2, "[0-9]+");

template <class T>
struct LexList {
	static constexpr bool tail = true;
	
	using next = void;
};

#define lexlist_next(A, B) 				\
	template <>					\
	struct LexList <A> {				\
		static constexpr bool tail = false;	\
		using next = B;				\
	};

lexlist_next(myident, mynumber);

template <class T>
std::string concat()
{
	std::string result = "(" + std::string(token <T> ::regex) + ")";
	if (!LexList <T> ::tail)
		result += "|" + concat <typename LexList <T> ::next> ();
	return result;
}

template <class Head>
std::regex compile()
{
	return std::regex(
		concat <Head> (),
		std::regex::extended 
			| std::regex::optimize
	);
}

template <class Head>
nabu::ret match(std::sregex_iterator &it, int index = 0)
{
	// If the next one is empty, we have reached
	if (it->str(index + 1).empty()) {
		std::cout << "LexList index " << index << " -> Head = " << typeid(Head).name() << "\n";
	}
	
	// Walk the list of tokens (if any left)
	if (!LexList <Head> ::tail)
		return match <typename LexList <Head> ::next> (it, index + 1);

	return nullptr;
}

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

	std::cout << "Source: " << source << std::endl;
	std::cout << "Regex: " << concat <myident> () << std::endl;

	std::regex re = compile <myident> ();

	std::sregex_iterator begin(source.begin(), source.end(), re);
	std::sregex_iterator end;

	std::cout << "Matches:\n";
	for (auto it = begin; it != end; it++) {
		std::smatch m = *it;
		std::cout << "\tMatch: " << m.str() << std::endl;
		match <myident> (it);
	}
}
