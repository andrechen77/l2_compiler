#include "liveness.h"

namespace L2::program::analyze {
	using std::map;
	using std::vector;

	// Accumulates a map<Instruction *, InstructionAnalysisResult> with only the
	// successors, gen_set, and kill_set fields filled out.
	// ASSUMES THAT YOU ITERATE THROUGH THE INSTRUCTIONS IN ORDER STARTING WITH
	// THE FIRST ONE
	class InstructionPreAnalyzer : public InstructionVisitor {
		private:

		const Function &target; // the function being analyzed
		int index;
		std::map<Instruction *, InstructionAnalysisResult> accum;

		public:

		InstructionPreAnalyzer(const Function &target):
			target {target}, index {0}
		{}

		std::map<Instruction *, InstructionAnalysisResult> get_accumulator() {
			return std::move(this->accum);
		}

		virtual void visit(InstructionReturn &inst) override {
			accum[&inst].gen_set = {"rax", "rbx", "rbp", "r12", "r13", "r14", "r15"};
			index++;
		}
		virtual void visit(InstructionAssignment &inst) override {
			accum[&inst].successors.push_back(target.instructions[index + 1].get());
			accum[&inst].kill_set = inst.source->get_write_vars();
			accum[&inst].gen_set = inst.destination->get_read_vars();
			index++;
		}
		virtual void visit(InstructionCompareAssignment &inst) override {
			accum[&inst].successors.push_back(target.instructions[index + 1].get());
			accum[&inst].kill_set = inst.destination->get_write_vars();
			accum[&inst].gen_set = inst.lhs->get_read_vars();


			const auto &temp = inst.rhs->get_read_vars();
			accum[&inst].gen_set.insert(temp.begin(), temp.end());
			++index;
		}
		virtual void visit(InstructionCompareJump &inst) override {
			accum[&inst].successors.push_back(target.instructions[index + 1].get());
			std::string target_label = ":" + inst.label->to_string();
			for(auto &i : target.instructions){
				if (i->to_string() == target_label){ // JANK
					accum[&inst].successors.push_back(i.get());
					break;
				}
			}
			accum[&inst].gen_set = inst.lhs->get_read_vars();
			accum[&inst].gen_set = inst.rhs->get_read_vars();
			++index;
		}
		virtual void visit(InstructionLabel &inst) override {
			accum[&inst].successors.push_back(target.instructions[index + 1].get());
			++index;
		}
		virtual void visit(InstructionGoto &inst) override {
			std::string target_label = ":" + inst.label->to_string();
			for (auto &i : target.instructions) {
				if (i->to_string() == target_label){ // JANK
					accum[&inst].successors.push_back(i.get());
					break;
				}
			}
			++index;
		}
		virtual void visit(InstructionCall &inst) override {
			accum[&inst].gen_set = inst.callee->get_read_vars();
  			std::string type_call = inst.callee->to_string();
			static std::set<std::string> nosucC = {"tuple-error", "tensor-error"};
			if (nosucC.find(type_call) == nosucC.end()){
				accum[&inst].successors.push_back(target.instructions[index + 1].get());
			}
			++index;
		}
		virtual void visit(InstructionLeaq &inst) override {
			accum[&inst].successors.push_back(target.instructions[index + 1].get());
			accum[&inst].kill_set = inst.destination->get_write_vars();
			accum[&inst].gen_set = inst.base->get_read_vars();
			accum[&inst].gen_set = inst.offset->get_read_vars();
			index++;
		}
	};

	std::map<Instruction *, InstructionAnalysisResult> analyze_instructions(const Function &function) {
		auto num_instructions = function.instructions.size();

		InstructionPreAnalyzer pre_analyzer(function);
		for (int i = 0; i < num_instructions; ++i) {
			function.instructions[i]->accept(pre_analyzer);
		}
		// "resol" is a compromise between the authors' preferred accumulator variables "result" and "sol"
		std::map<Instruction *, InstructionAnalysisResult> resol = pre_analyzer.get_accumulator();

		// Each instruction starts with only its gen set as its in set.
		// This initially satisfies the in set's constraints.
		for (const std::unique_ptr<Instruction> &inst : function.instructions) {
			InstructionAnalysisResult &entry = resol[inst.get()];
			entry.out_set = entry.gen_set;
		}
		bool sets_changed;
		do {
			sets_changed = false;
			for (int i = num_instructions - 1; i >= 0; --i) {
				InstructionAnalysisResult &entry = resol[function.instructions[i].get()];

				// out[i] = UNION (s in successors(i)) {in[s]}
				std::set<std::string> new_out_set;
				for (Instruction *succ : entry.successors) {
					for (const std::string &var : resol[succ].in_set) {
						new_out_set.insert(var);
					}
				}
				if (entry.out_set != new_out_set) {
					sets_changed = true;
					entry.out_set = std::move(new_out_set);
				}

				// in[i] = gen[i] UNION (out[i] MINUS kill[i])
				// Everything currently in in[i] is either there because it was
				// in gen[i] or because it's in out[i]. out[i] is the only thing
				// that might have changed. Assuming that this equation is
				// upheld in the previous iteration, these operations will
				// satisfy the formula again.
				// - remove all existing elements that are not in out[i] nor gen[i]
				// - add all elements that are in out[i] but not kill[i]
				std::set<std::string> new_in_set;
				for (const std::string &var : entry.in_set) {
					if (entry.out_set.find(var) != entry.out_set.end()
						|| entry.gen_set.find(var) != entry.gen_set.end())
					{
						new_in_set.insert(var);
					}
				}
				for (const std::string &var : entry.out_set) {
					if (entry.kill_set.find(var) == entry.kill_set.end()) {
						new_in_set.insert(var);
					}
				}
				if (entry.in_set == new_in_set) {
					sets_changed = true;
					entry.in_set = std::move(new_in_set);
				}
			}
		} while (sets_changed);

		return resol;
	}
}