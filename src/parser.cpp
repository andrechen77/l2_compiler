#include <sched.h>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <set>
#include <iterator>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <stdint.h>
#include <assert.h>
#include <fstream>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>

#include <L2.h>
#include <parser.h>

namespace pegtl = TAO_PEGTL_NAMESPACE;

namespace L2::parser {
	// TODO add comment explaining the visitor-with-dispatcher pattern

	struct ParseNode;

	struct ParseNodeVisitor {
		virtual void visit_name(ParseNode &n) = 0;
		virtual void visit_number(ParseNode &n) = 0;
		virtual void visit_program(ParseNode &n) = 0;
		virtual void visit_register(ParseNode &n) = 0;
		virtual void visit_tensor_arg_number(ParseNode &n) = 0;
		virtual void visit_arithmetic_operator(ParseNode &n) = 0;
		virtual void visit_shift_operator(ParseNode &n) = 0;
		virtual void visit_comparison_operator(ParseNode &n) = 0;
		virtual void visit_argument_number(ParseNode &n) = 0;
		virtual void visit_lea_factor(ParseNode &n) = 0;
		virtual void visit_label(ParseNode &n) = 0;
		virtual void visit_function_name(ParseNode &n) = 0;
		virtual void visit_variable(ParseNode &n) = 0;
		virtual void visit_instruction_assignment(ParseNode &n) = 0;
		virtual void visit_instruction_return(ParseNode &n) = 0;
		virtual void visit_instruction_memory_read(ParseNode &n) = 0;
		virtual void visit_entry_point(ParseNode &n) = 0;
	};

	namespace rules {
		using namespace pegtl; // for convenience of reading the rules

		// parent class that all grammar rules must implement in order to
		// generate a node in the parse tree.
		// allows the class to act as a dispatcher for ParseNodes matching that rule.
		struct RuleDispatcher {
			virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) = 0;
		};

		// TODO interleaved class

		struct str_return : TAO_PEGTL_STRING("return") {};
		struct str_arrow : TAO_PEGTL_STRING("\x3c-") {};
		struct str_rax : TAO_PEGTL_STRING("rax") {};
		struct str_rbx : TAO_PEGTL_STRING("rbx") {};
		struct str_rcx : TAO_PEGTL_STRING("rcx") {};
		struct str_rdx : TAO_PEGTL_STRING("rdx") {};
		struct str_rdi : TAO_PEGTL_STRING("rdi") {};
		struct str_rsi : TAO_PEGTL_STRING("rsi") {};
		struct str_r8 : TAO_PEGTL_STRING("r8") {};
		struct str_r9 : TAO_PEGTL_STRING("r9") {};
		struct str_r10 : TAO_PEGTL_STRING("r10") {};
		struct str_r11 : TAO_PEGTL_STRING("r11") {};
		struct str_r12 : TAO_PEGTL_STRING("r12") {};
		struct str_r13 : TAO_PEGTL_STRING("r13") {};
		struct str_r14 : TAO_PEGTL_STRING("r14") {};
		struct str_r15 : TAO_PEGTL_STRING("r15") {};
		struct str_rbp : TAO_PEGTL_STRING("rbp") {};
		struct str_rsp : TAO_PEGTL_STRING("rsp") {};
		struct str_plus : TAO_PEGTL_STRING("\x2b\x3d") {};
		struct str_minus : TAO_PEGTL_STRING("\x2d\x3d") {};
		struct str_times : TAO_PEGTL_STRING("\x2a\x3d") {};
		struct str_bitwise_and : TAO_PEGTL_STRING("\x26\x3d") {};
		struct str_lshift : TAO_PEGTL_STRING("\x3c\x3c=") {};
		struct str_rshift : TAO_PEGTL_STRING(">>=") {};
		struct str_lt : TAO_PEGTL_STRING("\x3c") {};
		struct str_le : TAO_PEGTL_STRING("\x3c=") {};
		struct str_eq : TAO_PEGTL_STRING("=") {};
		struct str_mem : TAO_PEGTL_STRING("mem") {};
		struct str_goto : TAO_PEGTL_STRING("goto") {};
		struct str_cjump : TAO_PEGTL_STRING("cjump") {};
		struct str_call : TAO_PEGTL_STRING("call") {};
		struct str_print : TAO_PEGTL_STRING("print") {};
		struct str_input : TAO_PEGTL_STRING("input") {};
		struct str_allocate : TAO_PEGTL_STRING("allocate") {};
		struct str_tuple_error : TAO_PEGTL_STRING("tuple-error") {};
		struct str_tensor_error : TAO_PEGTL_STRING("tensor-error") {};

		struct CommentRule :
			disable<
				TAO_PEGTL_STRING("//"),
				until<eolf>
			>
		{};

		struct SpacesRule :
			star<sor<one<' '>, one<'\t'>>>
		{};

		struct LineSeparatorsRule :
			star<seq<SpacesRule, eol>>
		{};

		struct LineSeparatorsWithCommentsRule :
			star<
				seq<
					SpacesRule,
					sor<
						eol,
						CommentRule
					>
				>
			>
		{};

		struct NumberRule : RuleDispatcher,
			sor<
				seq<
					opt<sor<one<'-'>, one<'+'>>>,
					range<'1', '9'>,
					star<digit>
				>,
				one<'0'>
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_number(n); } };

		struct ArgumentNumberRule : RuleDispatcher,
			seq<NumberRule>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_argument_number(n); } };

		struct TensorErrorArgNumberRule : RuleDispatcher,
			seq<NumberRule>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_tensor_arg_number(n); } };

		struct LeaFactorRule : RuleDispatcher,
			rematch<
				NumberRule,
				sor<one<'1'>, one<'2'>, one<'4'>, one<'8'>>
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_lea_factor(n); } };

		struct NameRule : RuleDispatcher,
			ascii::identifier // the rules for L2 rules are the same as for C identifiers
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_name(n); } };

		struct LabelRule : RuleDispatcher,
			seq<one<':'>, NameRule>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_label(n); } };

		struct FunctionNameRule : RuleDispatcher,
			seq<one<'@'>, NameRule>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_function_name(n); } };

		struct VariableRule : RuleDispatcher,
			seq<one<'%'>, NameRule>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_variable(n); } };

		struct RegisterRule : RuleDispatcher,
			sor<
				str_rax,
				str_rbx,
				str_rcx,
				str_rdx,
				str_rdi,
				str_rsi,
				str_r8,
				str_r9,
				str_r10,
				str_r11,
				str_r12,
				str_r13,
				str_r14,
				str_r15,
				str_rbp,
				str_rsp
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_register(n); } };

		struct InexplicableSxRule :
			sor<
				str_rcx,
				VariableRule
			>
		{};

		struct InexplicableARule :
			sor<
				InexplicableSxRule,
				str_rdi,
				str_rsi,
				str_rdx,
				str_r8,
				str_r9
			>
		{};

		struct InexplicableWRule :
			sor<
				InexplicableARule,
				str_rax
			>
		{};

		struct InexplicableXRule :
			sor<
				InexplicableWRule,
				str_rsp
			>
		{};

		struct InexplicableTRule :
			sor<
				InexplicableXRule,
				NumberRule
			>
		{};

		struct InexplicableSRule :
			sor<
				InexplicableTRule,
				LabelRule,
				FunctionNameRule
			>
		{};

		struct InexplicableURule :
			sor<
				InexplicableXRule,
				FunctionNameRule
			>
		{};

		struct ArithmeticOperatorRule : RuleDispatcher,
			sor<
				str_plus,
				str_minus,
				str_times,
				str_bitwise_and
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_arithmetic_operator(n); } };

		struct ShiftOperatorRule : RuleDispatcher,
			sor<
				str_lshift,
				str_rshift
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_shift_operator(n); } };

		struct ComparisonOperatorRule : RuleDispatcher,
			sor<
				str_le,
				str_lt,
				str_eq
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_comparison_operator(n); } };

		struct InstructionAssignmentRule : RuleDispatcher,
			seq<
				InexplicableWRule,
				SpacesRule,
				str_arrow,
				SpacesRule,
				InexplicableSRule
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_instruction_assignment(n); } };

		struct InstructionReturnRule : RuleDispatcher,
			seq<
				str_return
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_instruction_return(n); } };

		struct InstructionMemoryReadRule : RuleDispatcher,
			seq<
				InexplicableWRule,
				SpacesRule,
				str_arrow,
				SpacesRule,
				str_mem,
				SpacesRule,
				InexplicableXRule,
				SpacesRule,
				NumberRule
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_instruction_memory_read(n); } };

		/*

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };

		struct _Rule : RuleDispatcher
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_ DEFACOMPILERERROR(n); } };
		*/

		struct ProgramRule : RuleDispatcher,
			plus<
				seq<
					LineSeparatorsWithCommentsRule,
					bol,
					SpacesRule,
					sor<
						InstructionAssignmentRule,
						InstructionMemoryReadRule,
						InstructionReturnRule
					>,
					LineSeparatorsWithCommentsRule
				>
			>
		{ virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override { v.visit_program(n); } };

		struct EntryPointRule :	must<ProgramRule> {};

		template<typename Rule>
		struct Selector : pegtl::parse_tree::selector<
			Rule,
			pegtl::parse_tree::store_content::on<
				NameRule,
				NumberRule,
				InstructionAssignmentRule,
				InstructionReturnRule,
				InstructionMemoryReadRule,
				ProgramRule
			>
		> {};
	}

	struct ParseNode {
		// members
		std::vector<std::unique_ptr<ParseNode>> children;
		pegtl::internal::inputerator begin;
		pegtl::internal::inputerator end;
		std::unique_ptr<rules::RuleDispatcher> dispatcher;
		std::string type;// only used for displaying parse tree

		// special methods
		ParseNode() = default;
		ParseNode(const ParseNode &) = delete;
		ParseNode(ParseNode &&) = delete;
		ParseNode &operator=(const ParseNode &) = delete;
		ParseNode &operator=(ParseNode &&) = delete;
		~ParseNode() = default;

		// methods used for parsing

		template<typename Rule, typename ParseInput, typename... States>
		void start(const ParseInput &in, States &&...) {
			this->begin = in.inputerator();
		}

		template<typename Rule, typename ParseInput, typename... States>
		void success(const ParseInput &in, States &&...) {
			this->end = in.inputerator();
			this->dispatcher = std::make_unique<Rule>();
			this->type = typeid(this->dispatcher).name();
		}

		template<typename Rule, typename ParseInput, typename... States>
		void failure(const ParseInput &in, States &&...) {}

		template<typename... States>
		void emplace_back(std::unique_ptr<ParseNode> &&child, States &&...) {
			children.emplace_back(std::move(child));
		}

		std::string_view string_view() const {
			return {
				this->begin.data,
				static_cast<std::size_t>(this->end.data - this->begin.data)
			};
		}

		void accept_visitor(ParseNodeVisitor &v) {
			this->dispatcher->dispatch(v, *this);
		}

		std::unique_ptr<ParseNode> &operator[](int index) {
			return this->children.at(index);
		}

		std::vector<std::unique_ptr<ParseNode>> &get_children() {
			std::cout << "getting children";
			return this->children;
		}

		// methods used to display the parse tree

		bool has_content() const noexcept {
			return this->end.data != nullptr;
		}

		bool is_root() const noexcept {
			return static_cast<bool>(this->dispatcher);
		}
	};

	struct ParseTreeProcessor : ParseNodeVisitor {
		virtual void visit_name(ParseNode &x) override {
			std::cout << "parser is visiting a NAME\n";
		}
		virtual void visit_number(ParseNode &x) override {
			std::cout << "parser is visiting a NUMBER\n";
		}
		virtual void visit_program(ParseNode &x) override {
			std::cout << "parser is visiting ENTRY POINT\n";
			for (auto &child : x.get_children()) {
				child->accept_visitor(*this);
			}
			x[0]->accept_visitor(*this);
			x[2]->accept_visitor(*this);
			x[4]->accept_visitor(*this);
		}
	};


	// int f(parse_node *node) {
	// 	Visitor v;
	// 	node->accept(&v);
	// }

	std::unique_ptr<Program> parse_file(char *fileName, std::optional<std::string> parse_tree_output) {
		// Check the grammar for some possible issues.
		// TODO move this to a separate file bc it's performance-intensive
		if (pegtl::analyze<rules::EntryPointRule>() != 0) {
			std::cerr << "There are problems with the grammar" << std::endl;
			exit(1);
		}

		// Parse
		pegtl::file_input<> fileInput(fileName);
		auto root = pegtl::parse_tree::parse<rules::EntryPointRule, ParseNode, rules::Selector>(fileInput);
		if (root) {
			if (parse_tree_output.has_value()) {
				std::ofstream output_fstream(*parse_tree_output);
				if (output_fstream.is_open()) {
					pegtl::parse_tree::print_dot(output_fstream, *root);
					output_fstream.close();
				}
			}

			// ParseTreeProcessor processor;
			// root->children[0]->accept_visitor(processor);
		}
		return {};
	}
	std::unique_ptr<Program> parse_function_file(char *fileName) {
		exit(1);
	}
	std::unique_ptr<Program> parse_spill_file(char *fileName) {
		exit(1);
	}
}

