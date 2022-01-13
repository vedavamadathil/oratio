#include "lexer.hpp"
#include "gactions.hpp"

std::string read(const std::string &path)
{
	std::ifstream t(path);
	std::stringstream buffer;
	buffer << t.rdbuf();
	return buffer.str();
}

int main(int argc, char *argv[])
{
	using namespace nabu;

	if (argc < 2) {
		std::cout << "Usage: nabu <file.nabu>" << std::endl;
		return 1;
	}

	std::string source = read(argv[1]);
	
	using parser::rd::grammar;
	using parser::rd::option;

	using gmacro = grammar <macro>;

	std::cout << "Source: " << source << std::endl;
	parser::Queue q = parser::lexq <identifier> (source);
	/* while (!q.empty()) {
		parser::lexicon lptr = q.front();
		q.pop_front();

		std::cout << "lexicon: " << lptr->str() << std::endl;
	} */

	parser::lexicon lptr;
	lptr = grammar <void> ::value(q);
	while ((lptr = gmacro::value(q)));
}

// Static variables
decltype(grammar_action <macro> ::state)
grammar_action <macro> ::state;
