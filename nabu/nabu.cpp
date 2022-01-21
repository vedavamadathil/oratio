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

	using g_macro = grammar <macro>;
	using g_assignment = grammar <assignment>;
	using g_statement = grammar <statement>;

	std::cout << "Source: " << source << std::endl;
	parser::Queue q = parser::lexq <identifier> (source);

// #define SHOW_TOKENS

#ifdef SHOW_TOKENS

	while (!q.empty()) {
		parser::lexicon lptr = q.front();
		q.pop_front();

		std::cout << "lexicon: " << lptr->str() << std::endl;
	}

#else

	parser::lexicon lptr;
        while ((lptr = g_statement::value(q)));

        // Parsing summary
	std::cout << "=== Glob ===" << std::endl;
	for (auto &i : glob.lexers) {
		std::cout << i.name << ": " << i.regex << std::endl;
	}

	for (auto &i : glob.parsers) {
		std::cout << i.name << ": " << i.code << ": (";
                for (const auto &a: i.args)
                        std::cout << a << ", ";

                std::cout << "): " << i.action << std::endl;
	}

#endif
        
        return 0;
}

// Global variables
Glob glob;

// Static variables
decltype(grammar_action <macro> ::state)
grammar_action <macro> ::state;
