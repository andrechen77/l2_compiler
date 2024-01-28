#pragma once

#include <memory>
#include <vector>
#include <map>
#include <optional>
#include <string>
#include <iostream>
#include <string_view>
#include <set>

namespace L2::program {
	// TODO remove this (or move it to something like simone's architecture.cpp)
	// enum struct RegisterID {
	// 	rax,
	// 	rcx,
	// 	rdx,
	// 	rdi,
	// 	rsi,
	// 	r8,
	// 	r9,
	// 	rsp
	// };

	struct Variable;
	struct Register;
	class Scope;

	class Expr {
		public:

		virtual std::string to_string() const = 0;

		// which sub-values are read when this Expr is read
		virtual std::set<Variable *> get_vars_on_read() const {
			return {};
		}
		// which sub-values are read/written when this Expr is written
		virtual std::set<Variable *> get_vars_on_write(bool get_read_vars) const {
			return {};
		}
		virtual void bind_all(Scope &fun_scope){}
	};

	class RegisterRef : public Expr {
		private:
		std::string_view free_name;
		Register *referent;

		public:

		RegisterRef(Register* referent);
		RegisterRef(const std::string_view &free_name);

		virtual std::string to_string() const override;
		virtual std::set<Variable *> get_vars_on_read() const override;
		virtual std::set<Variable *> get_vars_on_write(bool get_read_vars) const override;
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct NumberLiteral : Expr {
		int64_t value;

		NumberLiteral(int64_t value) : value {value} {}

		virtual std::string to_string() const override;
	};

	struct StackArg : Expr {
		std::unique_ptr<NumberLiteral> stack_num;

		StackArg(std::unique_ptr<NumberLiteral> &&stack_num) : stack_num {std::move(stack_num)} {}

		virtual std::string to_string() const override;
	};

	struct MemoryLocation : Expr {
		std::unique_ptr<Expr> base;
		std::unique_ptr<NumberLiteral> offset;

		MemoryLocation(std::unique_ptr<Expr> &&base, std::unique_ptr<NumberLiteral> &&offset) :
			base {std::move(base)}, offset {std::move(offset)}
		{}

		virtual std::string to_string() const override;
		virtual std::set<Variable *> get_vars_on_read() const override;
		virtual std::set<Variable *> get_vars_on_write(bool get_read_vars) const override;

	};

	struct InstructionLabel;

	class LabelRef : public Expr {
		private:
		std::string free_name;
		InstructionLabel *referent;

		public:

		LabelRef(const std::string_view &free_name);
		LabelRef(InstructionLabel *referent);

		void bind(InstructionLabel *referent);
		std::string_view get_label_name() const;
		virtual std::string to_string() const override;
		virtual void bind_all(Scope &fun_scope) override;
	};

	/*
	FUTURE: Instead of these containing the name of the variable itself (i hate strings),
	we rename these to VariableRefs and have them refer to a Variable contained
	in the innermost Scope. A Scope is an object that is owned by something like
	a Function which just describes all the names (such as variable names and
	label names) within that scope. This happens during parsing.
	Then we can have instruction analysis work with references to those Variable
	objects instead of with strings (I hate strings)
	*/
	class VariableRef : public Expr {
		private:
		// the null-ness of this->referent determines whether this is a free
		// or bound variable.
		// once bound, this->name has no meaning and should never be used
		std::string_view free_name;
		Variable *referent;

		public:

		// VariableRefs *must* be bound before the passed-in string_view becomes invalid
		VariableRef(const std::string_view &free_name);
		VariableRef(Variable *referent);

		void bind(Variable *referent);

		std::string_view get_var_name() const;
		virtual std::string to_string() const override;
		virtual std::set<Variable *> get_vars_on_read() const override;
		virtual std::set<Variable *> get_vars_on_write(bool get_read_vars) const override;
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct Function;

	class FunctionRef : public Expr {
		private:

		// is_std determines whether this refers to a standard library function
		// or an L2 function. if the former, then this->name will be permanently
		// held as the name of the std function; otherwise this FunctionRef
		// should be bound to an L2 function
		// TODO make it so that we can refer to std functions similarly to
		// L2 functions such as by making a superclass of both
		bool is_std;
		std::string name;
		Function *referent;

		public:

		FunctionRef(const std::string_view &name, bool is_std);
		FunctionRef(Function *referent);

		void bind(Function *referent);

		// TODO why is this here?
		FunctionRef(const FunctionRef &other) :
			name {other.name},
			is_std {other.is_std}
		{}

		std::string_view get_fun_name() const;
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
		virtual void bind_all(Scope &fun_scope){}
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
		std::unique_ptr<Expr> source;
		AssignOperator op;
		std::unique_ptr<Expr> destination;

		InstructionAssignment(
			AssignOperator op,
			std::unique_ptr<Expr> &&source,
			std::unique_ptr<Expr> &&destination
		) :
			op { op }, source { std::move(source) }, destination { std::move(destination )}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	enum struct ComparisonOperator {
		lt,
		le,
		eq
	};

	ComparisonOperator str_to_cmp_op(const std::string_view &str);

	std::string to_string(ComparisonOperator op);

	struct InstructionCompareAssignment : Instruction {
		std::unique_ptr<Expr> destination;
		ComparisonOperator op;
		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;

		InstructionCompareAssignment(
			std::unique_ptr<Expr> &&destination,
			ComparisonOperator op,
			std::unique_ptr<Expr> &&lhs,
			std::unique_ptr<Expr> &&rhs
		) :
			destination {std::move(destination)},
			op {op},
			lhs {std::move(lhs)},
			rhs {std::move(rhs)}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct InstructionCompareJump : Instruction {
		ComparisonOperator op;
		std::unique_ptr<Expr> lhs;
		std::unique_ptr<Expr> rhs;
		std::unique_ptr<LabelRef> label;

		InstructionCompareJump(
			ComparisonOperator op,
			std::unique_ptr<Expr> &&lhs,
			std::unique_ptr<Expr> &&rhs,
			std::unique_ptr<LabelRef> label
		):
			op {op}, lhs{std::move(lhs)}, rhs{std::move(rhs)}, label{std::move(label)}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct InstructionLabel : Instruction {
		std::string label_name;

		InstructionLabel(const std::string_view &label_name) : label_name {label_name} {}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct InstructionGoto : Instruction {
		std::unique_ptr<LabelRef> label;

		InstructionGoto(std::unique_ptr<LabelRef> &&label) : label {std::move(label)} {}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct InstructionCall : Instruction {
		std::unique_ptr<Expr> callee;
		int64_t num_arguments; // specified by user, doesn't necessarily match function's actual num args

		InstructionCall(std::unique_ptr<Expr> &&callee, int64_t num_arguments) :
			callee {std::move(callee)},
			num_arguments {num_arguments}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct InstructionLeaq : Instruction {
		std::unique_ptr<Expr> destination;
		std::unique_ptr<Expr> base;
		std::unique_ptr<Expr> offset;
		int64_t scale;

		InstructionLeaq(
			std::unique_ptr<Expr> &&destination,
			std::unique_ptr<Expr> &&base,
			std::unique_ptr<Expr> &&offset,
			int64_t scale
		) :
			destination {std::move(destination)},
			base {std::move(base)},
			offset {std::move(offset)},
			scale {scale}
		{}

		virtual std::string to_string() const override;
		virtual void accept(InstructionVisitor &v) override { v.visit(*this); }
		virtual void bind_all(Scope &fun_scope) override;
	};

	struct Item {};

	struct Variable : Item {
		std::string name;

		Variable(const std::string_view &name) : name {name} {}
	};

	struct Register : Variable {
		Register(const std::string_view &name) : Variable(name) {}
	};

	// TODO create a Scope class
	// which maps variable names (strings) to Variable objects
	// which maps label names (strings) to their locations
	class Scope {
		std::map<std::string, InstructionLabel *, std::less<void>> label_dic;
		std::map<std::string, Variable, std::less<void>> var_dic;
		std::map<std::string, Register, std::less<void>> reg_dic;
		std::map<std::string, Function *, std::less<void>> fun_dic;
		std::map<std::string, std::vector<LabelRef*>, std::less<void>> unmatched_labels;
		std::optional<Scope *> parent;

		public:

		Scope(std::optional<Scope *> parent = {});

		VariableRef get_variable_create(const std::string_view &name);

		std::optional<VariableRef> get_variable_maybe(const std::string_view &name);

		RegisterRef get_register_or_fail(const std::string_view &name);

		void add_pending_label_ref(LabelRef *label_ref);

		void resolve_label(InstructionLabel &instruction_label);

		// TODO labels later
	};

	struct Function {
		std::string name;
		std::unique_ptr<NumberLiteral> num_arguments;
		std::vector<std::unique_ptr<Instruction>> instructions;
		Scope fun_scope;

		Function(
			const std::string_view &name,
			std::unique_ptr<NumberLiteral> &&num_arguments
			Scope *fun_scope
		) :
			name {name},
			num_arguments {std::move(num_arguments)},
			instructions {},
			scope {}
		{}

		void add_instruction(std::unique_ptr<Instruction> &&inst);
		void bind_all(Scope &program_scope);

		std::string to_string() const;
	};

	class Program {
		private:

		std::unique_ptr<FunctionRef> entry_function_ref;
		std::vector<std::unique_ptr<Function>> functions;
		Scope program_scope;

		public:

		Program(
			std::unique_ptr<FunctionRef> &&entry_function_ref,
		);

		std::string to_string() const;
		void add_function(std::unique_ptr<Function> &&func);

	};
}
