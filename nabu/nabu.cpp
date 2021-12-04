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
string lang_name;
string first_tag;

bool no_main_rule = false;

// Literals constants and rules
const char walrus_str[] = ":=";
const char entry_str[] = "entry";
const char noentry_str[] = "noentry";
const char source_str[] = "source";
const char rules_str[] = "rules";
const char project_str[] = "project";

// Sources
StringFeeder sf = R"(
#project mylang

#entry number

paren := '(' expression ')'
number := k8 {body123}
	| digit*
	| identifier digit+ {body456}
)";

// File writer
int nabu_out(std::ofstream &fout)
{
	// Read the source
	rule <statement_list> ::value(&sf);

	// Set main rule
	if (no_main_rule) {
		main_rule = "";
	} else {
		if (main_rule.empty())
			main_rule = first_tag;
	}

	// Add language namespace
	if (!lang_name.empty())
		fout << "namespace " << lang_name << " {" << endl;

	// Output all rule tags
	fout << sources::hrule_tag << std::endl;
	for (auto &tag : tags)
		fout << "struct " << tag << " {};" << endl;

	// Output all code
	fout << sources::hrule << std::endl;
	for (auto &line : code)
		fout << line << endl;

	// Close language namespace
	if (!lang_name.empty())
		fout << "\n}" << endl;
	
	// Main function
	if (!main_rule.empty()) {
		// TODO: need to code args as well
		fout << format(sources::main, main_rule) << endl;
	}

	return 0;
}

int main()
{
	// Open output file
	ofstream fout("example.nabu.cpp");

	// Write output
	return nabu_out(fout);
}