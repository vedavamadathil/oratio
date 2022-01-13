#ifndef GACTIONS_H_
#define GACTIONS_H_

#include "lexer.hpp"

using nabu::parser::rd::grammar_action;

// Macro handler
template <>
struct grammar_action <macro> {
	static constexpr bool available = true;

	// Macro related variables
	static struct {
		std::string project;
		std::string entry;
	} state;

	// Macro handlers
	static void project(parser::Queue &q) {
		parser::lexicon lptr = q.front();
		if (lptr->id == token <identifier> ::id) {
			state.project = get <std::string> (lptr);
			q.pop_front();
		}
	}

	static void entry(parser::Queue &q) {
		if (!parser::expect <identifier, std::string> (q, state.entry)) {
			std::cout << "Expected identifier after @entry!\n";
		}
	}

	static void rules(parser::Queue &q) {
		std::cout << "Entering rules section\n";
	}

	// Grammar action
	static void action(parser::lexicon lptr, parser::Queue &q) {
		static std::unordered_map <
				std::string,
				void (*)(parser::Queue &)
			> handlers {
			{"@project", &project},
			{"@entry", &entry},
			{"@rules", &rules}
		};

		// std::cout << "MACRO: " << lptr->str() << std::endl;

		std::string macro = get <std::string> (lptr);
		if (handlers.find(macro) == handlers.end()) {
			std::cout << "BAD MACRO!\n";
			// TODO: error handling
			return;
		}

		// Handle the macro
		handlers[macro](q);

		// Expect a newline
		if (!parser::expect <newline> (q))
			std::cout << "Expected newline after macro!\n";
	}
};

// Warlus handler, for assignment
template <>
struct grammar_action <warlus> {
	static constexpr bool available = true;
};

#endif
