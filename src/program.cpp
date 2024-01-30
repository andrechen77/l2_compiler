#include "program.h"
#include "utils.h"
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

	L2FunctionRef::L2FunctionRef(const std::string_view &free_name) :
		free_name {free_name},
		referent {nullptr}
	{}

	void L2FunctionRef::bind(L2Function **referent) {
		this->referent = referent;
	}

	std::string_view L2FunctionRef::get_ref_name() const {
		if (this->referent) {
			return (*this->referent)->get_name();
		} else {
			return this->free_name;
		}
	}

	std::string L2FunctionRef::to_string() const {
		return "@" + std::string(this->get_ref_name());
	}

	void L2FunctionRef::bind_all(AggregateScope &agg_scope) {
		agg_scope.l2_function_scope.add_ref(*this);
	}

	ExternalFunctionRef::ExternalFunctionRef(const std::string_view &free_name) :
		free_name {free_name},
		referent {nullptr}
	{}

	void ExternalFunctionRef::bind(ExternalFunction **referent) {
		this->referent = referent;
	}

	std::string_view ExternalFunctionRef::get_ref_name() const {
		if (this->referent) {
			return (*this->referent)->get_name();
		} else {
			return this->free_name;
		}
	}

	std::string ExternalFunctionRef::to_string() const {
		return std::string(this->get_ref_name());
	}

	void ExternalFunctionRef::bind_all(AggregateScope &agg_scope) {
		agg_scope.external_function_scope.add_ref(*this);
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

	void AggregateScope::set_parent(AggregateScope &parent) {
		this->variable_scope.set_parent(parent.variable_scope);
		this->register_scope.set_parent(parent.register_scope);
		this->label_scope.set_parent(parent.label_scope);
		this->l2_function_scope.set_parent(parent.l2_function_scope);
		this->external_function_scope.set_parent(parent.external_function_scope);
	}

	Function::Function(const std::string_view &name, int64_t num_arguments) :
		name {name}, num_arguments {num_arguments}
	{}

	std::string Function::to_string() const { return this->name; }

	ExternalFunction::ExternalFunction(const std::string_view &name, int64_t num_arguments, bool never_returns) :
		Function(name, num_arguments),
		never_returns {never_returns}
	{}

	bool ExternalFunction::get_never_returns() const {
		return this->never_returns;
	}

	L2Function::L2Function(
		const std::string_view &name,
		int64_t num_arguments
	) :
		Function(name, num_arguments),
		instructions {},
		agg_scope {}
	{}

	void L2Function::add_instruction(std::unique_ptr<Instruction> &&inst) {
		inst->bind_all(this->agg_scope);
		this->instructions.push_back(std::move(inst));
	}

	void L2Function::bind_all(AggregateScope &agg_scope) {
		this->agg_scope.set_parent(agg_scope);
		agg_scope.l2_function_scope.resolve_item(this->get_name(), this);
	}

	std::string L2Function::to_string() const {
		std::string result = "(@"
			+ this->Function::to_string()
			+ " "
			+ std::to_string(this->get_num_arguments());
		for (const auto &inst : this->instructions) {
			result += "\n" + inst->to_string();
		}
		result += "\n)";
		return result;
	}

	bool L2Function::get_never_returns() const { return false; }

	Program::Program(std::unique_ptr<L2FunctionRef> &&entry_function_ref) :
		entry_function_ref {std::move(entry_function_ref)},
		functions {},
		agg_scope {}
	{
		this->agg_scope.l2_function_scope.add_ref(*(this->entry_function_ref));
	}

	std::string Program::to_string() const {
		std::string result = "(" + this->entry_function_ref->to_string();
		for (const auto &function : this->functions) {
			result += "\n" + function->to_string();
		}
		result += "\n)";
		return result;
	}

	void Program::add_l2_function(std::unique_ptr<L2Function> &&func){
		func->bind_all(this->agg_scope);
		this->functions.push_back(std::move(func));
	}

	void Program::add_external_function(std::unique_ptr<ExternalFunction> &&func) {
		this->agg_scope.external_function_scope.resolve_item(func->get_name(), func.get());
		this->external_functions.push_back(std::move(func));
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

	std::vector<std::unique_ptr<ExternalFunction>> generate_std_functions() {
		std::vector<std::unique_ptr<ExternalFunction>> result;
		result.push_back(std::make_unique<ExternalFunction>("print", 1, false));
		result.push_back(std::make_unique<ExternalFunction>("input", 0, false));
		result.push_back(std::make_unique<ExternalFunction>("allocate", 2, false));
		result.push_back(std::make_unique<ExternalFunction>("tensor-error", 3, true));
		result.push_back(std::make_unique<ExternalFunction>("tuple-error", -1, true));
		return result;
	}
}
