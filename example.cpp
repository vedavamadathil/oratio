#include <iostream>

#include "nabu.hpp"

using namespace std;
using namespace nabu;

// Sources
StringFeeder sf = R"(myIdent)";

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
	/* rptr = parser.grammar <alnum> ();
	std::cout << "rptr = " << rptr << "\n";
	std::cout << "rptr->value = " << get <char> (rptr) << "\n"; */

	Seqret sret;
	bool b = MyParser::kplus <alnum> ::value(&parser, sret);
	std::cout << "b = " << b << "\n";
	std::cout << "sret.size = " << sret.size() << "\n";
}