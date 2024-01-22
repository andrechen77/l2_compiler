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
}