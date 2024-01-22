#include <L2.h>
#include <map>
#include <utility>
#include <functional>
#include <charconv>

namespace L2::program {
	std::map<std::string, RegisterID, std::less<void>> str_to_reg_id {
		{ "rax", RegisterID::rax },
		{ "rcx", RegisterID::rcx },
		{ "rdx", RegisterID::rdx },
		{ "rdi", RegisterID::rdi },
		{ "rsi", RegisterID::rsi },
		{ "r8", RegisterID::r8 },
		{ "r9", RegisterID::r9 },
		{ "rsp", RegisterID::rsp }
	};
	std::string reg_id_to_str[] = {
		"rax",
		"rcx",
		"rdx",
		"rdi",
		"rsi",
		"r8",
		"r9",
		"rsp"
	};

	Register::Register(const std::string_view &id) :
		id {str_to_reg_id.find(id)->second}
	{}

	std::string Register::to_string() const {
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
}