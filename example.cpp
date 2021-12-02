#include <iostream>

#include "oratio.hpp"

using namespace std;
using namespace oratio;

// Main function
StringFeeder sf = R"(
y(x) = x
)";

int main()
{
	Parser <lit <'y'>> parser {.feeder = &sf};
	// ret *rptr = Parser::rule <lit <'y'>> ::value(&parser);
	ret *rptr = parser.grammar <lit <'y'>> ();
	std::cout << "rptr = " << rptr << std::endl;
}