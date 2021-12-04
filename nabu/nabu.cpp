#include <iostream>

#include "rules.hpp"

using namespace std;
using namespace nabu;

// Literals constants and rules
const char walrus[] = ":=";

// All rule tags
set <std::string> tags;

// Sources
StringFeeder sf = R"(
paren := '(' expression ')'
number := k8 {body123}
	| digit*
	| identifier digit+ {body456}
)";

// Main function
int main()
{
	ret *rptr = rule <statement_list> ::value(&sf);
	/* ReturnVector rvec = getrv(rptr);
	std::cout << rvec.json() << std::endl; */
}