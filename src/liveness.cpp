// #include "liveness.h"
// #include <string>
// #include <iostream>

// namespace L2::program::analyze {
// 	/*
// 	TODO liveness analysis shouldn't deal with std::strings, but rather pointers to
// 	Variables
// 	*/

// 	const std::set<std::string> caller_saved_registers {
// 		"rax", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
// 	};

// 	const std::vector<std::string> argument_registers {
// 		"rdi", "rsi", "rdx", "rcx", "r8", "r9"
// 	};

// 	const std::set<std::string> callee_saved_registers {
// 		"r12", "r13", "r14", "r15", "rbp", "rbx"
// 	};

// 	// Accumulates a map<Instruction *, InstructionAnalysisResult> with only the
// 	// successors, gen_set, and kill_set fields filled out.
// 	// ASSUMES THAT YOU ITERATE THROUGH THE INSTRUCTIONS IN ORDER STARTING WITH
// 	// THE FIRST ONE
// 	class InstructionPreAnalyzer : public InstructionVisitor {
// 		private:

// 		const Function &target; // the function being analyzed
// 		int index;
// 		std::map<Instruction *, InstructionAnalysisResult> accum;

// 		public:

// 		InstructionPreAnalyzer(const Function &target):
// 			target {target}, index {0}
// 		{}

// 		std::map<Instruction *, InstructionAnalysisResult> get_accumulator() {
// 			return std::move(this->accum);
// 		}

// 		virtual void visit(InstructionReturn &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];
// 			entry.gen_set = callee_saved_registers;
// 			entry.gen_set.insert("rax");
// 			index++;
// 		}
// 		virtual void visit(InstructionAssignment &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];

// 			entry.successors.push_back(target.instructions[index + 1].get());

// 			entry.kill_set = inst.destination->get_vars_on_write(false);

// 			entry.gen_set = inst.source->get_vars_on_read();
// 			entry.gen_set.merge(inst.destination->get_vars_on_write(true));
// 			if (inst.op != AssignOperator::pure) {
// 				// also reads from the destination
// 				entry.gen_set.merge(inst.destination->get_vars_on_read());
// 			}

// 			index++;
// 		}
// 		virtual void visit(InstructionCompareAssignment &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];

// 			entry.successors.push_back(target.instructions[index + 1].get());

// 			entry.kill_set = inst.destination->get_vars_on_write(false);

// 			entry.gen_set = inst.lhs->get_vars_on_read();
// 			entry.gen_set.merge(inst.rhs->get_vars_on_read());

// 			++index;
// 		}
// 		virtual void visit(InstructionCompareJump &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];

// 			entry.successors.push_back(target.instructions[index + 1].get());
// 			std::string target_label = inst.label->to_string();
// 			for (auto &i : target.instructions){
// 				if (i->to_string() == target_label){ // JANK
// 					entry.successors.push_back(i.get());
// 					break;
// 				}
// 			}

// 			entry.gen_set = inst.lhs->get_vars_on_read();
// 			entry.gen_set.merge(inst.rhs->get_vars_on_read());

// 			++index;
// 		}
// 		virtual void visit(InstructionLabel &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];
// 			entry.successors.push_back(target.instructions[index + 1].get());
// 			++index;
// 		}
// 		virtual void visit(InstructionGoto &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];
// 			std::string target_label = inst.label->to_string();
// 			for (auto &i : target.instructions) {
// 				if (i->to_string() == target_label){ // JANK
// 					entry.successors.push_back(i.get());
// 					break;
// 				}
// 			}
// 			++index;
// 		}
// 		virtual void visit(InstructionCall &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];
// 			entry.gen_set = inst.callee->get_vars_on_read();
// 			for (int i = 0; i < inst.num_arguments && i < argument_registers.size(); ++i) {
// 				entry.gen_set.insert(argument_registers[i]);
// 			}
// 			entry.kill_set = caller_saved_registers;
//   			std::string type_call = inst.callee->to_string();
// 			static std::set<std::string> nosucC = {"tuple-error", "tensor-error"};
// 			if (nosucC.find(type_call) == nosucC.end()){
// 				entry.successors.push_back(target.instructions[index + 1].get());
// 			}
// 			++index;
// 		}
// 		virtual void visit(InstructionLeaq &inst) override {
// 			InstructionAnalysisResult &entry = accum[&inst];

// 			entry.successors.push_back(target.instructions[index + 1].get());

// 			entry.kill_set = inst.destination->get_vars_on_write(false);

// 			entry.gen_set = inst.base->get_vars_on_read();
// 			entry.gen_set.merge(inst.offset->get_vars_on_read());
// 			entry.gen_set.merge(inst.destination->get_vars_on_write(true));

// 			index++;
// 		}
// 	};

// 	std::string resized(std::string s, std::size_t length = 30) {
// 		s.resize(length, ' ');
// 		return s;
// 	}

// 	// std::string andre(const std::set<std::string> &bob) {
// 	// 	std::string result =  "{";
// 	// 	for (const std::string &str : bob) {
// 	// 		result += str + ", ";
// 	// 	}
// 	// 	return result + "}";
// 	// }

// 	std::map<Instruction *, InstructionAnalysisResult> analyze_instructions(const Function &function) {
// 		auto num_instructions = function.instructions.size();

// 		InstructionPreAnalyzer pre_analyzer(function);
// 		for (int i = 0; i < num_instructions; ++i) {
// 			function.instructions[i]->accept(pre_analyzer);
// 		}
// 		// "resol" is a compromise between the authors' preferred accumulator variables "result" and "sol"
// 		std::map<Instruction *, InstructionAnalysisResult> resol = pre_analyzer.get_accumulator();

// 		// Each instruction starts with only its gen set as its in set.
// 		// This initially satisfies the in set's constraints.
// 		for (const std::unique_ptr<Instruction> &inst : function.instructions) {
// 			InstructionAnalysisResult &entry = resol[inst.get()];
// 			entry.in_set = entry.gen_set;
// 			// std::cerr << resized(inst->to_string(), 30) << "\t";
// 			// std::cerr << kevin(entry.in_set) << "\n";
// 			// for (Instruction *succ : entry.successors) {
// 			// 	std::cerr << "\t" << succ->to_string() << "\n";
// 			// }
// 		}
// 		bool sets_changed;
// 		do {

// 			sets_changed = false;
// 			for (int i = num_instructions - 1; i >= 0; --i) {
// 				// for (const auto &inst : function.instructions) {
// 				// 	InstructionAnalysisResult &entry = resol[inst.get()];
// 				// 	std::cerr << resized(inst->to_string(), 50) << " ";
// 				// 	std::cerr << resized(kevin(entry.in_set), 50) << " " << resized(kevin(entry.out_set), 50) << "\n";
// 				// }
// 				// std::string pog;
// 				// std::cin >> pog;
// 				// std::cerr << "continuing\n";

// 				InstructionAnalysisResult &entry = resol[function.instructions[i].get()];

// 				// out[i] = UNION (s in successors(i)) {in[s]}
// 				std::set<std::string> new_out_set;
// 				for (Instruction *succ : entry.successors) {
// 					for (const std::string &var : resol[succ].in_set) {
// 						new_out_set.insert(var);
// 					}
// 				}
// 				if (entry.out_set != new_out_set) {
// 					sets_changed = true;
// 					entry.out_set = std::move(new_out_set);
// 				}

// 				// in[i] = gen[i] UNION (out[i] MINUS kill[i])
// 				// Everything currently in in[i] is either there because it was
// 				// in gen[i] or because it's in out[i]. out[i] is the only thing
// 				// that might have changed. Assuming that this equation is
// 				// upheld in the previous iteration, these operations will
// 				// satisfy the formula again.
// 				// - remove all existing elements that are not in out[i] nor gen[i]
// 				// - add all elements that are in out[i] but not kill[i]
// 				std::set<std::string> new_in_set;
// 				for (const std::string &var : entry.in_set) {
// 					if (entry.out_set.find(var) != entry.out_set.end()
// 						|| entry.gen_set.find(var) != entry.gen_set.end())
// 					{
// 						new_in_set.insert(var);
// 					}
// 				}
// 				for (const std::string &var : entry.out_set) {
// 					if (entry.kill_set.find(var) == entry.kill_set.end()) {
// 						new_in_set.insert(var);
// 					}
// 				}
// 				if (entry.in_set != new_in_set) {
// 					sets_changed = true;
// 					entry.in_set = std::move(new_in_set);
// 				}
// 			}
// 		} while (sets_changed);

// 		return resol;
// 	}

// 	void printDaLiveness(const Function &function, std::map<Instruction *, InstructionAnalysisResult> &liveness_results){
// 		std::cout << "(\n(in\n";
// 		for (const auto &instruction : function.instructions) {
// 			const InstructionAnalysisResult &entry = liveness_results[instruction.get()];
// 			std::cout << "(";
// 			for (auto element : entry.in_set) {
//         		std::cout << element << " ";
//     		}
// 			std::cout << ")\n";
// 		}

// 		std::cout << ")\n\n(out\n";
// 		// print out sets
// 		for (const auto &instruction : function.instructions) {
// 			const InstructionAnalysisResult &entry = liveness_results[instruction.get()];
// 			std::cout << "(";
// 			for (auto element : entry.out_set) {
//         		std::cout << element << " ";
//     		}
// 			std::cout << ")\n";
// 		}
// 		std::cout << ")\n\n)\n";
// 	}
// }
