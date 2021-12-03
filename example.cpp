#include <iostream>

#include "nabu.hpp"

using namespace std;
using namespace nabu;

// Sources
StringFeeder sf = R"(154.54k)";

// Parsers
struct DIGIT {};

// TODO: how to set space ignore for rules? (set on by default) -> skips space at the beginning only
struct MyParser : public Parser <START> {
	MyParser(Feeder *fd) : Parser <START> (fd) {}
};

// Main function
int main()
{
	MyParser parser(&sf);
	
	ret *rptr;
	
	rptr = parser.grammar <double> ();
	std::cout << "rptr = " << rptr << "\n";
	std::cout << "Read number " << get <double> (rptr) << std::endl;
	
	rptr = parser.grammar <lit <'k'>> ();
	std::cout << "rptr = " << rptr << "\n";
	std::cout << "Read number " << get <char> (rptr) << std::endl;
}