#include "program.h"
#include <map>
#include <utility>
#include <functional>
#include <charconv>

namespace L2::program {
	const std::map<std::string, RegisterID, std::less<void>> str_to_reg_id {
		{ "rax", RegisterID::rax },
		{ "rcx", RegisterID::rcx },
		{ "rdx", RegisterID::rdx },
		{ "rdi", RegisterID::rdi },
		{ "rsi", RegisterID::rsi },
		{ "r8", RegisterID::r8 },
		{ "r9", RegisterID::r9 },
		{ "rsp", RegisterID::rsp }
	};

	Register::Register(const std::string_view &id) :
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

	std::string Register::to_string() const {
		return reg_id_to_str[static_cast<int>(this->id)];
	}

	std::set<std::string> Register::get_read_vars() const {
		return {reg_id_to_str[static_cast<int>(this->id)]};
	}
	std::set<std::string> Register::get_write_vars() const {
		return {reg_id_to_str[static_cast<int>(this->id)]};
	}

	std::string StackArg::to_string() const {
        return "stack-arg " + this->stack_num->to_string();
    }

	std::string MemoryLocation::to_string() const {
		return "mem " + this->base->to_string() + " " + this->offset->to_string();
	}

	std::set<std::string> MemoryLocation::get_read_vars() const {
		return this->base->get_read_vars();
	}
	std::set<std::string> MemoryLocation::get_write_vars() const {
		return this->base->get_read_vars();
	}

	std::string NumberLiteral::to_string() const {
		return std::to_string(this->value);
	}

	std::string LabelLocation::to_string() const {
		return ":" + this->label_name;
	}

	std::string Variable::to_string() const {
		return "%" + this->var_name;
	}

	std::set<std::string> Variable::get_read_vars() const {
		return {this->var_name};
	}
	std::set<std::string> Variable::get_write_vars() const {
		return {this->var_name};
	}

	std::string FunctionRef::to_string() const {
		std::string result;
		if (!this->is_std) {
			result += "@";
		}
		result += this->function_name;
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

	std::string InstructionCompareJump::to_string() const {
		std::string sol = "cjump ";
		sol += this->lhs->to_string() + " ";
		sol += program::to_string(this->op) + " ";
		sol += this->rhs->to_string() + " ";
		sol += this->label->to_string();
		return sol;
	}

	std::string InstructionLabel::to_string() const {
		return this->label->to_string();
	}

	std::string InstructionGoto::to_string() const {
		return "goto " + this->label->to_string();
	}

	std::string InstructionCall::to_string() const {
		return "call " + this->callee->to_string() + " " + std::to_string(this->num_arguments);
	}

	std::string InstructionLeaq::to_string() const {
		return this->destination->to_string() + " @ " + this->base->to_string()
			+ " " + this->base->to_string() + " " + std::to_string(this->scale);
	}

	std::string Function::to_string() const {
		std::string result = "(" + this->name->to_string() + " " + this->num_arguments->to_string();
		for (const auto &inst : this->instructions) {
			result += "\n" + inst->to_string();
		}
		result += "\n)";
		return result;
	}

	std::string Program::to_string() const {
		std::string result = "(" + this->entry_function->to_string();
		for (const auto &function : this->functions) {
			result += "\n" + function->to_string();
		}
		result += "\n)";
		return result;
	}
}
