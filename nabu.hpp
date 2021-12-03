#ifndef NABU_H_
#define NABU_H_

// Standard headers
#include <string>
#include <vector>

namespace nabu {

// Forward declarations
template <class start>
struct Parser;

// Feeder ABC
class Feeder {
protected:
        bool _done = false;
public:
        virtual void move(int = 1) = 0;
        virtual char getc() const = 0;

        char next() {
                if (_done)
                        return EOF;

		// Get the next character and move
                char c = getc();
		move();

                return c;
        }
};

// String feeder
class StringFeeder : public Feeder {
        std::string	_source;
        int		_index;
public:
        StringFeeder(const char *str) : StringFeeder(std::string(str)) {}
        StringFeeder(const std::string &str) : _source(str), _index(0) {}

        // Virtual function overrides
        void move(int step) override {
		// Increment and check bounds
		_index += step;
		_done = (_index >= _source.size());

		// Cap the index
		_index = std::min((int) _source.size(), std::max(0, _index));
        }

        char getc() const override {
                return _source[_index];
        }
};

// Return type
class ret {};

// Constant returns
#define NABU_SUCCESS (ret *) 0x1
#define NABU_FAILURE nullptr

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
using Seqret = std::vector <ret *>;	// Should alias allocator's vector class

// Rule structures:
// TODO: docs -> always backup the tokens/characters if unsuccessful
template <class start, class T>
struct rule {
	static ret *value(Parser <start> *) {
		return nullptr;
	}
};

// Parser classes
struct START {};

// Parser class
template <class start>
struct Parser {
	Feeder *feeder;

	// Feeder getter methods
	char next() const {
		return feeder->next();
	}

	// Helper methods for rules
	void backup(int i = 1) const {
		feeder->move(-i);
	}

	ret *abort(int i = 1) const {
		// Move back and return
		feeder->move(-i);
		return NABU_FAILURE;
	}

	ret *noef(char c) const {
		backup(c != EOF);
		return NABU_FAILURE;
	}

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
		// TODO: refactor to process
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

	// Kleene star rule
	template <class T>
	struct kstar {
		static void value(Parser *parser, Seqret &sret) {
			ret *rptr;
			while ((rptr = rule <start, T> ::value(parser)))
				sret.push_back(rptr);
		}
	};

	// Kleene plus rule
	template <class T>
	struct kplus {
		// TODO: a static value method which returns seqret?
		// TODO: do above after implementing allocator and its vector
		static bool value(Parser *parser, Seqret &sret) {
			ret *rptr;
			while ((rptr = rule <start, T> ::value(parser)))
				sret.push_back(rptr);
			return (sret.size() > 0);
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

// Literal parser classes
template <char c>
struct lit {};

struct identifer {};

// Literal groups
struct digit {};
struct alpha {};
struct alnum {};

// Special characters
struct dot {};
struct comma {};

////////////////////////////////////
// Rule structure specializations //
////////////////////////////////////

// Character
template <class start, char c>
struct rule <start, lit <c>> {
	static ret *value(Parser <start> *pr) {
		char n = pr->next();
		if (n == c)
			return new Tret <char> (n);
		
		return pr->abort();
	}
};

// Identifier
template <class start>
struct rule <start, identifer> {
	static ret *value(Parser <start> *pr) {
		char n = pr->next();
		if (n == '_' || isalpha(n)) {
			std::string str;

			str += n;
			while (true) {
				n = pr->next();
				if (n == '_' || isalpha(n) || isdigit(n)) {
					str += n;
				} else {
					pr->backup();
					break;
				}
			}

			return new Tret <std::string> (str);
		}

		return pr->abort();
	}
};

// Digit
template <class start>
struct rule <start, digit> {
	static ret *value(Parser <start> *parser) {
		char n = parser->feeder->next();
		if (isdigit(n))
			return new Tret <int> (n - '0');
		
		return parser->noef(n);
	}
};

// Alpha
template <class start>
struct rule <start, alpha> {
	static ret *value(Parser <start> *parser) {
		char n = parser->feeder->next();
		if (isalpha(n))
			return new Tret <int> (n);
		
		return parser->noef(n);
	}
};

// Concatenated literal groups
template <class start>
struct rule <start, alnum> : public Parser <start> ::template multirule <digit, alpha> {};

// Rules for special characters
template <class start>
struct rule <start, dot> : public rule <start, lit <'.'>> {};

// Reading numbers
template <class start>
unsigned long long int atoi(Parser <start> *pr)
{
	// Return value
	unsigned long long int x = 0;

	char c = pr->next();
	while (isdigit(c)) {
		// Compute new value
		x = 10 * x + (c - '0');

		// Get the next character
		c = pr->next();
	}

	pr->backup();
	return x;
}

template <class start>
long double atof(Parser <start> *pr)
{
	// Temporary storage
	long double a = 0.0;
	int e = 0;
	int c;

	// Before decimal point
	while ((c = pr->next()) != '\0' && isdigit(c))
		a = a * 10.0 + (c - '0');

	// Decimal point
	if (c == '.') {
		while ((c = pr->next()) != '\0' && isdigit(c)) {
			a = a * 10.0 + (c - '0');
			e = e - 1;
		}
	}

	// Exponent
	if (c == 'e' || c == 'E') {
		// TODO: call the atoi helper function
		// Temporary variables
		int sign = 1;
		int i = 0;

		// Signs of exponent
		c = pr->next();
		if (c == '+') {
			c = pr->next();
		} else if (c == '-') {
			c = pr->next();
			sign = -1;
		}

		while (isdigit(c)) {
			i = i * 10 + (c - '0');
			c = pr->next();
		}

		// Apply the sign
		e += i * sign;
	}

	// TODO: move back?
	pr->backup();

	// Deal with the exponent
	while (e > 0) {
		a *= 10.0;
		e--;
	}

	while (e < 0) {
		a *= 0.1;
		e++;
	}

	// Return the value
	return a;
}

// Macro for creating templated type rules
#define template_type_rule(aux, type)				\
	template <class start>					\
	struct rule <start, type> {				\
		static ret *value(Parser <start> *parser) {	\
			return aux <start, type> (parser);	\
		}						\
	};

// Integer classes:
//	Will not return negative numbers
//	for the sake of simplicty. Negative
//	numbers can be added as a grammar.
template <class start, class I>
ret *integral_rule(Parser <start> *parser)
{
	// Read first character
	char n = parser->next();

	// Early failure
	if (!isdigit(n))
		return parser->abort();

	// Backup
	parser->backup();
	return new Tret <I> (atoi(parser));
}

template_type_rule(integral_rule, short int);
template_type_rule(integral_rule, int);
template_type_rule(integral_rule, long int);
template_type_rule(integral_rule, long long int);

// TODO: int8_t, int16_t, int32_t, int64_t and unsigned counter parts
// TODO: similar code for floating point
// TODO: create a templated helper function for integers and floating points
//	should also detect overflow
template <class start, class F>
static ret *float_rule(Parser <start> *parser) {
	// Read first character
	char n = parser->next();
	
	// Early failures
	if (!isdigit(n) && n != '.')
		return parser->abort();

	if (n == '.' && !isdigit(parser->next()))
		return parser->abort(2);

	// Backup
	parser->backup();

	// Get the value and return
	return new Tret <F> (atof(parser));
}

template_type_rule(float_rule, float);
template_type_rule(float_rule, double);
template_type_rule(float_rule, long double);

}

#endif