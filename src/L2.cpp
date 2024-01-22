#include <L2.h>
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

	std::string Register::to_string() const {
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
		return reg_id_to_str[static_cast<int>(this->id)];
	}

	MemoryLocation::MemoryLocation(std::unique_ptr<Value> &&base, int64_t offset) :
		base {std::move(base)}, offset {offset}
	{}

	std::string MemoryLocation::to_string() const {
		return "mem " + this->base->to_string() + " " + std::to_string(this->offset);
	}

	NumberLiteral::NumberLiteral(int64_t value) : value {value} {}

	std::string NumberLiteral::to_string() const {
		return std::to_string(this->value);
	}

	LabelLocation::LabelLocation(const std::string_view &label_name) : label_name {label_name} {}

	std::string LabelLocation::to_string() const {
		return this->label_name;
	}

	Variable::Variable(const std::string_view &var_name) : var_name {var_name} {}

	std::string Variable::to_string() const {
		return this->var_name;
	}

	FunctionRef::FunctionRef(const std::string_view &function_name, bool is_std) :
		function_name {function_name},
		is_std {is_std}
	{}

	std::string FunctionRef::to_string() const {
		std::string result;
		if (this->is_std) {
			result += "std::";
		}
		result += this->function_name;
		return result;
	}

	std::string InstructionReturn::to_string() const {
		return "return";
	}

	void InstructionReturn::accept(InstructionVisitor &v) { v.visit(*this); }

	std::string to_string(AssignOperator op) {
		static const std::string assign_operator_to_str[] = {
			"<-", "+=", "-=", "*=", "&=", "<<=", ">>="
		};
		return assign_operator_to_str[static_cast<int>(op)];
	}

	InstructionAssignment::InstructionAssignment(
		AssignOperator op,
		std::unique_ptr<Value> &&source,
		std::unique_ptr<Value> &&destination
	) :
		op { op }, source { std::move(source) }, destination { std::move(destination )}
	{}

	std::string InstructionAssignment::to_string() const {
		return this->source->to_string() + " " + program::to_string(this->op)
			+ " " + this->destination->to_string();
	}

	void InstructionAssignment::accept(InstructionVisitor &v) { v.visit(*this); }

	std::string to_string(ComparisonOperator op){
		static const std::string arr[] = {"<=", "<", "="};
		return arr[static_cast<int>(op)];
	}

	InstructionCompareAssignment::InstructionCompareAssignment(
		std::unique_ptr<Value> &&destination,
		ComparisonOperator op,
		std::unique_ptr<Value> &&lhs,
		std::unique_ptr<Value> &&rhs
	) :
		destination {std::move(destination)},
		op {op},
		lhs {std::move(lhs)},
		rhs {std::move(rhs)}
	{}

	std::string InstructionCompareAssignment::to_string() const {
		std::string result = "";
		result += this->destination->to_string() + " <- ";
		result += this->lhs->to_string();
		result += program::to_string(this->op);
		result += this->rhs->to_string();
		return result;
	}

	void InstructionCompareAssignment::accept(InstructionVisitor &v) {v.visit(*this); }

	InstructionCompareJump::InstructionCompareJump(
		const ComparisonOperator op,
		std::unique_ptr<Value> &&lhs,
		std::unique_ptr<Value> &&rhs,
		std::unique_ptr<LabelLocation> label
	):
		op {op}, lhs{std::move(lhs)}, rhs{std::move(rhs)}, label{std::move(label)}
	{}

	std::string InstructionCompareJump::to_string() const {
		std::string sol = "cjump ";
		sol += this->lhs->to_string() + " ";
		sol += program::to_string(this->op) + " ";
		sol += this->rhs->to_string() + " ";
		sol += this->label->to_string();
		return sol;
	}

	void InstructionCompareJump::accept(InstructionVisitor &v) { v.visit(*this); }

	InstructionGoto::InstructionGoto(std::unique_ptr<LabelLocation> &&label) :
		label {std::move(label)}
	{}

	std::string InstructionGoto::to_string() const {
		return "goto " + this->label->to_string();
	}

	void InstructionGoto::accept(InstructionVisitor &v) { v.visit(*this); }

	InstructionCall::InstructionCall(std::unique_ptr<Value> &&callee, int64_t num_arguments) :
		callee {std::move(callee)},
		num_arguments {num_arguments}
	{}

	std::string InstructionCall::to_string() const {
		return "call " + this->callee->to_string() + std::to_string(this->num_arguments);
	}

	void InstructionCall::accept(InstructionVisitor &v) { v.visit(*this); }

	InstructionLeaq::InstructionLeaq(
		std::unique_ptr<Value> &&destination,
		std::unique_ptr<Value> &&base,
		std::unique_ptr<Value> &&offset,
		int64_t scale
	) :
		destination {std::move(destination)},
		base {std::move(base)},
		offset {std::move(offset)},
		scale {scale}
	{}

	std::string InstructionLeaq::to_string() const {
		return this->destination->to_string() + " @ " + this->base->to_string()
			+ " " + this->base->to_string() + " " + std::to_string(this->scale);
	}

	void InstructionLeaq::accept(InstructionVisitor &v) { v.visit(*this); }
}