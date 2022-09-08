/* MIT License
 *
 * Copyright (c) 2021 Venkataram Edavamadathil Sivaram
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE
 */

#ifndef NABU_H_
#define NABU_H_

// Standard headers
#include <cassert>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <regex>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace nabu {

// Built-in typename system for debugging
#if defined(NABU_DEBUG_RULES) || defined(NABU_DEBUG_PARSER)

#define NABU_DEBUG_GENERIC

// Default type name
template <class T, class ... Args>
struct _type_name {
	static std::string name() {
		return _type_name <T> ::name() + std::string(", ")
			+ _type_name <Args...> ::name();
	}
};

// Specialization for single type
template <class T>
struct _type_name <T> {
	static std::string name() {
		return typeid(T).name();
	}
};

template <class ... Args>
std::string type_name()
{
	return "<" + _type_name <Args...> ::name() + ">";
}

#define register(T) \
	template <> \
	struct nabu::_type_name <T> { \
		static std::string name() { \
			return #T; \
		} \
	};

#else

#define register(T)

#endif

namespace cc {		// Compile-time utilities

// TODO: add nabu prefixes, because this namespace is being
// put in the global space

template <std::size_t n>
struct constant_index : std::integral_constant <std::size_t, n> {};

template <typename id, std::size_t rank, std::size_t acc>
constexpr constant_index <acc> counter_crumb(id,
		constant_index <rank>,
		constant_index <acc>) { return {}; }

#define COUNTER_READ_CRUMB(TAG, RANK, ACC ) counter_crumb( 	\
	TAG(), constant_index <RANK> (),			\
	constant_index <ACC> ())

#define COUNTER_READ( TAG ) COUNTER_READ_CRUMB(TAG, 1,		\
	COUNTER_READ_CRUMB(TAG, 2,				\
	COUNTER_READ_CRUMB(TAG, 4,				\
	COUNTER_READ_CRUMB(TAG, 8,				\
	COUNTER_READ_CRUMB(TAG, 16,				\
	COUNTER_READ_CRUMB(TAG, 32,				\
	COUNTER_READ_CRUMB(TAG, 64,				\
	COUNTER_READ_CRUMB(TAG, 128, 0))))))))

#define COUNTER_INC( TAG ) 						\
	constexpr constant_index< COUNTER_READ( TAG ) + 1 > 		\
	counter_crumb(TAG,						\
	constant_index <(COUNTER_READ(TAG) + 1) & ~COUNTER_READ(TAG)>,	\
	constant_index <(COUNTER_READ(TAG) + 1) & COUNTER_READ(TAG)>	\
	) { return {}; }

}			// Compile-time utilities

// Return type
class _ret {
public:
	virtual std::string str() const = 0;
};

// Smart pointer alias
using ret = std::shared_ptr <_ret>;

// Custom to_string() function
// TODO: why is this necessary
template <class T>
inline std::string convert_string(const T &t)
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
		return convert_string(value);
	}
};

template <>
inline std::string Tret <std::string> ::str() const {
	return "\"" + value + "\"";
}

template <>
inline std::string Tret <char> ::str() const {
	return std::string("\'") + value + "\'";
}

// Easy casting
template <class T>
T get(ret rptr)
{
	// TODO: add a NABU_NO_RTTI macro
	// to use dynamic cast here instead (for speed,
	// may want to use this!) -> throw if so
	return ((Tret <T> *) rptr.get())->value;
}

// Return vector class
// TODO: template from allocator
class ReturnVector : public _ret {
	std::vector <ret> _rets;

	using iterator = typename std::vector <ret>::iterator;
	using citerator = typename std::vector <ret>::const_iterator;
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

	// Inserting
	void push_back(ret rptr) {
		_rets.push_back(rptr);
	}

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

// Special (alternate) return type for multiplexs
//	contains information about the index
//	of the rule that succeeded
using mt_ret = std::pair <int, ret>;

// Printing mt_rets
template <>
inline std::string Tret <mt_ret> ::str() const
{
	return "<" + std::to_string(value.first) + ", "
		+ value.second->str() + ">";
}

// Constant returns
#define NABU_SUCCESS (ret ) 0x1
#define NABU_FAILURE nullptr

// Feeder ABC
class Feeder {
protected:
        // bool 		_done = false;

	// For pushing and popping characters
	std::stack <int>	_indices;
public:
        virtual void move(int = 1) = 0;
        virtual char getc() const = 0;
	virtual int cindex() const = 0;
	virtual size_t size() const = 0;

	// Position tracking
	virtual size_t line() const = 0;
	virtual size_t col() const = 0;

	// Convenience methods
	virtual std::string get_line(size_t) const = 0;

	// Source of characters
	virtual const std::string &source() const = 0;

	// Retrieves the current character and moves
        char next() {
		// Get the next character and move
                char c = getc();
		move();

                return c;
        }

	// Notes current index into stack
	void checkpoint() {
		_indices.push(cindex());
	}

	// Custom index
	void checkpoint(int index) {
		_indices.push(index);
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

	// TODO: refactor checkpoint methods,
	// and add a way to add named checkpoints

	// Dump checkpoints
	void dump_cps() {
		auto cpy = _indices;
		std::cout << "Checkpoint stack: ";
		while (!cpy.empty()) {
			int index = cpy.top();
			cpy.pop();

			std::cout << index << "\t";
		}
		std::cout << "\n";
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
		// Return string
		std::string out;

		// Loop until EOF or character
		char n;
		while (((n = next()) != EOF) && (n != c))
			out += n;

		// Return status
		return {n == c, out};
	}

	// Moves backward
	void backup(int i = 1) {
		move(-i);
	}

	// Skips white space
	void skip_space() {
		if (!isspace(getc()))
			return;

		// Loop until whitespace
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
		backup(i);
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
	std::string	_loc;
        std::string	_source;
        int		_index;
public:
        StringFeeder(const char *str) : StringFeeder(std::string(str)) {}
        StringFeeder(const std::string &str, const std::string &loc = "")
		: _loc(loc), _source(str), _index(0) {}

        // Virtual function overrides
        void move(int step) override {
		// Increment and check bounds
		_index += step;

		// Cap the index
		_index = std::min((int) _source.size(), std::max(0, _index));
        }

        char getc() const override {
		if (_index >= size())
			return EOF;
                return _source[_index];
        }

	virtual int cindex() const override {
		return _index;
	}

	virtual size_t size() const override {
		return _source.size();
	}

	// Position tracking
	virtual size_t line() const override {
		size_t line = 1;
		for (size_t i = 0; i <= _index; i++)
			line += (_source[i] == '\n');
		return line;
	}

	// Get the current column
	virtual size_t col() const override {
		size_t col = 1;
		for (size_t i = 0; i < _index; i++) {
			if (_source[i] == '\n')
				col = 1;
			else
				col++;
		}
		return col;
	}

	// Get a line
	virtual std::string get_line(size_t line) const override {
		std::string out;

		size_t l = 1;
		for (size_t i = 0; i < _source.size(); i++) {
			if (_source[i] == '\n') {
				if (++l > line)
					break;
			} else if (l == line) {
				out += _source[i];
			}
		}

		return out;
	}

	// Get location of source
	virtual const std::string &source() const override {
		return _loc;
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
		return StringFeeder(str, filename);
	}
};

// Color constants
#define NABU_RESET_COLOR "\033[0m"
#define NABU_BOLD_COLOR "\033[1m"
#define NABU_ITALIC_COLOR "\033[3m"
#define NABU_ERROR_COLOR "\033[91;1m"
#define NABU_OK_COLOR "\033[92;1m"
#define NABU_WARNING_COLOR "\033[93;1m"
#define NABU_NOTE_COLOR "\033[94;1m"

// Class for reading command line arguments
class ArgParser {
public:
	// Public aliases
	using Args = std::vector <std::string>;
	using StringMap = std::unordered_map <std::string, std::string>;
	using ArgsMap = std::unordered_map <std::string, int>;
private:
	// Set of all options
	std::set <std::string>	_optns;

	// Does the option take an argument?
	std::set <std::string>	_optn_args;

	// List of all aliases
	std::vector <Args>	_aliases;

	// Map options to aliases
	ArgsMap			_alias_map;

	// Description for each option
	StringMap		_descriptions;

	// Map each option and its argument
	//	empty means that its a boolean
	//	based arg (present/not present)
	StringMap		_matched_args;

	// Positional arguments
	Args			_pargs;

	// Name of the command
	std::string		_name;

	// Number of required positional arguments
	int			_nargs = -1;

	// Helper methods
	bool _optn_arg(const std::string &str) const {
		return _optn_args.find(str) != _optn_args.end();
	}

	bool _optn_present(const std::string &str) const {
		return _optns.find(str) != _optns.end();
	}

	bool _is_optn(const std::string &str) const {
		if (str.empty())
			return false;

		// The second hyphen is a redundant check
		return (str[0] == '-');
	}

	// Set value of option (and aliases)
	void _set_optn(const std::string &str, const std::string &val) {
		int index = _alias_map[str];

		// Set the aliases
		for (const auto &alias : _aliases[index])
			_matched_args[alias] = val;
	}

	// Parse options
	void _parse_option(int argc, char *argv[],
			const std::string &arg, int &i) {
		// Check help first
		if (arg == "-h" || arg == "--help") {
			help();
			exit(0);
		}

		// Check if option is not present
		if (!_optn_present(arg)) {
			fprintf(stderr, "%s%s: %serror:%s unknown option %s\n",
				NABU_BOLD_COLOR, _name.c_str(), NABU_ERROR_COLOR,
				NABU_RESET_COLOR, arg.c_str());
			exit(-1);
		}

		// Handle arguments
		if (_optn_arg(arg)) {
			if ((++i) >= argc) {
				fprintf(stderr, "%s%s: %serror:%s option %s need an argument\n",
					NABU_BOLD_COLOR, _name.c_str(),
					NABU_ERROR_COLOR, NABU_RESET_COLOR,
					arg.c_str());
				exit(-1);
			}

			_set_optn(arg, argv[i]);
		} else {
			_set_optn(arg, "");
		}
	}

	// Convert string to value for methods
	template <class T>
	T _convert(const std::string &) const {
		return T();
	}
public:
	// Option struct

	// TODO: option to provide argument name in help
	struct Option {
		Args		aliases;
		std::string	descr;
		bool		arg;

		// Constructor
		Option(const std::string &str, const std::string &descr = "",
			bool arg = false) : aliases {str}, descr(descr), arg(arg) {}

		Option(const Args &aliases, const std::string &descr = "",
			bool arg = false) : aliases(aliases), descr(descr), arg(arg) {}
	};

	// Number of required positional arguments
	ArgParser(const std::string &name = "", int nargs = -1)
			: _name(name), _nargs(nargs) {
		add_optn(Args {"-h", "--help"}, "show this message");
	}

	// Full constructor, with all options
	ArgParser(const std::string &name, int nargs,
			const std::vector <Option> &opts) : ArgParser(name, nargs) {
		for (const auto &opt : opts)
			add_optn(opt.aliases, opt.descr, opt.arg);
	}

	// Add an option
	void add_optn(const std::string &str, const std::string &descr = "",
			bool arg = false) {
		// Add the option
		_optns.insert(str);

		// Add option to those which take an argument
		if (arg)
			_optn_args.insert(str);

		// Add the description
		_descriptions[str] = descr;

		// Add the alias
		_aliases.push_back(Args {str});

		// Alias map to nullptr
		_alias_map[str] = (int) _aliases.size() - 1;
	}

	// Add an option with aliases
	void add_optn(const Args &args, const std::string &descr = "",
			bool arg = false) {
		// Add the options
		for (const auto &arg : args)
			_optns.insert(arg);

		// Add option to those which take an argument
		if (arg) {
			for (const auto &arg : args)
				_optn_args.insert(arg);
		}

		// Add the descriptions
		for (const auto &arg : args)
			_descriptions[arg] = descr;

		// Add the aliases
		_aliases.push_back(args);

		// Add to the alias map
		for (const auto &arg : args)
			_alias_map[arg] = (int) _aliases.size() - 1;
	}

	void parse(int argc, char *argv[]) {
		// Set name if empty
		if (_name.empty())
			_name = argv[0];

		// Process the arguments
		for (int i = 1; i < argc; i++) {
			std::string arg = argv[i];
			if (_is_optn(arg))
				_parse_option(argc, argv, arg, i);
			else
				_pargs.push_back(arg);
		}

		// Check number of positional args
		if (_nargs > 0 && _pargs.size() < _nargs) {
			fprintf(stderr, "%s%s: %serror:%s requires %d argument%c,"
				" was only provided %lu\n",
				NABU_BOLD_COLOR, _name.c_str(),
				NABU_ERROR_COLOR, NABU_RESET_COLOR,
				_nargs, 's' * (_nargs != 1),
				_pargs.size());
			exit(-1);
		}
	}

	const Args &pargs() const {
		return _pargs;
	}

	// Retrieving positional arguments
	template <class T = std::string>
	inline T get(size_t i) const {
		return _convert <T> (_pargs[i]);
	}

	// Retrieve optional arguments
	template <class T = std::string>
	inline T get_optn(const std::string &str) {
		// Check if its a valid option
		if (_optns.find(str) == _optns.end())
			throw bad_option(str);

		// Check if the option even takes arguments
		if (!_optn_arg(str))
			throw optn_no_args(str);

		// Check if the option value is null
		if (_matched_args.find(str) == _matched_args.end())
			throw optn_null_value(str);

		// Return the converted value
		return _convert <T> (_matched_args[str]);
	}

	// Print error as a command
	int error(const std::string &str) const {
		fprintf(stderr, "%s%s: %serror:%s %s\n",
			NABU_BOLD_COLOR, _name.c_str(), NABU_ERROR_COLOR,
			NABU_RESET_COLOR, str.c_str());
		return -1;
	}

	// Print warning as a command
	int warning(const std::string &str) const {
		fprintf(stderr, "%s%s: %swarning:%s %s\n",
			NABU_BOLD_COLOR, _name.c_str(), NABU_WARNING_COLOR,
			NABU_RESET_COLOR, str.c_str());
		return 0;
	}

	// Print help
	void help() {
		// Print format of command
		printf("usage: %s", _name.c_str());
		for (const Args &aliases : _aliases) {
			// Just use the first alias
			std::string optn = aliases[0];
			printf(" [%s%s]", optn.c_str(),
				_optn_arg(optn) ? " arg" : "");
		}
		printf("\n");

		// Stop if no optional arguments
		if (_optns.empty())
			return;

		// Print description
		printf("\noptional arguments:\n");
		for (const Args &alias : _aliases) {
			std::string combined;
			for (size_t i = 0; i < alias.size(); i++) {
				combined += alias[i];

				if (i != alias.size() - 1)
					combined += ", ";
			}

			printf("  %*s", 20, combined.c_str());

			std::string descr = _descriptions[alias[0]];
			if (descr.empty())
				printf(" [?]\n");
			else
				printf(" %s\n", descr.c_str());
		}
	}

	// For debugging
	void dump() {
		std::cout << "Positional arguments: ";
		for (size_t i = 0; i < _pargs.size(); i++) {
			std::cout << "\"" << _pargs[i] << "\"";
			if (i + 1 < _pargs.size())
				std::cout << ", ";
		}
		std::cout << std::endl;

		for (const Args &alias : _aliases) {
			std::string combined;
			for (size_t i = 0; i < alias.size(); i++) {
				combined += alias[i];

				if (i != alias.size() - 1)
					combined += ", ";
			}

			std::cout << "\t" << std::left << std::setw(20)
				<< combined << " ";

			std::string optn = alias[0];
			if (_matched_args.find(optn) == _matched_args.end()) {
				std::cout << "Null\n";
			} else {
				std::string value = _matched_args[optn];

				std::cout << (value.empty() ? "Present" : value) << "\n";
			}
		}
	}

	// Thrown if not an option
	class bad_option : public std::runtime_error {
	public:
		bad_option(const std::string &str) :
			std::runtime_error("ArgParser: has no registered"
				" option \"" + str + "\"") {}
	};

	// Thrown if the option does not take an argument
	class optn_no_args : public std::runtime_error {
	public:
		optn_no_args(const std::string &str) :
			std::runtime_error("ArgParser: option \"" +
				str + "\" does not take arguments") {}
	};

	// Thrown if the option is null
	class optn_null_value : public std::runtime_error {
	public:
		optn_null_value(const std::string &str) :
			std::runtime_error("ArgParser: option \"" +
				str + "\" has null value (not specified)") {}
	};
};

template <>
inline std::string ArgParser::_convert <std::string> (const std::string &str) const {
	return str;
}

// Integral types
template <>
inline long long int ArgParser::_convert <long long int> (const std::string &str) const {
	return std::stoll(str);
}

template <>
inline long int ArgParser::_convert <long int> (const std::string &str) const {
	return _convert <long long int> (str);
}

template <>
inline int ArgParser::_convert <int> (const std::string &str) const {
	return _convert <long long int> (str);
}

// Floating types
template <>
inline long double ArgParser::_convert <long double> (const std::string &str) const {
	return std::stold(str);
}

template <>
inline double ArgParser::_convert <double> (const std::string &str) const {
	return _convert <long double> (str);
}

template <>
inline float ArgParser::_convert <float> (const std::string &str) const {
	return _convert <long double> (str);
}

// Boolean conversion
template <>
inline bool ArgParser::_convert <bool> (const std::string &str) const {
	return str == "true" || str == "1";
}

// Get option for booleans
//	special case because options that take no
//	arguments are true or false depending on
//	whether the option is present or not
template <>
inline bool ArgParser::get_optn <bool> (const std::string &str) {
	// Check if its a valid option
	if (_optns.find(str) == _optns.end())
		throw bad_option(str);

	// If its empty and no argument, return true
	if (!_optn_arg(str)) {
		if (_matched_args.find(str) == _matched_args.end())
			return false;
		if (_matched_args[str].empty())
			return true;
		throw optn_no_args(str);
	}

	// Check if the option value is null
	if (_matched_args.find(str) == _matched_args.end())
		throw optn_null_value(str);

	// Return the converted value
	return _convert <bool> (_matched_args[str]);
}

namespace rules {

// Rule structures
// TODO: docs -> always backup the tokens/characters if unsuccessful
template <class T>
struct rule {
	static ret value(Feeder *) {
		return nullptr;
	}
};

// Type to string converter
template <class T>
struct name {
	// static constexpr char value[] = "?";
	static const std::string value;
};

// template <class T>
// constexpr char name <T> ::value[];

template <class T>
const std::string name <T> ::value = typeid(T()).name();

// Setting grammar off
template <class T>
struct grammar_debug_off {
	static constexpr bool value = false;
};

template <class T>
constexpr bool grammar_debug_off <T> ::value;

// Macro for the above
#define set_debug_off(T)					\
	template <>						\
	struct nabu::grammar_debug_off <T> {			\
		static constexpr bool value = true;		\
	};							\
								\
	constexpr bool nabu::grammar_debug_off <T> ::value;

// State of debug printing
// 	has a dummy template parameter
// 	to keep in this header
template <class T = void>
struct printing {
	static size_t level;
	static size_t indent;

	// Get indent string
	static std::string get_indent() {
		return std::string(indent * level, ' ');
	}

	static std::string get_next_indent() {
		return std::string(indent * (level + 1), ' ');
	}
};

// Indent level
template <class T>
size_t printing <T> ::level = 0;

// Indent size
template <class T>
size_t printing <T> ::indent = 2;

// Debugging wrapper for rule
template <class T>
struct grammar {
	static const char reset[];
	static const char tag[];
	static const char success[];
	static const char failure[];

	static ret value(Feeder *fd) {

#ifdef NABU_DEBUG_RULES

		// Early exit
		if (grammar_debug_off <T> ::value)
			return rule <T> ::value(fd);

		std::string indent = printing <> ::get_indent();

		// Print and evaluate
		std::cout << indent << "<" << tag << name <T> ::value
			<< reset << ">\n";

		printing <> ::level++;
		ret rptr = rule <T> ::value(fd);

		std::string nindent = printing <> ::get_next_indent();
		if (rptr)
			std::cout << nindent << success << "SUCCESS" << reset << " [" << rptr->str() << "]\n";
		else
			std::cout << nindent << failure << "FAILURE\n" << reset;

		std::cout << indent << "<" << tag << "/" << name <T> ::value
			<< reset << ">\n\n";
		printing <> ::level--;

		// Return value
		return rptr;

#else

	// Redirect to plain rule
	return rule <T> ::value(fd);

#endif

	}
};

// Static variables
template <class T>
const char grammar <T> ::reset[] = "\033[0m";

template <class T>
const char grammar <T> ::tag[] = "\033[4;33m";

template <class T>
const char grammar <T> ::success[] = "\033[1;92m";

template <class T>
const char grammar <T> ::failure[] = "\033[1;91m";

// multiplex (simultaneous) template
template <class ... T>
struct multiplex {
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

// NOTE: multiplex returns the first valid return
template <class T, class ... U>
struct multiplex <T, U...> {
	static ret value(Feeder *fd) {
		// Evaluate
		ret rptr = grammar <T> ::value(fd);
		if (rptr)
			return rptr;

		return multiplex <U...> ::value(fd);
	}

	static ret _process(Feeder *fd, int &prev) {
		ret rptr = grammar <T> ::value(fd);
		if (rptr)
			return rptr;

		prev++;
		return multiplex <U...> ::_process(fd, prev);
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
struct sequential {
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

// Sequential rules
template <class T>
struct sequential <T> {
	static ret value(Feeder *fd, bool skip = true) {
		fd->checkpoint();				// Create checkpoint
		std::vector <ret > sret;
		if (_process(fd, sret, skip)) {
			fd->erase_cp();				// Erase checkpoint
			return ret(new ReturnVector(sret));
		}

		fd->respawn();					// Reset at checkpoint
		return nullptr;
	}

	static bool _process(Feeder *fd, std::vector <ret> &sret, bool skip) {
		if (skip)
			fd->skip_space();

		ret rptr = grammar <T> ::value(fd);
		if (rptr) {
			sret.push_back(rptr);
			return true;
		}

		return false;
	}
protected:
	// Protected method that is equal to value
	// 	exists to conveniently call from derived
	//	classes without instantiating sequential
	static ret _value(Feeder *fd, bool skip = true) {
		return value(fd, skip);
	}
};

template <class T, class ... U>
struct sequential <T, U...> {
	static ret value(Feeder *fd, bool skip = true) {
		fd->checkpoint();				// Create checkpoint
		std::vector <ret > sret;
		if (_process(fd, sret, skip)) {
			fd->erase_cp();				// Erase checkpoint
			return ret(new ReturnVector(sret));
		}

		fd->respawn();					// Reset at checkpoint
		return nullptr;
	}

	static bool _process(Feeder *fd, std::vector <ret> &sret, bool skip) {
		if (skip)
			fd->skip_space();

		ret rptr = grammar <T> ::value(fd);
		if (!rptr) {
			// Clear on failure
			sret.clear();
			return false;	// Failure on first token
		}

		if (sequential <U...> ::_process(fd, sret, skip)) {
			sret.insert(sret.begin(), rptr);
			return true;
		}

		// Clear on failure
		sret.clear();
		return false;
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
		std::vector <ret> rets;
		ret rptr;
		while ((rptr = grammar <T> ::value(fd)))
			rets.push_back(rptr);
		return ret(new ReturnVector(rets));
	}
protected:
	static ret _value(Feeder *fd) {
		return value(fd);
	}
};

// Kleene plus rule
template <class T>
struct kplus {
	static ret value(Feeder *fd) {
		std::vector <ret> rets;
		ret rptr;
		while ((rptr = grammar <T> ::value(fd)))
			rets.push_back(rptr);

		if (rets.size() > 0)
			return ret(new ReturnVector(rets));
		return nullptr;
	}
protected:
	static ret _value(Feeder *fd) {
		return value(fd);
	}
};

// TODO: share these prebuilt structs in the nabu namespace,
// do the same thing in parser and rules subspace

// Empty rule
struct epsilon {};

// Space character
struct space {};

// Generic space skipper wrapper
template <class T>
struct skipper {};

template <class T>
struct skipper_no_nl {};

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

// Identifier and word
struct word {};
struct identifier {};

// Literal groups
struct digit {};
struct alpha {};
struct alnum {};

// Special characters
struct dot {};
struct comma {};
struct equals {};

// For templates
template <class T>
struct name <skipper <T>> {
	static const std::string value;
};

template <class T>
const std::string name <skipper <T>> ::value = "skipper <" + std::string(name <T> ::value) + ">";

// Skipper no newline
template <class T>
struct name <skipper_no_nl <T>> {
	static const std::string value;
};

template <class T>
const std::string name <skipper_no_nl <T>> ::value = "skipper_no_nl <" + std::string(name <T> ::value) + ">";

// Literary characters
template <char c>
struct name <lit <c>> {
	static const std::string value;
};

template <char c>
const std::string name <lit <c>> ::value = std::string("lit <\'") + c + "\'>";

// Literary string
template <const char *s>
struct name <str <s>> {
	static const std::string value;
};

template <const char *s>
const std::string name <str <s>> ::value = std::string("str <\"") + s + "\">";

// Space literal
template <char c>
struct name <space_lit <c>> {
	static const std::string value;
};

template <char c>
const std::string name <space_lit <c>> ::value = std::string("space_lit <\'") + c + "\'>";

// Delimter string
template <char c>
struct name <delim_str <c>> {
	static const std::string value;
};

template <char c>
const std::string name <delim_str <c>> ::value = std::string("delim_str <\'") + c + "\'>";

////////////////////////////////////
// Rule structure specializations //
////////////////////////////////////

// Empty rule
template <>
struct rule <epsilon> {
	static ret value(Feeder *fd) {
		return ret(new Tret <std::string> ("ε"));
	}
};

// Space character
template <>
struct rule <space> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (isspace(n))
			return ret(new Tret <char> (n));

		return fd->abort();	// TODO: isnt this neof?
	}
};

// Whitespace skippter
template <class T>
struct rule <skipper <T>> {
	static ret value(Feeder *fd) {
		fd->skip_space();
		return grammar <T> ::value(fd);
	}
};

// Space without newline rule
template <class T>
struct rule <skipper_no_nl <T>> {
	static ret value(Feeder *fd) {
		fd->skip_space_no_nl();
		return grammar <T> ::value(fd);
	}
};

// Character
template <char c>
struct rule <lit <c>> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (n == c)
			return ret(new Tret <char> (n));

		return fd->noef(n);
	}
};

// Space ignoring character
template <char c>
struct rule <space_lit <c>> {
	static ret value(Feeder *fd) {
		// Skip space first
		fd->skip_space();

		char n = fd->getc();
		if (n == c) {
			// Move ahead
			fd->next();

			return ret(new Tret <char> (n));
		}

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
		fd->checkpoint();		// Create checkpoint

		char n = fd->next();
		if (n != '\"') {
			fd->respawn();		// Reset at checkpoint
			return NABU_FAILURE;
		}

		std::string str;

		// Read until closing quote or EOF
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
			} else if (n == EOF) {
				// TODO: checkpoint
				// Respawn to checkpoint and return nullptr
				fd->respawn();
				return nullptr;
			} else {
				str.push_back(n);
			}
		}

		fd->erase_cp();			// Erase the checkpoint
		return ret(new Tret <std::string> (str));
	}
};

// C++ character and string
template <>
struct rule <char> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (n == EOF)
			return nullptr;

		return ret(new Tret <char> (n));
	}
};

// String: characters with no space
template <>
struct rule <std::string> {
	static ret value(Feeder *fd) {
		char n = fd->next();
		if (!isspace(n) && n != EOF) {
			std::string str;

			str += n;
			while (true) {
				n = fd->next();
				if (isspace(n) || n == EOF) {
					fd->noef(n);
					break;
				} else {
					str += n;
				}
			}

			return ret(new Tret <std::string> (str));
		}

		return fd->abort();
	}
};

// Word, same as string
template <>
struct rule <word> {
	static ret value(Feeder *fd) {
		return rule <std::string> ::value(fd);
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
					fd->noef(n);
					break;
				}
			}

			return ret(new Tret <std::string> (str));
		}

		return fd->noef(n);
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
struct rule <alnum> : public multiplex <digit, alpha> {};

// Macro for generating rules for special characters
#define special_lit(name, c)						\
	template <> struct rule <name> : public rule <lit <c>> {};

special_lit(dot, '.');
special_lit(comma, ',');
special_lit(equals, '=');

// Reading numbers
inline unsigned long long int atoi(Feeder *fd)
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

	fd->noef(c);
	return x;
}

inline long double atof(Feeder *fd)
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
			if (fd->getc() == EOF)		\
				return nullptr;		\
			return aux <type> (fd);		\
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

namespace parser {

// Holds lexing information for a single lexical token
template <class T>
struct token {
	static constexpr int id = -1;
	static constexpr const char *regex = "";
	static constexpr bool overloaded = false;

#ifdef NABU_DEBUG_PARSER

	static constexpr const char *name = "?";

#endif

	// Returns type of cast
	using cast_type = int;

	static cast_type cast(const std::string &s) {
		return 0;
	}
};

#ifdef NABU_DEBUG_PARSER

// Token with a regex (dummy)
#define mk_id(T, value)							\
	template <>							\
	struct token <T> {						\
		static constexpr int id = value;			\
		static constexpr const char *regex = "";		\
		static constexpr bool overloaded = false;		\
		static constexpr const char *name = #T;			\
									\
		using cast_type = int;					\
									\
		static cast_type cast(const std::string &s) {		\
			return 0;					\
		}							\
	};

#else

// Token with a regex (dummy)
#define mk_id(T, value)							\
	template <>							\
	struct token <T> {						\
		static constexpr int id = value;			\
		static constexpr const char *regex = "";		\
		static constexpr bool overloaded = false;		\
									\
		using cast_type = int;					\
									\
		static cast_type cast(const std::string &s) {		\
			return 0;					\
		}							\
	};

#endif

#define auto_mk_id(T)							\
	COUNTER_INC(int)						\
	mk_id(T, COUNTER_READ(int))

// Some explicit specialization
template <>
struct token <void> {
	static constexpr int id = 0;
	static constexpr const char *regex = "";
	static constexpr bool overloaded = false;

#ifdef NABU_DEBUG_PARSER

	static constexpr const char *name = "void";

#endif

	using cast_type = int;

	static cast_type cast(const std::string &s) {
		return 0;
	}
};

#ifdef NABU_DEBUG_PARSER

#define mk_token(T, value, str)						\
	template <>							\
	struct nabu::parser::token <T> {				\
		static constexpr int id = value;			\
		static constexpr const char *regex = str;		\
		static constexpr bool overloaded = false;		\
		static constexpr const char *name = #T;			\
									\
		using cast_type = int;					\
									\
		static cast_type cast(const std::string &s) {		\
			return 0;					\
		}							\
	};								\
									\
	template <>							\
	struct nabu::_type_name <T> {					\
		static std::string name() {				\
			return #T;					\
		}							\
	};

#else

#define mk_token(T, value, str)						\
	template <>							\
	struct nabu::parser::token <T> {				\
		static constexpr int id = value;			\
		static constexpr const char *regex = str;		\
		static constexpr bool overloaded = false;		\
									\
		using cast_type = int;					\
									\
		static cast_type cast(const std::string &s) {		\
			return 0;					\
		}							\
	};

#endif

#define auto_mk_token(T, regex) 		\
	COUNTER_INC(int);			\
	mk_token(T, COUNTER_READ(int), regex)

#ifdef NABU_DEBUG_PARSER

#define mk_overloaded_token(T, value, str, R, ftn)			\
	template <>							\
	struct nabu::parser::token <T> {				\
		static constexpr int id = value;			\
		static constexpr const char *regex = str;		\
		static constexpr const char *name = #T;			\
									\
		static constexpr bool overloaded = true;		\
									\
		using cast_type = R;					\
									\
		static cast_type cast(const std::string &s) {		\
			return ftn(s);					\
		}							\
	};								\
									\
	template <>							\
	struct nabu::_type_name <T> {					\
		static std::string name() {				\
			return #T;					\
		}							\
	};

#else

#define mk_overloaded_token(T, value, str, R, ftn)			\
	template <>							\
	struct nabu::parser::token <T> {				\
		static constexpr int id = value;			\
		static constexpr const char *regex = str;		\
									\
		static constexpr bool overloaded = true;		\
									\
		using cast_type = R;					\
									\
		static cast_type cast(const std::string &s) {		\
			return ftn(s);					\
		}							\
	};

#endif

#define auto_mk_overloaded_token(T, regex, R, ftn) 	\
	COUNTER_INC(int);				\
	mk_overloaded_token(T, COUNTER_READ(int), regex, R, ftn)


// #define set_nid(T) to set id to incremented value

// TODO: docs -> user should not need to this
struct _lexvalue {
	int id = -1;
	int line = -1;
	int col = -1;

#ifdef NABU_DEBUG_PARSER

	const char *name;
	
	_lexvalue(int v, const char *n) : id(v), name(n) {}
	_lexvalue(int v, const char *n, int l, int c)
			: id(v), name(n), line(l), col(c) {}

#else

	_lexvalue(int v) : id(v) {}
	_lexvalue(int v, int l, int c) : id(v), line(l), col(c) {}

#endif

	// Since this is a base class
	virtual ~_lexvalue() {}

	// Convert to string
	virtual std::string str() const {

#ifdef NABU_DEBUG_PARSER

		return "(name: " + std::string(name)
			+ ", line: " + std::to_string(line)
			+ ", col: " + std::to_string(col) + ")";

#else

		return "(id: " + std::to_string(id)
			+ ", line: " + std::to_string(line)
			+ ", col: " + std::to_string(col) + ")";

#endif

	}
};

// Lexicon value, defaults to empty (except ID)
template <class T>
struct lexvalue : public _lexvalue {
	T value;

#ifdef NABU_DEBUG_PARSER

	lexvalue(T a, int b, const char *n)  : _lexvalue(b, n), value(a) {}
	lexvalue(T a, int b, const char *n,  int l, int c)
			: _lexvalue(b, n, l, c), value(a) {}

#else

	lexvalue(T a, int b) : _lexvalue(b), value(a) {}
	lexvalue(T a, int b, int l, int c) : _lexvalue(b, l, c), value(a) {}

#endif

	// convert to string
	std::string str() const override {

#ifdef NABU_VERBOSE_OUTPUT

#ifdef NABU_DEBUG_PARSER
		
		return "(name: " + std::string(this->name)
			+ ", line: " + convert_string(this->line)
			+ ", col: " + convert_string(this->col)
			+ ", value: " + convert_string(value) + ")";

#else

		return "(id: " + convert_string(this->id)
			+ ", line: " + convert_string(this->line)
			+ ", col: " + convert_string(this->col)
			+ ", value: " + convert_string(value) + ")";

#endif

#else

		return convert_string(value);

#endif

	}
};

// Specialize str() for string
template <>
struct lexvalue <std::string> : public _lexvalue {
	std::string value;

#ifdef NABU_DEBUG_PARSER

	lexvalue(const std::string &a, int b, const char *n)  : _lexvalue(b, n), value(a) {}
	lexvalue(const std::string &a, int b, const char *n,  int l, int c)
			: _lexvalue(b, n, l, c), value(a) {}

#else

	lexvalue(const std::string &a, int b) : _lexvalue(b), value(a) {}
	lexvalue(const std::string &a, int b, int l, int c) : _lexvalue(b, l, c), value(a) {}

#endif

	// convert to string
	std::string str() const override {

#ifdef NABU_VERBOSE_OUTPUT

#ifdef NABU_DEBUG_PARSER

		return "(name: " + std::string(this->name)
			+ ", line: " + convert_string(this->line)
			+ ", col: " + convert_string(this->col)
			+ ", value: \"" + value + "\")";

#else

		return "(id: " + convert_string(this->id)
			+ ", line: " + convert_string(this->line)
			+ ", col: " + convert_string(this->col)
			+ ", value: \"" + value + "\")";

#endif

#else

		return "\"" + value + "\"";

#endif

	}
};

// Shared pointer alias
using lexicon = std::shared_ptr <_lexvalue>;

// Vector of lexvalues
using vec = std::vector <lexicon>;
using lexvec = lexvalue <vec>;

// Predefined lexids
mk_id(std::vector <lexicon>, 1);

// Queue of lexicons (for parsers)
using Queue = std::deque <lexicon>;

// Overload get for lexicons
template <class T>
T get(lexicon lptr)
{
	// TODO: same NABU_NO_RTTI dilemma here
	return ((lexvalue <T> *) lptr.get())->value;
}

// Cast to vector
inline std::vector <lexicon> tovec(lexicon lptr)
{
	return ((lexvec *) lptr.get())->value;
}

// Lexicon factory
template <class T>
inline lexicon make(const T &x)
{

#ifdef NABU_DEBUG_PARSER

		lexicon lptr(new lexvalue <T> (
			x, 
			token <decltype(x)> ::id,
			token <decltype(x)> ::name
		));

#else

		lexicon lptr(new lexvalue <T> (
			x, token <decltype(x)> ::id
		));

#endif

	return lptr;
}

// Read a an expected token
template <class E>
bool expect(Queue &q)
{
	if (q.empty())
		return false;

	lexicon lptr = q.front();
	if (lptr->id != token <E> ::id)
		return false;

	q.pop_front();
	return true;
}

// Get element from queue if it matches
template <class E, class T>
bool expect(Queue &q, T &value)
{
	if (q.empty())
		return false;

	lexicon lptr = q.front();
	if (lptr->id == token <E> ::id) {
		value = get <T> (lptr);
		q.pop_front();
		return true;
	}

	return false;
}

// Defines the sequence of lexical rules
template <class T>
struct lexlist {
	static constexpr bool tail = true;

	using next = void;
};

#define lexlist_next(A, B) 				\
	template <>					\
	struct nabu::parser::lexlist <A> {		\
		static constexpr bool tail = false;	\
		using next = B;				\
	};

// Defines which lexicons should be ignored
template <class T>
struct ignore {
	static constexpr bool value = false;
};

#define ignore(T)					\
	template <>					\
	struct nabu::parser::ignore <T> {		\
		static constexpr bool value = true;	\
	};

// Get Nth regex from lexlist
template <class T, int N>
struct get_regex {
	static constexpr int id = get_regex <typename lexlist <T> ::next, N - 1> ::id;
};

template <class T>
struct get_regex <T, 0> {
	static constexpr const char *regex = token <T> ::regex;
};

// Concatenate regex for a set of lexical rules (lexlist)
template <class T>
std::string concat()
{
	std::string result = "(" + std::string(token <T> ::regex) + ")";
	if (!lexlist <T> ::tail)
		result += "|" + concat <typename lexlist <T> ::next> ();
	return result;
}

// Handle regex compilation errors
template <class T>
void regex_error_handle()
{
	try {
		std::regex r(token <T> ::regex);

		if (!lexlist <T> ::tail)
			regex_error_handle <typename lexlist <T> ::next> ();
	} catch (std::regex_error &e) {
		std::string id = "(id: " + std::to_string(token <T> ::id) + ")";

#ifdef NABU_DEBUG_PARSER

		id = "(name: " + std::string(token <T> ::name) + ")";

#endif

		std::cerr << "[nabu] Error compiling regex " << id << ":\n";
		std::cerr << "\t" << e.what() << std::endl;

		std::string gen = "(" + std::string(token <T> ::regex) + ")";
		std::cerr << "\tgenerated regex: \"" << gen << "\"" << std::endl;

		exit(1);
	}
}

#ifdef NABU_DEBUG_PARSER

#define ID_TYPE const char *
#define ID_NULL nullptr

#else

#define ID_TYPE int
#define ID_NULL -1

#endif

// Check for cyclic list
template <class T>
ID_TYPE is_cyclic(std::set <int> &ids)
{
	int id = token <T> ::id;

#ifdef NABU_DEBUG_PARSER
	
	if (ids.find(id) != ids.end())
		return token <T> ::name;

	if (lexlist <T> ::tail)
		return nullptr;

#else

	if (ids.find(id) != ids.end())
		return id;

	if (lexlist <T> ::tail)
		return -1;

#endif

	ids.insert(id);
	return is_cyclic <typename lexlist <T> ::next> (ids);
}

// Get length of lexlist
template <class T>
constexpr int _length()
{
	if (lexlist <T> ::tail)
		return 1;

	return 1 + _length <typename lexlist <T> ::next> ();
}

// Compile the regex for a set of lexical rules
template <class Head>
inline std::regex compile()
{
	// First make sure the list is not cyclic
	std::set <int> ids;

	ID_TYPE id = is_cyclic <Head> (ids);
	if (id != ID_NULL) {

#ifdef NABU_DEBUG_PARSER

		std::string id_str = id;

#else

		std::string id_str = std::to_string(id);

#endif

		std::cerr << "[nabu] Error: lexical rule list is cyclic at (id: "
			<< id_str << ")" << std::endl;
		exit(1);
	}

	try {
		// TODO: an option to choose between different regex engines
		// either macro or struct option
		return std::regex(
			concat <Head> (),
			std::regex::ECMAScript | std::regex::optimize
		);
	} catch (std::regex_error &e) {
		regex_error_handle <Head> ();
	}

	return std::regex {};
}

// Create the line and column table for a string
using line_table = std::vector <std::pair <int, int>>;
using line_list = std::vector <std::string>;

inline line_table line_column(const std::string &str)
{
	line_table ret;

	int line = 1;
	int column = 1;

	for (int i = 0; i < str.size(); i++) {
		ret.push_back(std::make_pair(line, column));

		if (str[i] == '\n') {
			line++;
			column = 1;
		} else {
			column++;
		}
	}

	return ret;
}

inline line_list split_lines(const std::string &str)
{
	line_list ret;

	std::string line;
	
	std::string tmp = str + "\n";
	for (int i = 0; i < tmp.size(); i++) {
		if (tmp[i] == '\n') {
			ret.push_back(line);
			line.clear();
		} else {
			line += tmp[i];
		}
	}

	return ret;
}

// Convert a matched token to its token value
template <class Head>
parser::lexicon match(std::sregex_iterator &it, const line_table &ltbl, bool &ignored, int index = 0)
{
	// Alias to keep things clean
	using Node = token <Head>;
	using Next = typename lexlist <Head> ::next;

	// If the next one is not empty, we have reached
	if (it->str(index + 1).size() > 0) {
		size_t sindex = it->position(index + 1);

		size_t line = ltbl[sindex].first;
		size_t col = ltbl[sindex].second;

		if (ignore <Head> ::value)
			ignored = true;

		if (Node::overloaded) {

#ifdef NABU_DEBUG_PARSER

			return parser::lexicon(new parser::lexvalue
				<typename Node::cast_type> (
					Node::cast(it->str(index + 1)),
					Node::id, Node::name, line, col
				)
			);

#else

			return parser::lexicon(new parser::lexvalue
				<typename Node::cast_type> (
					Node::cast(it->str(index + 1)),
					Node::id, line, col
				)
			);

#endif

		} else {

#ifdef NABU_DEBUG_PARSER

			return parser::lexicon(
				new parser::lexvalue <std::string> (
					it->str(index + 1),
					Node::id, Node::name, line, col
				)
			);

#else

			return parser::lexicon(
				new parser::_lexvalue(
					Node::id, line, col
				)
			);

#endif

		}
	}

	// Walk the list of tokens (if any left)
	if (!lexlist <Head> ::tail)
		return match <Next> (it, ltbl, ignored, index + 1);

	return nullptr;
}

// Default error for lexing
[[noreturn]]
inline void error(const std::string &str, const line_list &lines, int line, int col)
{
	std::string line_str = lines[line - 1];
	// std::cout << "[nabu] Error: \"" << line_str << "\"" << std::endl;
	printf("%s[nabu-lexq]%s error: read bad lexicon \"%s\" at line %d, column %d\n",
		NABU_ERROR_COLOR, NABU_RESET_COLOR,
		str.c_str(), line, col);
	printf(" %5d | %s%s%s\n", line,
		NABU_ERROR_COLOR, line_str.c_str(),
		NABU_RESET_COLOR);
	std::string space(col - 1, ' ');
	std::string squiggle(line_str.size() - col, '~');
	printf(" %5c | %s%s^%s%s\n", ' ',
		NABU_ERROR_COLOR, space.c_str(),
		squiggle.c_str(), NABU_RESET_COLOR);
	exit(1);
}

// Split a string without spaces
inline std::vector <std::string> split(const std::string &str)
{
	std::vector <std::string> ret;

	std::string curr;
	std::string content = str + '\n';
	for (int i = 0; i < content.length(); i++) {
		char c = content[i];
		if (isspace(c)) {
			if (curr.length() > 0) {
				ret.push_back(curr);
				curr.clear();
			}
		} else {
			curr += c;
		}
	}

	return ret;
}

// Lexer error handler
// TODO: docs -> specialize to overload the handling
template <class Head>
void lerror_handler(const std::string &err, const line_list &lines, int line, int col)
{
	error(err, lines, line, col);
}

// Lexes a string and returns a queue of tokens
template <class Head, bool ignore_error = false>
Queue lexq(const std::string &source)
{
	std::regex re = compile <Head> ();

#ifdef NABU_DEBUG_PARSER
	
	printf("%s[nabu-lexq]%s successfully compiled regex: \"%s\"\n",
		NABU_OK_COLOR, NABU_RESET_COLOR,
		concat <Head> ().c_str());

	// The idea is to use the regex on a string
	// 	with all possible characters, so that
	// 	at least one of them will match
	std::string _str = "";
	for (int c = 32; c < 255; c++) {
		if (isprint(c))
			_str += (char) c;
	}

	// Get number of match groups
	auto b = std::sregex_iterator(_str.begin(), _str.end(), re);
	auto e = std::sregex_iterator();
	if (b->size() != _length <Head> () + 1) {
		printf("%s[nabu-lexq]%s warning: Regex has an excess capture group."
			" Please remove any capture groups in lexicon"
			" regex to receive expected results.\n",
			NABU_WARNING_COLOR, NABU_RESET_COLOR);
	}

#endif

	std::sregex_iterator begin(source.begin(), source.end(), re);
	std::sregex_iterator end;

	line_table ltbl = line_column(source);
	line_list lines = split_lines(source);

	// Store previous index
	int prev = -1;

	Queue q;
	for (auto it = begin; it != end; it++) {
		int pos = it->position();
		int len = it->length();

		if (pos > prev + 1 && !ignore_error) {
			// Make sure prev is a valid index
			prev = std::max(0, prev);
			std::string s = source.substr(prev, pos - prev);
			std::vector <std::string> sp = split(s);

			if (sp.size() > 0) {
				/*std::cout << "Error [1]: " << sp[0] << std::endl;
				for (int i = 0; i < prev + 1; i++) {
					int line = ltbl[i].first;
					int col = ltbl[i].second;
					std::cout << "char: " << source[i] << " line: " << line << " col: " << col << std::endl;
				} */

				lerror_handler <Head> (sp[0], lines,
					ltbl[prev].first, ltbl[prev].second + 1);
			}
		}

		bool ignored = false;
		parser::lexicon lptr = match <Head> (it, ltbl, ignored);

		if (lptr == nullptr && !ignore_error && !ignored) {
			std::string s = source.substr(pos, len);
			std::vector <std::string> sp = split(s);

			int line = ltbl[pos].first;
			int col = ltbl[pos].second;

			if (sp.size() > 0) {
				std::cout << "Error [2]: " << sp[0] << std::endl;
				lerror_handler <Head> (sp[0], lines, line, col);
			}
			
			std::cout << "Error [3]: " << s << std::endl;
			std::string c(1, source[pos]);
			lerror_handler <Head> ({c}, lines, line, col);
		}

		if (!ignored)
			q.push_back(lptr);

		// Update the previous position
		prev = pos + len;
	}

	return q;
}

// Parser using recursive descent
namespace rd {

#ifdef NABU_DEBUG_PARSER

struct _debugger {
	static constexpr int _indent = 4;
	static constexpr int _max_type_len = 30;

	static int &depth(int i = 0) {
		static int d = 0;
		static int prev = 0;

		prev = d;
		if (i != 0 && (d + i) >= 0)
			d += i;

		return prev;
	}

	static std::string indent(bool nest, int i = 0) {
		static std::string langle = "\u2514";
		static std::string line = "\u2500";

		if (depth() < 1)
			return std::string(depth(i) * _indent, ' ');

		if (nest) {
			std::string s = std::string((depth(i) - 1) * _indent, ' ');
			s += langle;
			for (int i = 0; i < _indent - 1; i++)
				s += line;
			return NABU_WARNING_COLOR + s + NABU_RESET_COLOR;
		}

		return std::string(depth(i) * _indent, ' ');
	}

	template <class ... Args>
	static std::string type() {
		std::string ret = type_name <Args...> ();
		int len = ret.length();
		if (len > _max_type_len)
			ret = ret.substr(0, _max_type_len - 4) + "...>";

		return ret;
	}

	template <class ... Args>
	static void log_exec() {
		printf("%s%s[nabu-rd]%s executing grammar action for %s%s%s\n",
			indent(false).c_str(),
			NABU_NOTE_COLOR, NABU_RESET_COLOR,
			NABU_ITALIC_COLOR NABU_BOLD_COLOR,
			type <Args...> ().c_str(),
			NABU_RESET_COLOR
		);
	}

	template <class ... Args>
	static void log_grammar() {
		printf("%s%s[nabu-rd]%s attempting grammar for %s%s%s {\n",
			indent(true, 1).c_str(),
			NABU_WARNING_COLOR, NABU_RESET_COLOR,
			NABU_ITALIC_COLOR NABU_BOLD_COLOR,
			type <Args...> ().c_str(),
			NABU_RESET_COLOR
		);
	}

	template <class ... Args>
	static void log_grammar_success(lexicon lptr) {
		std::string i1 = indent(false, -1);
		std::string i2 = indent(false);
		printf("%s%s[nabu-rd]%s success, value = %s\n%s} %s%s%s\n\n",
			i1.c_str(),
			NABU_OK_COLOR, NABU_RESET_COLOR,
			lptr ? lptr->str().c_str() : "nullptr",
			i2.c_str(),
			NABU_ITALIC_COLOR NABU_BOLD_COLOR,
			type <Args...> ().c_str(),
			NABU_RESET_COLOR
		);
	}

	template <class ... Args>
	static void log_grammar_failure(lexicon lptr) {
		std::string i1 = indent(false, -1);
		std::string i2 = indent(false);
		printf("%s%s[nabu-rd]%s failure, value = %s\n%s} %s%s%s\n\n",
			i1.c_str(),
			NABU_ERROR_COLOR, NABU_RESET_COLOR,
			lptr ? lptr->str().c_str() : "nullptr",
			i2.c_str(),
			NABU_ITALIC_COLOR NABU_BOLD_COLOR,
			type <Args...> ().c_str(),
			NABU_RESET_COLOR
		);
	}
};

#define log_exec(...) _debugger::log_exec <__VA_ARGS__> ();
#define log_grammar(...) _debugger::log_grammar <__VA_ARGS__> ();

#define log_grammar_end_success(lptr, ...) _debugger::log_grammar_success <__VA_ARGS__> (lptr);
#define log_grammar_end_failure(lptr, ...) _debugger::log_grammar_failure <__VA_ARGS__> (lptr);

#else

#define log_exec(...)
#define log_grammar(...)

#define log_grammar_end_success(lptr, ...)
#define log_grammar_end_failure(lptr, ...)

#endif

// Dual queue system for parsing and restoring on failure
class DualQueue {
	Queue &q;
	Queue r;
public:
	DualQueue(Queue &q_) : q(q_) {}

	// No copy constructor
	DualQueue(const DualQueue &) = delete;
	DualQueue &operator=(const DualQueue &) = delete;

	lexicon front() {
		return q.front();
	}

	void pop() {
		if (q.size() > 0) {
			lexicon lptr = q.front();
			q.pop_front();
			r.push_back(lptr);
		}

		// TODO: should throw instead?
	}

	void restore() {
		while (r.size() > 0) {
			q.push_front(r.back());
			r.pop_back();
		}
	}

	bool empty() {
		return q.empty();
	}
};

// Grammar actions
// 	performed when a grammar is matched
//
// TODO: plain function
template <class ... Args>
struct grammar_action {
	static void action(DualQueue &, const lexicon &) {}
};

// Nested grammar groups
template <class T, class ... Args>
struct alias {};

// Alternative grammar groups
template <class T, class ... Args>
struct option {
	// Map of lexicon to option index
	static std::map <lexicon, int> &map() {
		static std::map <lexicon, int> m;
		return m;
	}
};

// Repetition grammar groups
template <class T, int N = -1>
struct repeat {};

// TODO: clean up from here...
template <class T, class ... Args>
struct execute;

template <class U, class ... V>
static void exec_step(DualQueue &dq, const vec &v, int i) {
	execute <U> ::exec(dq, v.at(i));

	if constexpr (sizeof...(V) > 0) {
		// Execute nested grammar actions
		exec_step <V...> (dq, v, i + 1);
	}
}

// Execute grammar actions
// 	Executes each single grammar action
// 	before executing the overall grammar action
//
// 	i.e. grammar_action <A, B, C> will execute
// 		A::action()
// 		B::action()
// 		C::action()
// 		<A, B, C>::action()
//
// 	In particular note that subexpressions like
// 	<A, B> and <B, C> will not be executed
template <class T, class ... Args>
struct execute {
	static void exec(DualQueue &dq, const lexicon &lptr) {
		if constexpr (sizeof...(Args) == 0) {
			// Single grammar action
			log_exec(T);
			grammar_action <T> ::action(dq, lptr);
		} else {
			// Execute nested grammar actions
			vec v = tovec(lptr);
			exec_step <T, Args...> (dq, v, 0);

			log_exec(T, Args...);
			grammar_action <T, Args...> ::action(dq, lptr);
		}
	}
};

// Speciliaze for alias
template <class T, class ... Args>
struct execute <alias <T, Args...>> {
	static void exec(DualQueue &dq, const lexicon &lptr) {
		if constexpr (sizeof...(Args) == 0) {
			// Single lexicon, no need to expand
			execute <T> ::exec(dq, lptr);

			// Overall grammar action
			log_exec(alias <T>);
			grammar_action <alias <T>> ::action(dq, lptr);
		} else {
			// Expand lexicon
			vec v = tovec(lptr);
			exec_step <T, Args...> (dq, v, 0);

			// Overall grammar action
			log_exec(alias <T, Args...>);
			grammar_action <alias <T, Args...>> ::action(dq, lptr);
		}
	}
};

// Speciliaze for option
template <class T, class ... Args>
struct execute <option <T, Args...>> {
	static void do_option(DualQueue &dq, const lexicon &lptr, int i) {
		if (i == 0)
			return execute <T> ::exec(dq, lptr);

		if constexpr (sizeof...(Args) > 0)
			execute <option <Args...>> ::do_option(dq, lptr, i - 1);
	}

	static void exec(DualQueue &dq, const lexicon &lptr) {
		// Never nested, so no need to expand lexicon
		int i = option <T, Args...> ::map().at(lptr);

		// Execute single grammar action
		do_option(dq, lptr, i);

		// Overall grammar action
		log_exec(option <T, Args...>);
		grammar_action <option <T, Args...>> ::action(dq, lptr);
	}
};

// Speciliaze for repeat
template <class T, int N>
struct execute <repeat <T, N>> {
	static void exec(DualQueue &dq, const lexicon &lptr) {
		// Always nested, so expand lexicon
		vec v = tovec(lptr);

		assert(N < 0 || v.size() == N);
		for (int i = 0; i < v.size(); i++)
			execute <T> ::exec(dq, v.at(i));

		// Overall grammar action
		log_exec(repeat <T, N>);
		grammar_action <repeat <T, N>> ::action(dq, lptr);
	}
};

// Recursion for grammar
template <class T, class ... Args>
struct grammar {
	template <class U, class ... V>
	static bool _process(DualQueue &dq, vec &v) {
		lexicon lptr = grammar <U> ::value(dq, false);
		if (lptr) {
			if constexpr (sizeof...(V) > 0) {
				if (!_process <V...> (dq, v))
					return false;
			}

			v.insert(v.begin(), lptr);
			return true;
		}

		return false;
	}

	static lexicon value(DualQueue &dq, bool exec = true) {
		if constexpr (sizeof...(Args) == 0) {
			// Single grammar
			if (dq.empty())
				return nullptr;

			log_grammar(T);
			lexicon lptr = dq.front();
			if (!lptr || lptr->id != token <T> ::id) {
				log_grammar_end_failure(lptr, T);
				return nullptr;
			}

			log_grammar_end_success(lptr, T);
			if (exec)
				execute <T> ::exec(dq, lptr);

			dq.pop();
			return lptr;
		}

		// Multiple arguments
		log_grammar(T, Args...);

		vec v;
		if (_process <T, Args...> (dq, v)) {
			lexicon lptr = make(v);
			log_grammar_end_success(lptr, T, Args...);
			if (exec)
				execute <T, Args...> ::exec(dq, lptr);

			return lptr;
		}

		if (exec)
			dq.restore();

		log_grammar_end_failure(nullptr, T, Args...);
		return nullptr;
	}
};

// Void grammar as epsilon (no action)
template <>
struct grammar <void> {
	static lexicon value(DualQueue &dq, bool exec = true) {
		log_grammar(void);
		log_grammar_end_success(nullptr, void);
		lexicon lptr = make(-1);
		if (exec)
			execute <void> ::exec(dq, lptr);
		return lptr;
	}
};

// Alias grammar
template <class T, class ... Args>
struct grammar <alias <T, Args...>> {
	static lexicon value(DualQueue &dq, bool exec = true) {
		log_grammar(alias <T, Args...>);
		lexicon lptr = grammar <T, Args...> ::value(dq, false);
		if (lptr) {
			log_grammar_end_success(lptr, alias <T, Args...>);
			if (exec)
				execute <alias <T, Args...>> ::exec(dq, lptr);
			return lptr;
		}

		if (exec)
		     dq.restore();

		log_grammar_end_failure(nullptr, alias <T, Args...>);
		return lptr;
	}
};

// Option grammar
template <class T, class ... Args>
struct grammar <option <T, Args...>> {
	static int _process(DualQueue &dq, lexicon &lptr) {
		if ((lptr = grammar <T> ::value(dq, false)))
			return 0;

		if constexpr (sizeof...(Args) > 0) {
			int i = grammar <option <Args...>> ::_process(dq, lptr);
			if (i >= 0)
				return i + 1;
		}

		return -1;
	}

	static lexicon value(DualQueue &dq, bool exec = true) {
		log_grammar(option <T, Args...>);

		lexicon lptr;
		int i = _process(dq, lptr);
		if (i >= 0) {
			log_grammar_end_success(lptr, option <T, Args...>);

			option <T, Args...> ::map().insert({lptr, i});
			if (exec)
				execute <option <T, Args...>> ::exec(dq, lptr);
			return lptr;
		}

		if (exec)
			dq.restore();

		log_grammar_end_failure(nullptr, option <T, Args...>);
		return nullptr;
	}
};

// Repetition grammar
template <class T, int N>
struct grammar <repeat <T, N>> {
	static lexicon value(DualQueue &dq, bool exec = true) {
		log_grammar(repeat <T, N>);

		vec v;
		if constexpr (N < 0) {
			// Repeat until failure (always succeeds)
			while (true) {
				lexicon lptr = grammar <T> ::value(dq, false);
				if (!lptr)
					break;

				v.insert(v.begin(), lptr);
			}

			lexicon lptr = make(v);
			log_grammar_end_success(lptr, repeat <T, N>);

			if (exec)
				execute <repeat <T, N>> ::exec(dq, lptr);

			return lptr;
		}

		// Repeat N times
		for (int i = 0; i < N; i++) {
			lexicon lptr = grammar <T> ::value(dq, false);
			if (!lptr) {
				if (exec)
					dq.restore();

				log_grammar_end_failure(nullptr, repeat <T, N>);
				return nullptr;
			}

			v.insert(v.begin(), lptr);
		}

		lexicon lptr = make(v);
		log_grammar_end_success(lptr, repeat <T, N>);

		if (exec)
			execute <repeat <T, N>> ::exec(dq, lptr);

		return lptr;
	}
};

}

}

#ifdef NABU_DEBUG_GENERIC

// Type names for this namespace
template <class T, int N>
struct _type_name <parser::rd::repeat <T, N>> {
	static std::string name() {
		return "repeat <" + _type_name <T> ::name() 
			+ ", " + std::to_string(N) + ">";
	}
};

template <class ... Args>
struct _type_name <parser::rd::option <Args...>> {
	static std::string name() {
		return "option <" + _type_name <Args...> ::name() + ">";
	}
};

template <class ... Args>
struct _type_name <parser::rd::alias <Args...>> {
	static std::string name() {
		return "alias <" + _type_name <Args...> ::name() + ">";
	}
};

#endif

// Overload convert string for lexvec
inline std::string pretty_lexvec(const std::vector <parser::lexicon> &v, int indent = 0)
{
        std::string tabs(indent, '\t');
	std::string s = "{\n\t" + tabs;
	for (int i = 0; i < v.size(); i++) {
                // Check if it is a lexvec
                if (v[i]->id == parser::token <std::vector <parser::lexicon>> ::id) {
                        s += pretty_lexvec(
                                tovec(v[i]),
                                indent + 1
                        );
                } else {
                        s += v[i]->str();
                }

		if (i != v.size() - 1)
			s += ",\n\t" + tabs;
	}

	return s + "\n" + tabs + "}";
}

template <>
inline std::string convert_string <std::vector <parser::lexicon>>
		(const std::vector <parser::lexicon> &v)
{

#ifdef NABU_DEBUG_PARSER

	std::string s = "vec {";
	for (int i = 0; i < v.size(); i++) {
		s += v[i]->str();
		if (i != v.size() - 1)
			s += ", ";
	}

	return s + "}";

#else

        return pretty_lexvec(v);

#endif

}

}

// Macro for name generation
#define set_name(T, str)				\
	template <>					\
	struct nabu::rules::name <T> {			\
		static constexpr char value[] = #str;	\
	};						\
							\
	constexpr char nabu::rules::name <T> ::value[];

// TODO: figure out how to make this work, inline?
/* Set names
set_name(nabu::rules::word, word);
set_name(nabu::rules::identifier, identifier);
set_name(nabu::rules::cstr, cstr);
set_name(nabu::rules::cchar, cchar); */

// TODO: name should be inside the nabu generic namespace

// Include the counters by default
using namespace nabu::cc;

// Increement the counter to account for pre-specialized
COUNTER_INC(int);
COUNTER_INC(int);

#endif
