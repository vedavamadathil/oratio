#include <iostream>

#include "nabu.hpp"

using namespace std;
using namespace nabu;

// Sources
StringFeeder sf = R"(--
	myIdent =	anotherIdent
)";

// Parsers
// TODO: how to set space ignore for rules? (set on by default) -> skips space at the beginning only
struct MyParser : public Parser <START> {
	MyParser(Feeder *fd) : Parser <START> (fd) {}
};

static const char walrus[] = ":=";
static const char minuses[] = "--";

// Main function
int main()
{
	// MyParser parser(&sf);
	
	ret *rptr;
	
	rptr = rule <str <walrus>> ::value(&sf);
	std::cout << "rptr = " << rptr << "\n";
	// std::cout << "rptr->value = \'" << get <std::string> (rptr) << "\'\n";
	
	rptr = rule <str <minuses>> ::value(&sf);
	std::cout << "rptr = " << rptr << "\n";
	std::cout << "rptr->value = \'" << get <std::string> (rptr) << "\'\n";

	ReturnVector rvec = getrv(seqrule <identifer, equals, identifer> ::value(&sf));

	// ReturnVector rvec = MyParser::kstar <alnum> ::value(&parser);
	std::cout << "b = " << (bool) rvec << "\n";
	std::cout << "sret.size = " << rvec.size() << "\n";

	/* for (auto &sret : rvec) {
		std::cout << "sret.str = \'" << get <std::string> (sret) << "\'\n";
		std::cout << "\tsret.char = \'" << get <char> (sret) << "\'\n";
	} */

	std::cout << "elem 0 = " << get <std::string> (rvec[0]) << "\n";
	std::cout << "elem 1 = " << get <char> (rvec[1]) << "\n";
	std::cout << "elem 2 = " << get <std::string> (rvec[2]) << "\n";
}