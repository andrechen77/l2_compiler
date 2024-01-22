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

	struct FunctionRef : Value {
		std::string function_name;
		bool is_std;

		FunctionRef(const std::string_view &var_name, bool is_std);

		virtual std::string to_string() const override;
	};

	class InstructionVisitor;

	struct Instruction {
		virtual std::string to_string() const = 0;
		virtual void accept(InstructionVisitor &v) = 0;
	};

	struct InstructionReturn : Instruction {
		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override;
	};

	enum struct AssignOperator {
		pure,
		add,
		subtract,
		multiply,
		bitwise_and,
		lshift,
		rshift
	};

	std::string to_string(AssignOperator op);

	struct InstructionAssignment : Instruction {
		std::unique_ptr<Value> source;
		AssignOperator op;
		std::unique_ptr<Value> destination;

		InstructionAssignment(
			AssignOperator op,
			std::unique_ptr<Value> &&source,
			std::unique_ptr<Value> &&destination
		);

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override;
	};

	enum struct ComparisonOperator {
		lt,
		le,
		eq
	};

	std::string to_string(ComparisonOperator op);

	struct InstructionCompareAssignment : Instruction {
		std::unique_ptr<Value> destination;
		ComparisonOperator op;
		std::unique_ptr<Value> lhs;
		std::unique_ptr<Value> rhs;

		InstructionCompareAssignment(
			std::unique_ptr<Value> &&destination,
			ComparisonOperator op,
			std::unique_ptr<Value> &&lhs,
			std::unique_ptr<Value> &&rhs
		);

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override;
	};

	struct InstructionGoto : Instruction {
		std::unique_ptr<LabelLocation> label;

		InstructionGoto(std::unique_ptr<LabelLocation> &&label);

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override;
	};

	struct InstructionCall : Instruction {
		std::unique_ptr<Value> callee;
		int64_t num_arguments; // specified by user, doesn't necessarily match function's actual num args

		InstructionCall(std::unique_ptr<Value> &&callee, int64_t num_arguments);

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override;
	};

	struct InstructionLeaq : Instruction {
		std::unique_ptr<Value> destination;
		std::unique_ptr<Value> base;
		std::unique_ptr<Value> offset;
		int64_t scale;

		InstructionLeaq(
			std::unique_ptr<Value> &&destination,
			std::unique_ptr<Value> &&base,
			std::unique_ptr<Value> &&offset,
			int64_t scale
		);

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override;
	};

	class InstructionVisitor {
		public:

		virtual void visit(InstructionReturn &inst) = 0;
		virtual void visit(InstructionAssignment &inst) = 0;
		virtual void visit(InstructionCompareAssignment &inst) = 0;
		virtual void visit(InstructionGoto &inst) = 0;
		virtual void visit(InstructionCall &inst) = 0;
		virtual void visit(InstructionLeaq &inst) = 0;
		// TODO add visit methods for all instructions here
	};

	struct Program {};
}
