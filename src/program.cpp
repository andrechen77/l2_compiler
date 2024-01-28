#include "program.h"
#include <map>
#include <utility>
#include <functional>
#include <charconv>

namespace L2::program {
	// TODO remove this
	/* const std::map<std::string, RegisterID, std::less<void>> str_to_reg_id {
		{ "rax", RegisterID::rax },
		{ "rcx", RegisterID::rcx },
		{ "rdx", RegisterID::rdx },
		{ "rdi", RegisterID::rdi },
		{ "rsi", RegisterID::rsi },
		{ "r8", RegisterID::r8 },
		{ "r9", RegisterID::r9 },
		{ "rsp", RegisterID::rsp }
	};

	RegisterRef::RegisterRef(const std::string_view &id) :
		id {str_to_reg_id.find(id)->second}
	{}

	static const std::string reg_id_to_str[] = {
		"rax",
		"rcx",
		"rdx",
		"rdi",
		"rsi",
		"r8",
		"r9",
		"rsp"
	};
	*/
	std::string RegisterRef::to_string() const {
		if (this->referent) {
			return this->referent->name;
		} else {
			return std::string(this->free_name);
		}
	}

	void RegisterRef::bind_all(Scope &fun_scope) {
		*this = fun_scope.get_register_or_fail(this->free_name);
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

	std::string NumberLiteral::to_string() const {
		return std::to_string(this->value);
	}

	LabelRef::LabelRef(const std::string_view &free_name) :
		free_name {free_name},
		referent {nullptr}
	{}

	LabelRef::LabelRef(InstructionLabel *referent) :
		free_name {},
		referent {referent}
	{}

	void LabelRef::bind(InstructionLabel *referent) {
		this->referent = referent;
	}

	void LabelRef::bind_all(Scope &fun_scope){
		fun_scope.add_pending_label_ref(this);
	}

	std::string_view LabelRef::get_label_name() const {
		if (this->referent) {
			return this->referent->label_name;
		} else {
			return this->free_name;
		}
	}

	std::string LabelRef::to_string() const {
		return ":" + std::string(this->get_label_name());
	}

	VariableRef::VariableRef(const std::string_view &free_name) :
		free_name {free_name},
		referent {nullptr}
	{}

	VariableRef::VariableRef(Variable *referent) :
		free_name {},
		referent {referent}
	{}

	void VariableRef::bind_all(Scope &fun_scope) {
		*this = fun_scope.get_variable_create(this->free_name);
	}

	std::string_view VariableRef::get_var_name() const {
		if (this->referent) {
			return this->referent->name;
		} else {
			return this->free_name;
		}
	}

	std::string VariableRef::to_string() const {
		return "%" + std::string(this->get_var_name());
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

	FunctionRef::FunctionRef(Function *referent) :
		name {},
		is_std {false},
		referent {referent}
	{}

	void FunctionRef::bind(Function *referent) {
		if (this->is_std) {
			std::cerr << "cannot bind a FunctionRef to an std function.\n";
			exit(-1);
		}

		this->referent = referent;
	}

	std::string_view FunctionRef::get_fun_name() const {
		if (this->is_std) {
			return this->name;
		}

		if (this->referent) {
			return this->referent->name;
		} else {
			return this->name;
		}
	}

	std::string FunctionRef::to_string() const {
		std::string result;
		if (!this->is_std) {
			result += "@";
		}
		result += this->get_fun_name();
		return result;
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
	void InstructionAssignment::bind_all(Scope &scope) {
		this->source->bind_all(scope);
		this->destination->bind_all(scope);
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

	void InstructionCompareAssignment::bind_all(Scope &scope) {
		this->destination->bind_all(scope);
		this->lhs->bind_all(scope);
		this->rhs->bind_all(scope);
	}

	std::string InstructionCompareJump::to_string() const {
		std::string sol = "cjump ";
		sol += this->lhs->to_string() + " ";
		sol += program::to_string(this->op) + " ";
		sol += this->rhs->to_string() + " ";
		sol += this->label->to_string();
		return sol;
	}

	void InstructionCompareJump::bind_all(Scope &scope) {
		this->label->bind_all(scope);
		this->lhs->bind_all(scope);
		this->rhs->bind_all(scope);
	}

	std::string InstructionLabel::to_string() const {
		return this->label_name;
	}

	void InstructionLabel::bind_all(Scope &scope) {
		scope.resolve_label(*this);
	}

	std::string InstructionGoto::to_string() const {
		return "goto " + this->label->to_string();
	}

	void InstructionGoto::bind_all(Scope &scope) {
		this->label->bind_all(scope);
	}

	std::string InstructionCall::to_string() const {
		return "call " + this->callee->to_string() + " " + std::to_string(this->num_arguments);
	}

	void InstructionCall::bind_all(Scope &scope) {
		this->callee->bind_all(scope);
	}

	std::string InstructionLeaq::to_string() const {
		return this->destination->to_string() + " @ " + this->base->to_string()
			+ " " + this->base->to_string() + " " + std::to_string(this->scale);
	}
	
	void InstructionLeaq::bind_all(Scope &scope) {
		this->destination->bind_all(scope);
		this->base->bind_all(scope);
		this->offset->bind_all(scope);
	}

	Scope::Scope(std::optional<Scope *> parent) : parent {parent} {} // default initialize everything else

	VariableRef Scope::get_variable_create(const std::string_view &name) {
		std::optional<VariableRef> maybe_variable = get_variable_maybe(name);
		if (maybe_variable) {
			// the variable already exists, so return it
			return *maybe_variable;
		} else {
			// the variable doens't exist, so make one in this scope
			auto pair = this->var_dic.insert(std::make_pair(std::string(name), Variable(name)));
			return VariableRef(&(pair.first->second));
		}
	}

	std::optional<VariableRef> Scope::get_variable_maybe(const std::string_view &name) {
		auto it = this->var_dic.find(name);
		if (it == this->var_dic.end()) {
			if (this->parent) {
				return (*this->parent)->get_variable_maybe(name);
			} else {
				return {};
			}
		} else {
			return std::make_optional<VariableRef>(&(it->second));
		}
	}

	RegisterRef Scope::get_register_or_fail(const std::string_view &name) {
		auto it = this->reg_dic.find(name);
		if (it == this->reg_dic.end()){
			if (this->parent) {
				return (*this->parent)->get_register_or_fail(name);
			} else {
				std::cerr << "HOLY SHIT THE WORLD IS BURNING NO REGISTER: " << name << " REEEEE\n";
				exit(-1);
			}
		} else {
			return &it->second;
		}
	}

	void Scope::add_pending_label_ref(LabelRef *label_ref) {
		std::string_view label_name = label_ref->get_label_name();
		auto instruction_label_it = this->label_dic.find(label_name);
		if (instruction_label_it == this->label_dic.end()) {
			// if here then label_ref has not matched yet
			this->unmatched_labels[std::string(label_name)].push_back(label_ref);
		} else {
			// if here then label_ref has been matched
			label_ref->bind(instruction_label_it->second);
		}
	}

	void Scope::resolve_label(InstructionLabel &instruction_label){
		auto instruction_label_it = this->label_dic.find(instruction_label.label_name);
		if (instruction_label_it == this->label_dic.end()) {
			std::cerr << "TWO LABELS OF THE SAME NAME: " << instruction_label.label_name << "\n";
		}

		auto unmatched_labels_vector_it = this->unmatched_labels.find(instruction_label.label_name);
		if (unmatched_labels_vector_it != this->unmatched_labels.end()) {
			for (LabelRef *label_ref : unmatched_labels_vector_it->second) {
				label_ref->bind(&instruction_label);
			}
			this->unmatched_labels.erase(unmatched_labels_vector_it);
		}
	}

	void Function::add_instruction(std::unique_ptr<Instruction> &&inst) {
		inst->bind_all(this->scope);
		this->instructions.push_back(std::move(inst));
	}

	void Function::bind_all(Scope &program_scope){
		//TODO
	}

	std::string Function::to_string() const {
		std::string result = "(" + this->name + " " + this->num_arguments->to_string();
		for (const auto &inst : this->instructions) {
			result += "\n" + inst->to_string();
		}
		result += "\n)";
		return result;
	}

	Program::Program(
		std::unique_ptr<FunctionRef> &&entry_function_ref,
	) :
		entry_function_ref {std::move(entry_function_ref)},
		program_scope {}
	{}

	std::string Program::to_string() const {
		std::string result = "(" + this->entry_function_ref->to_string();
		for (const auto &function : this->functions) {
			result += "\n" + function->to_string();
		}
		result += "\n)";
		return result;
	}
	
	void Program::add_function(std::unique_ptr<Function> &&func){
		func->bind_all(this->program_scope);
		if (function->name == this->entry_function_ref->get_fun_name()) {
			this->entry_function_ref->bind(function.get());
			break;
		}
		this->functions.push_back(move(func));
	}
}
