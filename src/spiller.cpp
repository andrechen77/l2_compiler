#include "spiller.h"
#include <iostream>
#include <set>
#include <string>
#include <algorithm>

namespace L2::program::spiller {

	class ExprReplaceVisitor : public ExprVisitor {
		private:
		std::string replace;
		Variable *target;
		AggregateScope &agg_scope;
		
		public:
		ExprReplaceVisitor(AggregateScope &agg_scope, std::string replace, Variable* target): 
			replace{replace},
			target {target},
			agg_scope {agg_scope}
		{}

		virtual void visit(RegisterRef &expr) {}
		virtual void visit(NumberLiteral &expr) {}
		virtual void visit(StackArg &expr) {}
		virtual void visit(MemoryLocation &expr) {
			expr.base->accept(*this);
		}
		virtual void visit(LabelRef &expr) {}
		virtual void visit(VariableRef &expr){
			if (expr.get_referent() == target){
				expr.bind(agg_scope.variable_scope.get_item_or_create(replace));
			}
		}
		virtual void visit(L2FunctionRef &expr) {}
		virtual void visit(ExternalFunctionRef &expr) {}
	};

	class InstructionSpiller : public InstructionVisitor {
		private:
		L2Function &function;
		Variable *var;
		std::string prefix;
		int prefix_count;
		int num_calls;
		int index;
		Register *rsp;

		public:
		InstructionSpiller(L2Function &function, Variable *var, std::string prefix, int num_calls):
			function {function}, 
			var {var}, 
			prefix {prefix},
			prefix_count{0}, 
			index {0},
			num_calls {num_calls}
		{
			auto maybe_rsp = this->function.agg_scope.register_scope.get_item_maybe("rsp");
			if (maybe_rsp) {
				this->rsp = *maybe_rsp;
			} else {
				std::cerr << "HOLY SHIT MY ASS IS BURNING NO REGISTER FOUND RSP\n";
				exit(-1);
			}
		}

		virtual void visit(InstructionReturn &inst) override {
			index++;
		}

		virtual void visit(InstructionAssignment &inst) override {
			// find if the source uses var
			std::set<Variable *, std::less<void>> write_dest = inst.destination->get_vars_on_write(false);
			bool write_dest_count = write_dest.count(var) > 0;
			std::set<Variable *, std::less<void>> read_source = inst.source->get_vars_on_read();
			bool read_source_count = read_source.count(var) > 0;
			std::set<Variable *, std::less<void>> read_dest = inst.source->get_vars_on_write(true);
			bool read_dest_count = read_dest.count(var) > 0;
			std::set<Variable *, std::less<void>> read_dest_update;
			if (inst.op != AssignOperator::pure) {
				// also reads from the destination
				read_dest_update = inst.destination->get_vars_on_read();
			}
			bool read_dest_update_count = read_dest_update.count(var) > 0;
			
			
			if (write_dest_count || read_source_count || read_dest_count || read_dest_update_count){
				std::string new_var_name = prefix + std::to_string(prefix_count);
				ExprReplaceVisitor v(function.agg_scope, new_var_name, var);
				inst.source->accept(v);
				inst.destination->accept(v);


				if (read_source_count || read_dest_count || read_dest_update_count){
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count)),
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							)
						)
					);
					index++;
				}
				if(write_dest_count){
					index++;
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							),
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count))
						)
					);
				}
				prefix_count++;
			}
			++index;
		}

		virtual void visit(InstructionCompareAssignment &inst) override {
			std::set<Variable *, std::less<void>> write_dest = inst.destination->get_vars_on_write(false);
			bool write_dest_count = write_dest.count(var) > 0;
			std::set<Variable *, std::less<void>> read_lhs = inst.lhs->get_vars_on_read();
			bool read_lhs_count = read_lhs.count(var) > 0;
			std::set<Variable *, std::less<void>> read_rhs = inst.rhs->get_vars_on_read();
			bool read_rhs_count = read_rhs.count(var) > 0;
			if (write_dest_count || read_lhs_count || read_rhs_count){
				std::string new_var_name = prefix + std::to_string(prefix_count);
				ExprReplaceVisitor v(function.agg_scope, new_var_name, var);
				inst.lhs->accept(v);
				inst.rhs->accept(v);
				inst.destination->accept(v);
				if (read_lhs_count || read_rhs_count){
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count)),
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							)
						)
					);
					index++;
				}
				if(write_dest_count){
					index++;
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							),
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count))
						)
					);
				}
			}
			index++;
		}

		virtual void visit(InstructionCompareJump &inst) override {
			std::set<Variable *, std::less<void>> read_lhs = inst.lhs->get_vars_on_read();
			bool read_lhs_count = read_lhs.count(var) > 0;
			std::set<Variable *, std::less<void>> read_rhs = inst.rhs->get_vars_on_read();
			bool read_rhs_count = read_rhs.count(var) > 0;
			if (read_lhs_count || read_rhs_count){
				std::string new_var_name = prefix + std::to_string(prefix_count);
				ExprReplaceVisitor v(function.agg_scope, new_var_name, var);
				inst.lhs->accept(v);
				inst.rhs->accept(v);
				if (read_lhs_count || read_rhs_count){
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count)),
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							)
						)
					);
					index++;
				}
			}
			++index;
		}

		virtual void visit(InstructionLabel &inst) override {
			index++;
		}

		virtual void visit(InstructionGoto &inst) override {
			++index;
		}

		virtual void visit(InstructionCall &inst) override {
			std::set<Variable *, std::less<void>> read_callee = inst.callee->get_vars_on_read();
			bool read_callee_count = read_callee.count(var) > 0;
			if (read_callee_count){
				std::string new_var_name = prefix + std::to_string(prefix_count);
				ExprReplaceVisitor v(function.agg_scope, new_var_name, var);
				inst.callee->accept(v);
				function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count)),
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							)
						)
					);
					index++;
			}
			index++;
		}

		virtual void visit(InstructionLeaq &inst) override {
			std::set<Variable *, std::less<void>> write_dest = inst.destination->get_vars_on_read();
			bool write_dest_count = write_dest.count(var) > 0;
			std::set<Variable *, std::less<void>> read_dest = inst.destination->get_vars_on_write(true);
			bool read_dest_count = read_dest.count(var) > 0;
			std::set<Variable *, std::less<void>> read_base = inst.base->get_vars_on_write(true);
			bool read_base_count = read_base.count(var) > 0;
			std::set<Variable *, std::less<void>> read_offset = inst.offset->get_vars_on_write(true);
			bool read_offset_count = read_offset.count(var) > 0;
			if (write_dest_count || read_dest_count || read_base_count || read_offset_count){
				std::string new_var_name = prefix + std::to_string(prefix_count);
				ExprReplaceVisitor v(function.agg_scope, new_var_name, var);
				inst.destination->accept(v);
				inst.base->accept(v);
				inst.offset->accept(v);
				if (read_dest_count || read_base_count || read_offset_count){
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count)),
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							)
						)
					);
					index++;
				}
				if(write_dest_count){
					index++;
					function.insert_instruction(
						index,
						std::make_unique<InstructionAssignment>(
							AssignOperator::pure,
							std::make_unique<MemoryLocation>(
								std::make_unique<RegisterRef>(this->rsp),
								std::make_unique<NumberLiteral>(num_calls * 8)
							),
							std::make_unique<VariableRef>(prefix + std::to_string(prefix_count))
						)
					);
				}
			}
			++index;
		}

		int get_index(){ return index; }
	};

	void spill(L2Function &function, Variable *var, std::string prefix, int spill_calls=0){
		L2Function &function_test = function;
		InstructionSpiller spiller(function_test, var, prefix, spill_calls);
		while (spiller.get_index() < function.instructions.size()){
			function.instructions[spiller.get_index()]->accept(spiller);
		}
		
	}

	void printDaSpiller(){

	}
}