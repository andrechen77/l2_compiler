#pragma once
#include "program.h"
#include <vector>
#include <map>
#include <set>

namespace L2::program::analyze {
	struct InstructionAnalysisResult {
		std::vector<Instruction *> successors;
		// FUTURE: so many sets of strings... see comment in program.h
		std::set<std::string> gen_set;
		std::set<std::string> kill_set;
		std::set<std::string> in_set;
		std::set<std::string> out_set;
	};

	std::map<Instruction *, InstructionAnalysisResult> analyze_instructions(const Function &function);

	void printDaLiveness(const Function &function, std::map<Instruction *, InstructionAnalysisResult> &liveness_results);
}
