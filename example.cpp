#include <iostream>

#include "oratio.hpp"

using namespace std;
using namespace oratio;

// Sources
StringFeeder sf = R"(01
num := *digit
)";

// Parsers
struct DIGIT {};

struct MyParser : public Parser <START> {
	MyParser(Feeder *fd) : Parser <START> (fd) {}
};

template <>
struct rule <MyParser::entry, DIGIT> : MyParser::multirule <lit <'1'>, lit <'0'>> {};

// Main function
int main()
{
	MyParser parser(&sf);
	
	ret *rptr;
	rptr = parser.grammar <lit <'\n'>> ();
	std::cout << "rptr = " << rptr << std::endl;
	
	rptr = parser.grammar <DIGIT> ();
	std::cout << "rptr = \'" << get <char> (rptr) << "\'" << std::endl;
	
	rptr = parser.grammar <DIGIT> ();
	std::cout << "rptr = \'" << get <char> (rptr) << "\'" << std::endl;

	// parser.grammar <START> ();
}