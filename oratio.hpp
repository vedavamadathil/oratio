#ifndef ORATIO_H_
#define ORATIO_H_

// Standard headers
#include <string>
#include <vector>

namespace oratio {

// Feeder ABC
class Feeder {
        bool _done = false;
public:
        virtual bool move() = 0;
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
        size_t          _index;
        std::string     _source;
public:
        StringFeeder(const char *str) : StringFeeder(std::string(str)) {}
        StringFeeder(const std::string &str) : _source(str), _index(0) {}

        // Virtual function overrides
        bool move() override {
                return (_index++ < _source.size());
        }

        char getc() const override {
                return _source[_index];
        }
};

struct Server {};

struct ret {};

// Literary value
template <char c>
struct lit {};

// Parser classes
enum class Grammars {
	START
};

// Parser class
template <class start>
struct Parser {
	Feeder *feeder;

	// Sequential rule return value
	using Seqret = std::vector <ret *>;

	// Rule structures
	template <class T>
	struct rule {
		static ret *value(Parser *) {
			return nullptr;
		}
	};

	template <char c>
	struct rule <lit <c>> {
		static ret *value(Parser *parser) {
			char n = parser->feeder->next();
			return (ret *) new char(n);
		}
	};

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
			ret *rptr = rule <T> ::value(parser);
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
			ret *rptr = rule <T> ::value(parser);
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
		return rule <T> ::value(this);
	}

	// Parse
	void parse() {
		return grammar <start> ();
	}

	void parse(Feeder *fd) {
		feeder = fd;
		parse();
	}
};


}

#endif
