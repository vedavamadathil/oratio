#include <algorithm>
#include <fstream>
#include <iostream>

#include "rules.hpp"

using namespace std;
using namespace nabu;

// State singleton
State state;

// Literals constants and rules
const char walrus_str[] = ":=";

// Macros
const char entry_str[] = "entry";
const char noentry_str[] = "noentry";
const char source_str[] = "source";
const char rules_str[] = "rules";
const char nojson_str[] = "nojson";
const char project_str[] = "project";

// Replace tabs with spaces
std::string replace_tabs(const std::string &str)
{
	std::string out;
	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == '\t')
			out += std::string(8, ' ');
		else
			out += str[i];
	}
	return out;
}

// Process unresolved symbols
void warn_unresolved(Feeder *fd)
{
	// TODO: feeder method to extract a single line
	// TODO: in the warning also show curly chars under the symbol

	// Transfer unresolved symbols to vector
	std::vector <std::pair <std::string, int>> unresolved(
		state.unresolved.begin(), state.unresolved.end()
	);

	// Sort the unresolved symbols by line number
	std::sort(unresolved.begin(), unresolved.end(),
		[](const std::pair <std::string, int> &a, const std::pair <std::string, int> &b) {
			return a.second < b.second;
		}
	);

	for (const auto &pr : unresolved) {
		// Print warning
		warn_no_col(fd->source(), pr.second, "rule \'%s\' is not defined"
			" anywhere\n", pr.first.c_str());
		
		// Print location
		std::string line = replace_tabs(fd->get_line(pr.second));

		std::cout << "\t" << line << std::endl;

		size_t pos = line.find(pr.first);
		std::cout << "\t" << std::string(pos, ' ') << BOLD << "^"
			<< std::string(pr.first.length() - 1, '~') << RESET << std::endl;
	
		// Increment warnings
		state.warnings++;
	}
}

// File writer
// TODO: separate stages into methods
std::string nabu_out(const std::string &file)
{
	// Input and output
	StringFeeder sf = StringFeeder::from_file(file);
	ofstream fout;

	// Read the source
	ret rptr = rule <statement_list> ::value(&sf);
	if (state.print_json)
		std::cout << getrv(rptr).json() << std::endl;
	
	// Generate warnings and errors
	warn_unresolved(&sf);

	// Summarize warnings and errors
	if (state.errors)
		printf("%lu error%c", state.errors, (state.errors) > 1 ? 's' : '\0');
	if (state.warnings) {
		printf("%s%lu warning%c", (state.errors ? ", and" : ""),
			state.warnings, (state.warnings) > 1 ? 's' : '\0');
	}
	if (state.errors || state.warnings)
		printf(" generated\n");

	// Set main rule
	std::string outname = file;
	if (state.no_main_rule) {
		state.main_rule = "";
		// TODO: need to check if source has main defined
		outname = file + ".hpp";
	} else {
		if (state.main_rule.empty())
			state.main_rule = state.first_tag;
		outname = file + ".cpp";
	}

	// Open output file
	fout.open(outname);

	// Write copyright
	fout << sources::copyright << std::endl;

	// Include nabu
	fout << "#include \"nabu.hpp\"\n\n";

	// Create string literals
	for (size_t i = 0; i < state.literals.size(); i++) {
		fout << "const char str_lit_" << i << "[] = \""
			<< state.literals[i] << "\";\n";
	}

	// Begin namespace
	if (!state.lang_name.empty())
		fout << "\nnamespace " << state.lang_name << " {\n";

	// Output all rule tags
	fout << sources::hrule_tag << std::endl;
	for (auto &tag : state.tags)
		fout << "struct " << tag << " {};\n";

	// End namespace
	if (!state.lang_name.empty())
		fout << "\n}\n";
	
	// Set names of the rules
	fout << sources::hrule_name << std::endl;
	for (auto &tag : state.tags) {
		fout << "set_name(" << state.lang_name << "::"
			<< tag << ", " << tag << ");\n";
	}

	// Output all code
	fout << sources::hrule << std::endl;
	for (auto &line : state.code)
		fout << line << endl;

	// Main function
	std::string main = state.lang_name + "::" + state.main_rule;
	if (!state.main_rule.empty()) {
		if (state.no_json)
			fout << format(sources::main_no_json, main) << endl;
		else
			fout << format(sources::main, main) << endl;
	}

	return outname;
}

// Argument parser
static ArgParser ap("nabu", 1, {
	ArgParser::Option(ArgParser::Args {"-j", "--json"}, "Print JSON output"),
	ArgParser::Option(ArgParser::Args {"-c", "--compile"},
		"Compile the generated code (if @noentry was not specified)")
});

int main(int argc, char *argv[])
{
	// Parse arguments
	ap.parse(argc, argv);

	// Extract argument info
	string filename = ap.get(0);
	if (ap.get_optn <bool> ("-j"))
		state.print_json = true;

	if (filename.substr(filename.size() - 5) != ".nabu")
		return ap.error("file extension must be *.nabu");

	// Check that file exists
	ifstream fin(filename);
	if (!fin.is_open())
		return ap.error("file " + filename + " does not exist");

	// Run
	std::string fout = nabu_out(filename);

	// Compile if requested
	if (ap.get_optn <bool> ("-c")) {
		std::string cmd = "g++ -std=c++11 -o "
			+ filename.substr(0, filename.size() - 5)
			+ " " + fout;
		system(cmd.c_str());

		// Print location of the executable
		printf("\nCompiled executable is located at %s\n",
			filename.substr(0, filename.size() - 5).c_str());
	}

	return 0;
}
