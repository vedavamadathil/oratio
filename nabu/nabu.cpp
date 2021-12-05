#include <iostream>
#include <fstream>

#include "rules.hpp"

using namespace std;
using namespace nabu;

// All rule tags
// TODO: put all this information into a struct
set <std::string> tags;
vector <std::string> code;

string main_rule;
string lang_name;		// TODO: what do we really this for?
string first_tag;

bool no_main_rule = false;
bool no_json = false;

// Struct prefixes
const char *prefix = "nbg_";

// Literals constants and rules
const char walrus_str[] = ":=";
const char entry_str[] = "entry";
const char noentry_str[] = "noentry";
const char source_str[] = "source";
const char rules_str[] = "rules";
const char nojson_str[] = "nojson";
const char project_str[] = "project";

// Sources
StringFeeder sf = R"(
@project mylang

@entry number

paren := '(' expression ')'
number := k8 {// nothing here...}
	| digit*
	| identifier digit+ {// also nothing here}
)";

// File writer
int nabu_out(const std::string &file)
{
	// Input and output
	StringFeeder sf = StringFeeder::from_file(file);
	ofstream fout;

	// Read the source
	ret *rptr = rule <statement_list> ::value(&sf);
	std::cout << getrv(rptr).json() << std::endl;
	// TODO: add debugging mode

	// Set main rule
	if (no_main_rule) {
		main_rule = "";
		// TODO: need to check if source has main defined
		fout.open(file + ".hpp");
	} else {
		if (main_rule.empty())
			main_rule = first_tag;
		fout.open(file + ".cpp");
	}

	// Write copyright
	fout << sources::copyright << std::endl;

	// Include nabu
	fout << "#include \"nabu.hpp\"\n\n";

	// Output all rule tags
	fout << sources::hrule_tag << std::endl;
	for (auto &tag : tags)
		fout << "struct " << tag << " {};" << endl;

	// Output all code
	fout << sources::hrule << std::endl;
	for (auto &line : code)
		fout << line << endl;
	
	// Main function
	if (!main_rule.empty()) {
		if (no_json)
			fout << format(sources::main_no_json, prefix + main_rule) << endl;
		else
			fout << format(sources::main, prefix + main_rule) << endl;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	// Check number of arguments
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file.nabu>\n", argv[0]);
		return 1;
	}

	// Check file extension
	string filename = argv[1];
	if (filename.substr(filename.size() - 5) != ".nabu") {
		fprintf(stderr, "Error: file extension must be *.nabu\n");
		return 1;
	}

	// Check that file exists
	ifstream fin(filename);
	if (!fin.is_open()) {
		fprintf(stderr, "Error: file %s does not exist\n", filename.c_str());
		return 1;
	}

	// Run
	return nabu_out(filename);
}