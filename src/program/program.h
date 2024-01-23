#pragma once

#include <memory>
#include <vector>
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

	struct NumberLiteral : Value {
		int64_t value;

		NumberLiteral(int64_t value) : value {value} {}

		virtual std::string to_string() const override;
	};

	struct StackArg : Value {
		std::unique_ptr<NumberLiteral> stack_num;

		StackArg(std::unique_ptr<NumberLiteral> &&stack_num) : stack_num {std::move(stack_num)} {}

		virtual std::string to_string() const override;
	};

	struct MemoryLocation : Value {
		std::unique_ptr<Value> base;
		std::unique_ptr<NumberLiteral> offset;

		MemoryLocation(std::unique_ptr<Value> &&base, std::unique_ptr<NumberLiteral> &&offset) :
			base {std::move(base)}, offset {std::move(offset)}
		{}

		virtual std::string to_string() const override;
	};

	struct LabelLocation : Value {
		std::string label_name;

		LabelLocation(const std::string_view &label_name) : label_name {label_name} {}

		virtual std::string to_string() const override;
	};

	struct Variable : Value {
		std::string var_name;

		Variable(const std::string_view &var_name) : var_name {var_name} {}

		virtual std::string to_string() const override;
	};

	struct FunctionRef : Value {
		std::string function_name;
		bool is_std;

		FunctionRef(const std::string_view &function_name, bool is_std) :
			function_name {function_name},
			is_std {is_std}
		{}

		virtual std::string to_string() const override;
	};

	struct InstructionReturn;
	struct InstructionAssignment;
	struct InstructionCompareAssignment;
	struct InstructionCompareJump;
	struct InstructionLabel;
	struct InstructionGoto;
	struct InstructionCall;
	struct InstructionLeaq;

	class InstructionVisitor {
		public:

		virtual void visit(InstructionReturn &inst) = 0;
		virtual void visit(InstructionAssignment &inst) = 0;
		virtual void visit(InstructionCompareAssignment &inst) = 0;
		virtual void visit(InstructionCompareJump &inst) = 0;
		virtual void visit(InstructionLabel &inst) = 0;
		virtual void visit(InstructionGoto &inst) = 0;
		virtual void visit(InstructionCall &inst) = 0;
		virtual void visit(InstructionLeaq &inst) = 0;
	};

	struct Instruction {
		virtual std::string to_string() const = 0;
		virtual void accept(InstructionVisitor &v) = 0;
	};

	struct InstructionReturn : Instruction {
		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
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

	AssignOperator str_to_ass_op(const std::string_view &str);

	std::string to_string(AssignOperator op);

	struct InstructionAssignment : Instruction {
		std::unique_ptr<Value> source;
		AssignOperator op;
		std::unique_ptr<Value> destination;

		InstructionAssignment(
			AssignOperator op,
			std::unique_ptr<Value> &&source,
			std::unique_ptr<Value> &&destination
		) :
			op { op }, source { std::move(source) }, destination { std::move(destination )}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
	};

	enum struct ComparisonOperator {
		lt,
		le,
		eq
	};

	ComparisonOperator str_to_cmp_op(const std::string_view &str);

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
		) :
			destination {std::move(destination)},
			op {op},
			lhs {std::move(lhs)},
			rhs {std::move(rhs)}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
	};

	struct InstructionCompareJump : Instruction {
		ComparisonOperator op;
		std::unique_ptr<Value> lhs;
		std::unique_ptr<Value> rhs;
		std::unique_ptr<LabelLocation> label;

		InstructionCompareJump(
			ComparisonOperator op,
			std::unique_ptr<Value> &&lhs,
			std::unique_ptr<Value> &&rhs,
			std::unique_ptr<LabelLocation> label
		):
			op {op}, lhs{std::move(lhs)}, rhs{std::move(rhs)}, label{std::move(label)}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
	};

	struct InstructionLabel : Instruction {
		std::unique_ptr<LabelLocation> label;

		InstructionLabel(std::unique_ptr<LabelLocation> &&label) : label {std::move(label)} {}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
	};

	struct InstructionGoto : Instruction {
		std::unique_ptr<LabelLocation> label;

		InstructionGoto(std::unique_ptr<LabelLocation> &&label) : label {std::move(label)} {}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
	};

	struct InstructionCall : Instruction {
		std::unique_ptr<Value> callee;
		int64_t num_arguments; // specified by user, doesn't necessarily match function's actual num args

		InstructionCall(std::unique_ptr<Value> &&callee, int64_t num_arguments) :
			callee {std::move(callee)},
			num_arguments {num_arguments}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
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
		) :
			destination {std::move(destination)},
			base {std::move(base)},
			offset {std::move(offset)},
			scale {scale}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
	};

	struct Function {
		std::unique_ptr<FunctionRef> name;
		std::unique_ptr<NumberLiteral> num_arguments;
		std::vector<std::unique_ptr<Instruction>> instructions;

		Function(
			std::unique_ptr<FunctionRef> &&name,
			std::unique_ptr<NumberLiteral> &&num_arguments,
			std::vector<std::unique_ptr<Instruction>> &&instructions
		) :
			name {std::move(name)},
			num_arguments {std::move(num_arguments)},
			instructions {std::move(instructions)}
		{}
	};

	struct Program {
		std::unique_ptr<FunctionRef> entry_function;
		std::vector<std::unique_ptr<Function>> functions;

		Program(
			std::unique_ptr<FunctionRef> &&entry_function,
			std::vector<std::unique_ptr<Function>> &&functions
		) :
			entry_function {std::move(entry_function)},
			functions {std::move(functions)}
		{}
	};
}
