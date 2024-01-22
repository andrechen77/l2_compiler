#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace L2::program {
	enum struct RegisterID {
		rax,
		rcx,
		rdx,
		rdi,
		rsi,
		r8,
		r9,
		rsp
	};

	struct Value {
		virtual std::string to_string() const = 0;
	};

	struct Register : Value {
		RegisterID id;
		Register(const std::string_view &id);
		virtual std::string to_string() const override;
	};

	struct MemoryLocation : Value {
		std::unique_ptr<Value> base;
		int64_t offset;
		MemoryLocation(std::unique_ptr<Value> &&base, int64_t offset);
		virtual std::string to_string() const override;
	};

	struct NumberLiteral : Value {
		int64_t value;
		NumberLiteral(int64_t value);
		virtual std::string to_string() const override;
	};

	struct LabelLocation : Value {
		std::string label_name;
		LabelLocation(const std::string_view &labelName);
		virtual std::string to_string() const override;
	};

	struct Variable : Value {
		std::string var_name;
		Variable(const std::string_view &var_name);
		virtual std::string to_string() const override;
	};

	class InstructionVisitor {
		virtual void visit(InstructionReturn &inst) = 0;
		// TODO add visit methods for all instructions here
	}

	struct Instruction {
		virtual std::string to_string() const = 0;
		virtual void accept(InstructionVisitor &v) = 0;
	}

	struct InstructionReturn : Instruction {
		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) const override;
	}

	struct Program {};
}
