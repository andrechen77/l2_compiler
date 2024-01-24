#include "parser/parser.h"
#include "program/liveness.h"
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <iostream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <assert.h>
#include <optional>

void print_help(char *progName) {
	std::cerr << "Usage: " << progName << " [-v] [-g 0|1] [-O 0|1|2] [-s] [-l] [-i] [-p] SOURCE" << std::endl;
	return;
}

int main(
	int argc,
	char **argv
) {
	bool enable_code_generator = true;
	bool spill_only = false;
	bool interference_only = false;
	bool liveness_only = false;
	std::optional<std::string> parse_tree_output;
	int32_t optLevel = 3;

	/*
	 * Check the compiler arguments.
	 */
	bool verbose = false;
	if (argc < 2) {
		print_help(argv[0]);
		return 1;
	}
	int32_t opt;
	int64_t functionNumber = -1;
	while ((opt = getopt(argc, argv, "vg:O:slip:")) != -1) {
		switch (opt) {
			case 'l':
				liveness_only = true;
				break;
			case 'i':
				interference_only = true;
				break;
			case 's':
				spill_only = true;
				break;
			case 'O':
				optLevel = strtoul(optarg, NULL, 0);
				break;
			case 'g':
				enable_code_generator = (strtoul(optarg, NULL, 0) == 0) ? false : true;
				break;
			case 'v':
				verbose = true;
				break;
			case 'p':
				parse_tree_output = std::string(optarg);
				break;
			default:
				print_help(argv[0]);
				return 1;
		}
	}

	/*
	 * Parse the input file.
	 */
	std::unique_ptr<L2::program::Program> p;
	if (spill_only) {
		// Parse an L2 function and the spill arguments.
		p = L2::parser::parse_spill_file(argv[optind]);
	} else if (liveness_only) {
		// Parse an L2 function.
		std::unique_ptr<L2::program::Function> f = L2::parser::parse_function_file(argv[optind]);
		std::cout << f->to_string();
		std::map<L2::program::Instruction *, L2::program::analyze::InstructionAnalysisResult> liveness_results
			= L2::program::analyze::analyze_instructions(*f);

		// print in sets
		L2::program::analyze::printDaLiveness(*f, liveness_results);
		return 0;
	} else if (interference_only){
		// Parse an L2 function.
		// p = L2::parser::parse_function_file(argv[optind]);
	} else {
		// Parse the L2 program.
		p = L2::parser::parse_file(argv[optind], parse_tree_output);
	}

	/*
	 * Special cases.
	 */
	if (spill_only) {

		/*
		 * Spill.
		 */
		//TODO

		/*
		 * Dump the L2 code.
		 */
		//TODO

		return 0;
	}

	/*
	 * Interference graph test.
	 */
	if (interference_only) {
		//TODO
		return 0;
	}

	/*
	 * Generate the target code.
	 */
	if (enable_code_generator) {
		//TODO
	}

	return 0;
}
