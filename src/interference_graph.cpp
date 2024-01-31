#include "interference_graph.h"
#include "program.h"

namespace L2::program::analyze {
	template<typename D, typename S>
	std::vector<D> &operator+=(std::vector<D> &dest, const std::vector<S> &source) {
		dest.insert(dest.end(), source.begin(), source.end());
		return dest;
	}

	// SIRR: shift instruction register restrictions
	class SirrInstVisitor : public InstructionVisitor {
		private:

		ColoringGraph<const Variable *> &target; // the graph to which the restrictions will be added
		utils::set<const Register *> non_rcx_registers;

		public:

		SirrInstVisitor(ColoringGraph<const Variable *> &target, utils::set<const Register *> non_rsp_registers) :
			target {target}, non_rcx_registers(non_rsp_registers.begin(), non_rsp_registers.end())
		{
			for (auto it = this->non_rcx_registers.begin(); it != this->non_rcx_registers.end(); ++it) {
				if ((*it)->name == "rcx") {
					this->non_rcx_registers.erase(it);
					break;
				}
			}
		}

		virtual void visit(InstructionReturn &inst) {}
		virtual void visit(InstructionCompareAssignment &inst) {}
		virtual void visit(InstructionCompareJump &inst) {}
		virtual void visit(InstructionLabel &inst) {}
		virtual void visit(InstructionGoto &inst) {}
		virtual void visit(InstructionCall &inst) {}
		virtual void visit(InstructionLeaq &inst) {}
		virtual void visit(InstructionAssignment &inst) {
			if (inst.op == AssignOperator::lshift || inst.op == AssignOperator::rshift) {
				for (const Variable *read_var : inst.source->get_vars_on_read()) {
					for (const Variable *reg : this->non_rcx_registers) {
						this->target.add_edge(read_var, reg);
					}
				}

				// If only C++ let you pass a set<subtype> as a set<supertype>...
				// this->target.add_total_bipartite(
				// 	inst.source->get_vars_on_read();,
				// 	this->non_rcx_registers
				// );
			}
		}
	};

	ColoringGraph<const Variable *> generate_interference_graph(
		const L2Function *l2_function,
		const InstructionsAnalysisResult &inst_analysis
	) {
		// TODO this will probably be more than necessary until we delete
		// spilled variables from the scope
		std::vector<const Variable *> total_vars = l2_function->agg_scope.variable_scope.get_all_items();
		utils::set<const Register *> non_rsp_registers;
		for (const Register *reg : l2_function->agg_scope.register_scope.get_all_items()) {
			if (reg->name != "rsp") {
				total_vars.push_back(reg);
				non_rsp_registers.insert(reg);
			}
		}

		ColoringGraph<const Variable *> result(total_vars);
		result.add_total_bipartite(
			(utils::set<const Variable *> &)non_rsp_registers, // I'm so sure these casts are safe... it's a set<derived> being passed into a CONST REF to set<base> ffs
			(utils::set<const Variable *> &)non_rsp_registers
		);

		SirrInstVisitor sirr_inst_visitor(result, std::move(non_rsp_registers));

		for (const auto &[inst_ptr, inst_analysis_result] : inst_analysis) {
			// add the in_set of this instruction to the graph
			result.add_clique(inst_analysis_result.in_set);

			// if this instruction has multiple successors, then also add the
			// out_set of this instruction, since the in_sets of the
			// succeeding instructions would not be enough to capture
			// all the conflicts
			if (inst_analysis_result.successors.size() > 1) {
				result.add_clique(inst_analysis_result.out_set);
			}

			// add edges between the kill and out sets
			result.add_total_bipartite(inst_analysis_result.out_set, inst_analysis_result.kill_set);

			// account for the special case where only rcx can be used as a shift argument
			inst_ptr->accept(sirr_inst_visitor);
		}
		return result;
	}
}
