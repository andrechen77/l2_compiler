#pragma once
#include "program.h"
#include "utils.h"
#include <vector>
#include <map>
#include <set>

namespace L2::program::analyze {
	struct InstructionAnalysisResult {
		std::vector<Instruction *> successors;
		// FUTURE: so many sets of strings... see comment in program.h
		utils::set<const Variable *> gen_set;
		utils::set<const Variable *> kill_set;
		utils::set<const Variable *> in_set;
		utils::set<const Variable *> out_set;
	};

	std::map<Instruction *, InstructionAnalysisResult> analyze_instructions(const L2Function &function);

	void print_liveness(const L2Function &function, std::map<Instruction *, InstructionAnalysisResult> &liveness_results);
}
