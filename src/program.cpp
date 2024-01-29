#include "program.h"
#include <map>
#include <utility>
#include <functional>
#include <charconv>

namespace L2::program {
	std::string_view RegisterRef::get_ref_name() const {
		if (this->referent) {
			return this->referent->name;
		} else {
			return this->free_name;
		}
	}

	std::string RegisterRef::to_string() const {
		return std::string(this->get_ref_name());
	}

	void RegisterRef::bind_all(AggregateScope &agg_scope) {
		agg_scope.register_scope.add_ref(*this);
	}

	void RegisterRef::bind(Register *referent) {
		this->referent = referent;
	}

	std::set<Variable *> RegisterRef::get_vars_on_read() const {
		// TODO if we are referring to rsp, then return nothing
		if (false) {
			return {};
		}
		return {this->referent};
	}
	std::set<Variable *> RegisterRef::get_vars_on_write(bool get_read_vars) const {
		// TODO if we are referring to rsp, then return nothing
		if (false) {
			return {};
		}
		if (get_read_vars) {
			return {};
		} else {
			return {this->referent};
		}
	}

	RegisterRef::RegisterRef(Register* referent) :
		Expr(),
		free_name {},
		referent {referent}
	{}

	RegisterRef::RegisterRef(const std::string_view &free_name) :
		Expr(),
		free_name {free_name},
		referent {nullptr}
	{}

	std::string StackArg::to_string() const {
        return "stack-arg " + this->stack_num->to_string();
    }

	std::string MemoryLocation::to_string() const {
		return "mem " + this->base->to_string() + " " + this->offset->to_string();
	}

	std::set<Variable *> MemoryLocation::get_vars_on_read() const {
		return this->base->get_vars_on_read();
	}

	std::set<Variable *> MemoryLocation::get_vars_on_write(bool get_read_vars) const {
		// the base is read even if this MemoryLocation is being written
		if (get_read_vars) {
			return this->base->get_vars_on_read();
		} else {
			return {};
		}
	}

	void MemoryLocation::bind_all(AggregateScope &agg_scope) {
		this->base->bind_all(agg_scope);
	}

	std::string NumberLiteral::to_string() const {
		return std::to_string(this->value);
	}

	LabelRef::LabelRef(const std::string_view &free_name) :
		free_name {free_name},
		referent {nullptr}
	{}

	void LabelRef::bind(InstructionLabel **referent) {
		this->referent = referent;
	}

	void LabelRef::bind_all(AggregateScope &agg_scope) {
		agg_scope.label_scope.add_ref(*this);
	}

	std::string_view LabelRef::get_ref_name() const {
		if (this->referent) {
			return (*this->referent)->label_name;
		} else {
			return this->free_name;
		}
	}

	std::string LabelRef::to_string() const {
		return ":" + std::string(this->get_ref_name());
	}

	VariableRef::VariableRef(const std::string_view &free_name) :
		free_name {free_name},
		referent {nullptr}
	{}

	VariableRef::VariableRef(Variable *referent) :
		free_name {},
		referent {referent}
	{}

	void VariableRef::bind_all(AggregateScope &agg_scope) {
		this->bind(agg_scope.variable_scope.get_item_or_create(this->get_ref_name()));
	}

	void VariableRef::bind(Variable *referent) {
		this->referent = referent;
	}

	std::string_view VariableRef::get_ref_name() const {
		if (this->referent) {
			return this->referent->name;
		} else {
			return this->free_name;
		}
	}

	std::string VariableRef::to_string() const {
		return "%" + std::string(this->get_ref_name());
	}

	std::set<Variable *> VariableRef::get_vars_on_read() const {
		return {this->referent};
	}
	std::set<Variable *> VariableRef::get_vars_on_write(bool get_read_vars) const {
		if (get_read_vars) {
			return {};
		} else {
			return {this->referent};
		}
	}

	FunctionRef::FunctionRef(const std::string_view &name, bool is_std) :
		name {name},
		is_std {is_std},
		referent {nullptr}
	{}

	void FunctionRef::bind(Function **referent) {
		if (this->is_std) {
			std::cerr << "cannot bind a FunctionRef to an std function.\n";
			exit(-1);
		}

		this->referent = referent;
	}

	std::string_view FunctionRef::get_ref_name() const {
		if (this->is_std) {
			return this->name;
		}

		if (this->referent) {
			return (*this->referent)->name;
		} else {
			return this->name;
		}
	}

	std::string FunctionRef::to_string() const {
		std::string result;
		if (!this->is_std) {
			result += "@";
		}
		result += this->get_ref_name();
		return result;
	}

	void FunctionRef::bind_all(AggregateScope &agg_scope) {
		if (!this->is_std) {
			agg_scope.function_scope.add_ref(*this);
		}
	}

	std::string InstructionReturn::to_string() const {
		return "return";
	}

	AssignOperator str_to_ass_op(const std::string_view &str) {
		const std::map<std::string, AssignOperator, std::less<void>> map {
			{ "<-", AssignOperator::pure },
			{ "+=", AssignOperator::add },
			{ "-=", AssignOperator::subtract },
			{ "*=", AssignOperator::multiply },
			{ "&=", AssignOperator::bitwise_and },
			{ "<<=", AssignOperator::lshift },
			{ ">>=", AssignOperator::rshift }
		};
		return map.find(str)->second;
	}

	std::string to_string(AssignOperator op) {
		static const std::string assign_operator_to_str[] = {
			"<-", "+=", "-=", "*=", "&=", "<<=", ">>="
		};
		return assign_operator_to_str[static_cast<int>(op)];
	}

	std::string InstructionAssignment::to_string() const {
		return this->destination->to_string() + " " + program::to_string(this->op)
			+ " " + this->source->to_string();
	}
	void InstructionAssignment::bind_all(AggregateScope &agg_scope) {
		this->source->bind_all(agg_scope);
		this->destination->bind_all(agg_scope);
	}

	std::string to_string(ComparisonOperator op){
		static const std::string arr[] = {"<=", "<", "="};
		return arr[static_cast<int>(op)];
	}

	ComparisonOperator str_to_cmp_op(const std::string_view &str) {
		const std::map<std::string, ComparisonOperator, std::less<void>> map {
			{ "<", ComparisonOperator::lt },
			{ "<=", ComparisonOperator::le },
			{ "=", ComparisonOperator::eq }
		};
		return map.find(str)->second;
	}

	std::string InstructionCompareAssignment::to_string() const {
		std::string result = "";
		result += this->destination->to_string() + " <- ";
		result += this->lhs->to_string();
		result += program::to_string(this->op);
		result += this->rhs->to_string();
		return result;
	}

	void InstructionCompareAssignment::bind_all(AggregateScope &agg_scope) {
		this->destination->bind_all(agg_scope);
		this->lhs->bind_all(agg_scope);
		this->rhs->bind_all(agg_scope);
	}

	std::string InstructionCompareJump::to_string() const {
		std::string sol = "cjump ";
		sol += this->lhs->to_string() + " ";
		sol += program::to_string(this->op) + " ";
		sol += this->rhs->to_string() + " ";
		sol += this->label->to_string();
		return sol;
	}

	void InstructionCompareJump::bind_all(AggregateScope &agg_scope) {
		this->label->bind_all(agg_scope);
		this->lhs->bind_all(agg_scope);
		this->rhs->bind_all(agg_scope);
	}

	std::string InstructionLabel::to_string() const {
		return this->label_name;
	}

	void InstructionLabel::bind_all(AggregateScope &agg_scope) {
		agg_scope.label_scope.resolve_item(this->label_name, this);
	}

	std::string InstructionGoto::to_string() const {
		return "goto " + this->label->to_string();
	}

	void InstructionGoto::bind_all(AggregateScope &agg_scope) {
		this->label->bind_all(agg_scope);
	}

	std::string InstructionCall::to_string() const {
		return "call " + this->callee->to_string() + " " + std::to_string(this->num_arguments);
	}

	void InstructionCall::bind_all(AggregateScope &agg_scope) {
		this->callee->bind_all(agg_scope);
	}

	std::string InstructionLeaq::to_string() const {
		return this->destination->to_string() + " @ " + this->base->to_string()
			+ " " + this->base->to_string() + " " + std::to_string(this->scale);
	}

	void InstructionLeaq::bind_all(AggregateScope &agg_scope) {
		this->destination->bind_all(agg_scope);
		this->base->bind_all(agg_scope);
		this->offset->bind_all(agg_scope);
	}

	void Function::add_instruction(std::unique_ptr<Instruction> &&inst) {
		inst->bind_all(this->agg_scope);
		this->instructions.push_back(std::move(inst));
	}

	void Function::bind_all(AggregateScope &agg_scope) {
		this->agg_scope.variable_scope.set_parent(agg_scope.variable_scope);
		this->agg_scope.register_scope.set_parent(agg_scope.register_scope);
		this->agg_scope.label_scope.set_parent(agg_scope.label_scope);
		this->agg_scope.function_scope.set_parent(agg_scope.function_scope);
		agg_scope.function_scope.resolve_item(this->name, this);
	}

	std::string Function::to_string() const {
		std::string result = "(@" + this->name + " " + this->num_arguments->to_string();
		for (const auto &inst : this->instructions) {
			result += "\n" + inst->to_string();
		}
		result += "\n)";
		return result;
	}

	Program::Program(std::unique_ptr<FunctionRef> &&entry_function_ref) :
		entry_function_ref {std::move(entry_function_ref)},
		functions {},
		agg_scope {}
	{
		this->agg_scope.function_scope.add_ref(*(this->entry_function_ref));
	}

	std::string Program::to_string() const {
		std::string result = "(" + this->entry_function_ref->to_string();
		for (const auto &function : this->functions) {
			result += "\n" + function->to_string();
		}
		result += "\n)";
		return result;
	}

	void Program::add_function(std::unique_ptr<Function> &&func){
		func->bind_all(this->agg_scope);
		this->functions.push_back(std::move(func));
	}

	AggregateScope &Program::get_scope() {
		return this->agg_scope;
	}

	std::vector<Register> generate_registers() {
		std::vector<Register> result;
		result.emplace_back("rax", true, -1);
		result.emplace_back("rdi", true, 0);
		result.emplace_back("rsi", true, 1);
		result.emplace_back("rdx", true, 2);
		result.emplace_back("rcx", true, 3);
		result.emplace_back("r8", true, 4);
		result.emplace_back("r9", true, 5);
		result.emplace_back("r10", true, -1);
		result.emplace_back("r11", true, -1);
		result.emplace_back("r12", false, -1);
		result.emplace_back("r13", false, -1);
		result.emplace_back("r14", false, -1);
		result.emplace_back("r15", false, -1);
		result.emplace_back("rbx", false, -1);
		result.emplace_back("rbp", false, -1);
		result.emplace_back("rsp", false, -1);
		return result;
	}
}
