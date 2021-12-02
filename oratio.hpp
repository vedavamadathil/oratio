#ifndef ORATIO_H_
#define ORATIO_H_

// Standard headers
#include <string>
#include <vector>

namespace oratio {

// Forward declarations
template <class start>
struct Parser;

// Feeder ABC
class Feeder {
        bool _done = false;
public:
        virtual bool move(int = 1) = 0;
        virtual char getc() const = 0;

        char next() {
                if (_done)
                        return EOF;

                char c = getc();
                _done = move();

                return c;
        }
};

// String feeder
class StringFeeder : public Feeder {
        int		_index;
        std::string	_source;
public:
        StringFeeder(const char *str) : StringFeeder(std::string(str)) {}
        StringFeeder(const std::string &str) : _source(str), _index(0) {}

        // Virtual function overrides
        bool move(int step) override {
		// Increment and check bounds
		_index += step;
		bool done = (_index >= _source.size());

		// Cap the index
		_index = std::min((int) _source.size(), std::max(0, _index));

		return done;
        }

        char getc() const override {
                return _source[_index];
        }
};

// Return type
struct ret {};

// Templates return type
template <class T>
struct Tret : public ret {
	T value;

	// Constructors and such
	Tret(const T &x) : value(x) {}
};

// Easy casting
template <class T>
constexpr T get(ret *rptr)
{
	return ((Tret <T> *) rptr)->value;
}

// Return value server and manager
struct Server {};

// Sequential rule return value
using Seqret = std::vector <ret *>;

// Rule structures:
// TODO: docs -> always backup the tokens/characters if unsuccessful
template <class start, class T, bool = true>
struct rule {
	static ret *value(Parser <start> *) {
		return nullptr;
	}
};

// Parser classes
struct START {};

// Literal parser classes
template <char c>
struct lit {};

// Parser class
template <class start>
struct Parser {
	Feeder *feeder;

	// Multirule (simultaneous) template
	template <class ... T>
	struct multirule {
		static ret *value(Parser *) {
			return nullptr;
		}
	};

	// NOTE: Multirule returns the first valid return
	template <class T, class ... U>
	struct multirule <T, U...> {
		static ret *value(Parser *parser) {
			ret *rptr = rule <start, T> ::value(parser);
			if (rptr)
				return rptr;

			return multirule <U...> ::value(parser);
		}
	};

	// Sequential rule
	template <class ... T>
	struct seqrule {
		static bool value(Parser *parser, Seqret &sret) {
			sret.clear();
			return false;
		}
	};

	template <class T, class ... U>
	struct seqrule <T, U...> {
		static bool value(Parser *parser, Seqret &sret) {
			ret *rptr = rule <start, T> ::value(parser);
			if (!rptr) {
				// Clear on failure
				sret.clear();
				return false;	// Failure on first token
			}

			if (seqrule <U...> ::value(parser, sret)) {
				sret.insert(sret.begin(), rptr);
				return true;
			}

			// Clear on failure
			sret.clear();
			return false;
		}
	};

	// Grammars
	template <class T>
	ret *grammar() {
		return rule <start, T> ::value(this);
	}

	// Constructors
	Parser(Feeder *fd) : feeder(fd) {}

	// Parse
	void parse() {
		return grammar <start> ();
	}

	void parse(Feeder *fd) {
		feeder = fd;
		parse();
	}

	// Start token type
	using entry = start;
};

// Rule structure specializations
template <class start, char c>
struct rule <start, lit <c>> {
	static ret *value(Parser <start> *parser) {
		char n = parser->feeder->next();
		if (n == c)
			return new Tret <char> (n);
		
		// Backup and return failure
		parser->feeder->move(-1);
		return nullptr;
		// return 0x1;	// TODO: add success and failure constant
	}
};

}

#endif
