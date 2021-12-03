#include <iostream>

#include "nabu.hpp"

using namespace std;
using namespace nabu;

// Sources
StringFeeder sf = R"(myIdent=anotherIdent)";

// Parsers
// TODO: how to set space ignore for rules? (set on by default) -> skips space at the beginning only
struct MyParser : public Parser <START> {
	MyParser(Feeder *fd) : Parser <START> (fd) {}
};

// Main function
int main()
{
	MyParser parser(&sf);
	
	ret *rptr;
	/* rptr = parser.grammar <equals> ();
	std::cout << "rptr = " << rptr << "\n";
	std::cout << "rptr->value = \'" << get <char> (rptr) << "\'\n"; */

	ReturnVector rvec = MyParser::seqrule <identifer, equals, identifer> ::value(&parser);
	std::cout << "b = " << (bool) rvec << "\n";
	std::cout << "sret.size = " << rvec.size() << "\n";
}