#ifndef NABU_H_
#define NABU_H_

// Standard headers
#include <fstream>
#include <memory>
#include <stack>
#include <string>
#include <vector>

namespace nabu {

// Return type
class _ret {
public:
	virtual std::string str() const = 0;
};

// Smart pointer alias
using ret = std::shared_ptr <_ret>;

// Custom to_string() function
template <class T>
std::string to_string(const T& t)
{
	return std::to_string(t);
}

// Templates return type
template <class T>
struct Tret : public _ret {
	T value;

	// Constructors and such
	Tret(const T &x) : value(x) {}

	// Default to_string method
	// TODO: will to_string induce rtti?
	std::string str() const override {
		return std::to_string(value);
	}
};

template <>
std::string Tret <std::string> ::str() const {
	return "\"" + value + "\"";
}

template <>
std::string Tret <char> ::str() const {
	return std::string("\'") + value + "\'";
}

// Easy casting
template <class T>
T get(ret rptr)
{
	// TODO: add a NABU_NO_RTTI macro
	// to use dynamic cast here instead (for speed,
	// may want to use this!)
	return ((Tret <T> *) rptr.get())->value;
}

// Return vector class
// TODO: template from allocator
class ReturnVector : public _ret {
	std::vector <ret> _rets;

	using iterator = typename std::vector <ret >::iterator;
	using citerator = typename std::vector <ret >::const_iterator;
public:
	// Constructors
	ReturnVector() {}
	ReturnVector(const std::vector <ret > &rets) : _rets(rets) {}

	// Getters
	const size_t size() const {
		return _rets.size();
	}

	// Indexing
	ret operator[](size_t index) {
		return _rets[index];
	}

	const ret operator[](size_t index) const {
		return _rets[index];
	}

	// Iterators
	iterator begin() {
		return _rets.begin();
	}

	iterator end() {
		return _rets.end();
	}

	citerator begin() const {
		return _rets.begin();
	}

	citerator end() const {
		return _rets.end();
	}

	// TODO: add pushback at some point

	// As a boolean, returns non-empty
	explicit operator bool() const {
		return !_rets.empty();
	}

	// Printing
	std::string str() const override {
		std::string str = "{";
		for (size_t i = 0; i < _rets.size(); i++) {
			str += _rets[i]->str();

			if (i + 1 < _rets.size())
				str += ", ";
		}

		return str + "}";
	}
	
	std::string json_str() const {
		std::string str = "[";
		for (size_t i = 0; i < _rets.size(); i++) {
			ret rptr = _rets[i];

			ReturnVector *rvptr = dynamic_cast <ReturnVector *> (rptr.get());
			if (rvptr)
				str += rvptr->json_str();
			else
				str += rptr->str();

			if (i + 1 < _rets.size())
				str += ", ";
		}

		return str + "]";
	}

	// Hierarchical printing
	std::string json(int ilev = 1) {
		std::string indent(ilev - 1, '\t');
		std::string str = indent + "[\n";

		for (size_t i = 0; i < _rets.size(); i++) {
			ret rptr = _rets[i];

			// TODO: any alternative to dynamic_cast? or should it be at
			// the user's discretion? (after all, this should be used
			// for debugging or should be expected to be a slow process)
			ReturnVector *rvptr = dynamic_cast <ReturnVector *> (rptr.get());
			if (rvptr) {
				std::string normal = rvptr->json_str();
				if (normal.length() > 30)
					str += rvptr->json(ilev + 1);
				else
					str += '\t' + indent + normal;
			} else {
				str += '\t' + indent + rptr->str();
			}

			if (i + 1 < _rets.size())
				str += ",\n";
		}

		return str + '\n' + indent + "]";
	}
};

// Cast to ReturnVector
inline ReturnVector getrv(ret rptr)
{
	return *((ReturnVector *) rptr.get());
}

// Constant returns
#define NABU_SUCCESS (ret ) 0x1
#define NABU_FAILURE nullptr

// Feeder ABC
class Feeder {
protected:
        bool 			_done = false;

	// For pushing and popping characters
	std::stack <int>	_indices;

public:
        virtual void move(int = 1) = 0;
        virtual char getc() const = 0;
	virtual int cindex() const = 0;
	virtual size_t size() const = 0;

	// Retrieves the next character
        char next() {
                if (_done)
                        return EOF;

		// Get the next character and move
                char c = getc();
		move();

                return c;
        }

	// Notes current index into stack
	void checkpoint() {
		_indices.push(cindex());
	}

	// Restores state from checkpoint
	void respawn() {
		int c = cindex();
		int p = _indices.top();
		_indices.pop();

		// Move by the delta amount
		move(p - c);
	}

	// Erases last checkpoint
	bool erase_cp() {
		if (_indices.empty())
			return false;

		_indices.pop();
		return true;
	}

	// Read n characters
	std::string read(int n) {
		std::string out;

		int k = n;

		char c;
		while (k-- > 0) {
			c = next();

			if (c == EOF)	
				return out;
			
			out += c;
		}

		return out;
	}

	// Read until a character is reached (delimiter is also read)
	//	returns success boolean (whether the boolean was reached)
	//	and the actual string
	std::pair <bool, std::string> read_until(char c) {
		std::string out;

		char n;
		while (((n = next()) != EOF) && (n != c))
			out += n;
		
		return {n == c, out};
	}
	
	// Moves backward
	void backup(int i = 1) {
		move(-i);
	}

	// Skips white space
	void skip_space() {
		while (isspace(next()));

		// Move back
		if (getc() != EOF)
			move(-1);
	}

	// Skip white space without newline
	void skip_space_no_nl() {
		char c = getc();
		if (!isspace(c) || c == '\n')
			return;
		
		while (isspace(c) && c != '\n')
			c = next();

		// Move back
		if (getc() != EOF)
			move(-1);
	}

	// Moves back and returns failure
	ret abort(int i = 1) {
		// Move back and return
		move(-i);
		return NABU_FAILURE;
	}

	// Moves back one character if c is not EOF
	ret noef(char c) {
		backup(c != EOF);
		return NABU_FAILURE;
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

	virtual int cindex() const override {
		return _index;
	}

	virtual size_t size() const override {
		return _source.size();
	}

	// From file
	static StringFeeder from_file(const std::string &filename) {
		std::ifstream file(filename);

		// Read the file
		std::string str;

		file.seekg(0, std::ios::end);
		str.reserve(file.tellg());
		file.seekg(0, std::ios::beg);

		str.assign((std::istreambuf_iterator <char> (file)),
			   std::istreambuf_iterator <char> ());
			
		// Return the feeder
		return StringFeeder(str);
	}
};

// TODO: class for reading argument (and then storing results into a map)

// Rule structures:
// TODO: docs -> always backup the tokens/characters if unsuccessful
template <class T>
struct rule {
	static ret value(Feeder *) {
		return nullptr;
	}
};

// Special (alternate) return type for multirules
//	contains information about the index
//	of the rule that succeeded
using mt_ret = std::pair <int, ret >;

// Printing mt_rets
template <>
std::string Tret <mt_ret> ::str() const
{
	return "<" + std::to_string(value.first) + "," + value.second->str() + ">";
}

// Multirule (simultaneous) template
template <class ... T>
struct multirule {
	static ret value(Feeder *) {
		return nullptr;
	}
	
	static ret _process(Feeder *, int &prev) {
		prev = -1;
		return nullptr;
	}
protected:
	// This method will never return nullptr
	static mt_ret _value(Feeder *fd) {
		return {-1, nullptr};
	}
};

// NOTE: Multirule returns the first valid return
template <class T, class ... U>
struct multirule <T, U...> {
	static ret value(Feeder *fd) {
		ret rptr = rule <T> ::value(fd);
		if (rptr)
			return rptr;

		return multirule <U...> ::value(fd);
	}

	static ret _process(Feeder *fd, int &prev) {
		ret rptr = rule <T> ::value(fd);
		if (rptr)
			return rptr;

		prev++;
		return multirule <U...> ::_process(fd, prev);
	}
protected:
	static mt_ret _value(Feeder *fd) {
		int prev = 0;
		ret rptr = _process(fd, prev);
		return {prev, rptr};	
	}
};

// Sequential rule
template <class ... T>
struct seqrule {
	// TODO: refactor to process
	static bool _process(Feeder *fd, std::vector <ret > &rets, bool skip) {
		rets.clear();
		return false;
	}
	
	static ret value(Feeder *fd, bool skip = true) {
		return ret(new ReturnVector());
	}
protected:
	static ret _value(Feeder *fd, bool skip = true) {
		return value(fd, skip);
	}
};

// Base case
template <class T>
struct seqrule <T> {
	static bool _process(Feeder *fd, std::vector <ret > &sret, bool skip) {
		if (skip)
			fd->skip_space();

		ret rptr = rule <T> ::value(fd);
		if (rptr) {
			sret.push_back(rptr);
			return true;
		}

		return false;
	}
	
	static ret value(Feeder *fd, bool skip = true) {
		std::vector <ret > sret;
		if (_process(fd, sret, skip))
			return ret(new ReturnVector(sret));
		return nullptr;
	}
protected:
	// Protected method that is equal to value
	// 	exists to conveniently call from derived
	//	classes without instantiating seqrule
	static ret _value(Feeder *fd, bool skip = true) {
		return value(fd, skip);
	}
};

template <class T, class ... U>
struct seqrule <T, U...> {
	static bool _process(Feeder *fd, std::vector <ret > &sret, bool skip) {
		fd->checkpoint();			// Need to respawn on failure
		if (skip)
			fd->skip_space();
		
		ret rptr = rule <T> ::value(fd);
		if (!rptr) {
			// Clear on failure
			sret.clear();
			fd->respawn();			// Respawn on failure
			return false;	// Failure on first token
		}

		if (seqrule <U...> ::_process(fd, sret, skip)) {
			sret.insert(sret.begin(), rptr);
			fd->erase_cp();			// Erase checkpoint
			return true;
		}

		// Clear on failure
		sret.clear();
		fd->respawn();				// Respawn on failure
		return false;
	}
	
	static ret value(Feeder *fd, bool skip = true) {
		std::vector <ret > sret;
		if (_process(fd, sret, skip))
			return ret(new ReturnVector(sret));
		return nullptr;
	}
protected:
	static ret _value(Feeder *fd, bool skip = true) {
		return value(fd, skip);
	}
};

// Kleene star rule
template <class T>
struct kstar {
	static ret value(Feeder *fd) {
		std::vector <ret > rets;
		ret rptr;
		while ((rptr = rule <T> ::value(fd)))
			rets.push_back(rptr);
		return ret(new ReturnVector(rets));
	}
};

// Kleene plus rule
template <class T>
struct kplus {
	static ret value(Feeder *fd) {
		std::vector <ret > rets;
		ret rptr;
		while ((rptr = rule <T> ::value(fd)))
			rets.push_back(rptr);
		
		if (rets.size() > 0)
			return ret(new ReturnVector(rets));
		return nullptr;
	}
};

// Generic space skipper wrapper
template <class T>
struct skipper;

template <class T>
struct skipper_no_nl;

// Literal parser classes
template <char c>
struct lit {};


template <char c>
struct space_lit {};

// Extracts string up to delimiter
template <char c, bool read = true>
struct delim_str {};

template <const char *s>
struct str {};

// Character in C
struct cchar {};

// String in C
struct cstr {};

struct identifier {};

// Literal groups
struct digit {};
struct alpha {};
struct alnum {};

// Special characters
struct dot {};
struct comma {};
struct equals {};

////////////////////////////////////
// Rule structure specializations //
////////////////////////////////////

// Whitespace skippter
template <class T>
struct rule <skipper <T>> {
	static ret value(Feeder *fd) {
		fd->skip_space();
		return rule <T> ::value(fd);
	}
};

// Space without newline rule
template <class T>
struct rule <skipper_no_nl <T>> {
	static ret value(Feeder *fd) {
		fd->skip_space_no_nl();
		return rule <T> ::value(fd);
	}
};

// Character
template <char c>
struct rule <lit <c>> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (n == c)
			return ret(new Tret <char> (n));
		
		return fd->abort();
	}
};

// Space ignoring character
template <char c>
struct rule <space_lit <c>> {
	static ret value(Feeder *fd) {
		// Skip space first
		fd->skip_space();

		char n = fd->next();
		if (n == c)
			return ret(new Tret <char> (n));
		
		return fd->abort();
	}
};

// String delimiter extractor
template <char c, bool read>
struct rule <delim_str <c, read>> {
	static ret value(Feeder *fd) {
		// Skip space first
		auto out = fd->read_until(c);

		// Go before the delimiter
		if (!read)
			fd->backup();

		if (out.first)
			return ret(new Tret <std::string> (out.second));
		return nullptr;
	}
};

// String
template <const char *s>
struct rule <str <s>> {
	static ret value(Feeder *fd) {
		std::string str = s;

		// Record current index for popping
		fd->checkpoint();

		std::string get = fd->read(str.size());
		if (str == get) {
			fd->erase_cp();
			return ret(new Tret <std::string> (str));
		}
		
		// Restore index and fail
		fd->respawn();
		return NABU_FAILURE;
	}
};

// C character
template <>
struct rule <cchar> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (n != '\'')
			return fd->abort();
		
		char c = fd->next();
		if (c == '\\') {
			char n = fd->next();
			if (n == '\'')
				return ret(new Tret <char> (c));
			
			// TODO: throw error
			return fd->abort();
		}

		n = fd->next();
		if (n != '\'')
			return fd->abort();
		
		return ret(new Tret <char> (c));
	}
};

// C string
template <>
struct rule <cstr> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (n != '\"')
			return fd->abort();

		std::string str;

		// Read until closing quote
		while (true) {
			n = fd->next();
			if (n == '\\') {
				n = fd->next();

				switch (n) {
				case '\"':	str += '\"';	break;
				case '\'':	str += '\'';	break;
				case '\\':	str += '\\';	break;
				case 'a':	str += '\a';	break;
				case 'b':	str += '\b';	break;
				case 'f':	str += '\f';	break;
				case 'n':	str += '\n';	break;
				case 'r':	str += '\r';	break;
				case 't':	str += '\t';	break;
				case 'v':	str += '\v';	break;
				case '0':	str += '\0';	break;
				default:
					return fd->abort();	// TODO: throw error
				}
			} else if (n == '\"') {
				break;
			} else {
				str.push_back(n);
			}
		}

		return ret(new Tret <std::string> (str));
	}
};

// Identifier
template <>
struct rule <identifier> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (n == '_' || isalpha(n)) {
			std::string str;

			str += n;
			while (true) {
				n = fd->next();
				if (n == '_' || isalpha(n) || isdigit(n)) {
					str += n;
				} else {
					fd->backup();
					break;
				}
			}

			return ret(new Tret <std::string> (str));
		}

		return fd->abort();
	}
};

// Digit
template <>
struct rule <digit> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (isdigit(n))
			return ret(new Tret <int> (n - '0'));
		
		return fd->noef(n);
	}
};

// Alpha
template <>
struct rule <alpha> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (isalpha(n))
			return ret(new Tret <int> (n));
		
		return fd->noef(n);
	}
};

// Concatenated literal groups
template <>
struct rule <alnum> : public multirule <digit, alpha> {};

// Macro for generating rules for special characters
#define special_lit(name, c)						\
	template <> struct rule <name> : public rule <lit <c>> {};

special_lit(dot, '.');
special_lit(comma, ',');
special_lit(equals, '=');

// Reading numbers
unsigned long long int atoi(Feeder *fd)
{
	// Return value
	unsigned long long int x = 0;

	char c = fd->next();
	while (isdigit(c)) {
		// Compute new value
		x = 10 * x + (c - '0');

		// Get the next character
		c = fd->next();
	}

	fd->backup();
	return x;
}

long double atof(Feeder *fd)
{
	// Temporary storage
	long double a = 0.0;
	int e = 0;
	int c;

	// Before decimal point
	while ((c = fd->next()) != '\0' && isdigit(c))
		a = a * 10.0 + (c - '0');

	// Decimal point
	if (c == '.') {
		while ((c = fd->next()) != '\0' && isdigit(c)) {
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
		c = fd->next();
		if (c == '+') {
			c = fd->next();
		} else if (c == '-') {
			c = fd->next();
			sign = -1;
		}

		while (isdigit(c)) {
			i = i * 10 + (c - '0');
			c = fd->next();
		}

		// Apply the sign
		e += i * sign;
	}

	// TODO: move back?
	fd->backup();

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
#define template_type_rule(aux, type)			\
	template <>					\
	struct rule <type> {				\
		static ret value(Feeder *fd) {		\
			return aux <type> (fd);	\
		}					\
	};

// Integer classes:
//	Will not return negative numbers
//	for the sake of simplicty. Negative
//	numbers can be added as a grammar.
template <class I>
ret integral_rule(Feeder *fd)
{
	// Read first character
	char n = fd->next();

	// Early failure
	if (!isdigit(n))
		return fd->abort();

	// Backup
	fd->backup();
	return ret(new Tret <I> (atoi(fd)));
}

template_type_rule(integral_rule, short int);
template_type_rule(integral_rule, int);
template_type_rule(integral_rule, long int);
template_type_rule(integral_rule, long long int);

// TODO: int8_t, int16_t, int32_t, int64_t and unsigned counter parts
// TODO: similar code for floating point
// TODO: create a templated helper function for integers and floating points
//	should also detect overflow
template <class F>
static ret float_rule(Feeder *fd) {
	// Read first character
	char n = fd->next();
	
	// Early failures
	if (!isdigit(n) && n != '.')
		return fd->abort();

	if (n == '.' && !isdigit(fd->next()))
		return fd->abort(2);

	// Backup
	fd->backup();

	// Get the value and return
	return ret(new Tret <F> (atof(fd)));
}

template_type_rule(float_rule, float);
template_type_rule(float_rule, double);
template_type_rule(float_rule, long double);

}

#endif