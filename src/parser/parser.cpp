#include "parser.h"
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

namespace pegtl = TAO_PEGTL_NAMESPACE;

namespace L2::parser {
	struct ParseNode;

	namespace rules {
		using namespace pegtl; // for convenience of reading the rules

		template<typename Result, typename Separator, typename...Rules>
		struct interleaved_impl;
		template<typename... Results, typename Separator, typename Rule0, typename... RulesRest>
		struct interleaved_impl<seq<Results...>, Separator, Rule0, RulesRest...> :
			interleaved_impl<seq<Results..., Rule0, Separator>, Separator, RulesRest...>
		{};
		template<typename... Results, typename Separator, typename Rule0>
		struct interleaved_impl<seq<Results...>, Separator, Rule0> {
			using type = seq<Results..., Rule0>;
		};
		template<typename Separator, typename... Rules>
		using interleaved = typename interleaved_impl<seq<>, Separator, Rules...>::type;

		struct str_return : TAO_PEGTL_STRING("return") {};
		struct str_arrow : TAO_PEGTL_STRING("\x3c-") {};
		struct str_rax : TAO_PEGTL_STRING("rax") {};
		struct str_rcx : TAO_PEGTL_STRING("rcx") {};
		struct str_rdx : TAO_PEGTL_STRING("rdx") {};
		struct str_rdi : TAO_PEGTL_STRING("rdi") {};
		struct str_rsi : TAO_PEGTL_STRING("rsi") {};
		struct str_r8 : TAO_PEGTL_STRING("r8") {};
		struct str_r9 : TAO_PEGTL_STRING("r9") {};
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
		struct str_stack_arg : TAO_PEGTL_STRING("stack-arg") {};
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

		struct NumberRule :
			sor<
				seq<
					opt<sor<one<'-'>, one<'+'>>>,
					range<'1', '9'>,
					star<digit>
				>,
				one<'0'>
			>
		{};

		struct ArgumentNumberRule :
			seq<NumberRule>
		{};

		struct LeaFactorRule :
			rematch<
				NumberRule,
				sor<one<'1'>, one<'2'>, one<'4'>, one<'8'>>
			>
		{};

		struct NameRule :
			ascii::identifier // the rules for L2 rules are the same as for C identifiers
		{};

		struct LabelRule :
			seq<one<':'>, NameRule>
		{};

		struct FunctionNameRule :
			seq<one<'@'>, NameRule>
		{};

		struct VariableRule :
			seq<one<'%'>, NameRule>
		{};

		struct RegisterRule :
			sor<
				str_rax,
				str_rcx,
				str_rdx,
				str_rdi,
				str_rsi,
				str_r8,
				str_r9,
				str_rsp
			>
		{};

		struct InexplicableSxRule :
			sor<
				rematch<
					RegisterRule,
					str_rcx
				>,
				VariableRule
			>
		{};

		struct InexplicableARule :
			sor<
				InexplicableSxRule,
				rematch<
					RegisterRule,
					sor<
						str_rdi,
						str_rsi,
						str_rdx,
						str_r8,
						str_r9
					>
				>
			>
		{};

		struct InexplicableWRule :
			sor<
				InexplicableARule,
				rematch<
					RegisterRule,
					str_rax
				>
			>
		{};

		struct InexplicableXRule :
			sor<
				InexplicableWRule,
				rematch<
					RegisterRule,
					str_rsp
				>
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

		struct ArithmeticOperatorRule :
			sor<
				str_plus,
				str_minus,
				str_times,
				str_bitwise_and
			>
		{};

		struct ShiftOperatorRule :
			sor<
				str_lshift,
				str_rshift
			>
		{};

		struct ComparisonOperatorRule :
			sor<
				str_le,
				str_lt,
				str_eq
			>
		{};

		struct InstructionAssignmentRule :
			seq<
				InexplicableWRule,
				SpacesRule,
				str_arrow,
				SpacesRule,
				InexplicableSRule
			>
		{};

		struct InstructionReturnRule :
			seq<
				str_return
			>
		{};

		struct InstructionMemoryReadRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				str_arrow,
				str_mem,
				InexplicableXRule,
				NumberRule
			>
		{};

		struct InstructionMemoryWriteRule :
			interleaved<
				SpacesRule,
				str_mem,
				InexplicableXRule,
				NumberRule,
				str_arrow,
				InexplicableSRule
			>
		{};

		struct InstructionArithmeticOperationRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				ArithmeticOperatorRule,
				InexplicableTRule
			>
		{};

		struct InstructionStackArgRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				str_arrow,
				str_stack_arg,
				NumberRule
			>
		{};

		struct InstructionShiftOperationRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				ShiftOperatorRule,
				sor<
					NumberRule,
					InexplicableSxRule
				>
			>
		{};

		struct InstructionPlusWriteMemoryRule :
			interleaved<
				SpacesRule,
				str_mem,
				InexplicableXRule,
				NumberRule,
				str_plus,
				InexplicableTRule
			>
		{};

		struct InstructionMinusWriteMemoryRule :
			interleaved<
				SpacesRule,
				str_mem,
				InexplicableXRule,
				NumberRule,
				str_minus,
				InexplicableTRule
			>
		{};

		struct InstructionPlusReadMemoryRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				str_plus,
				str_mem,
				InexplicableXRule,
				NumberRule
			>
		{};

		struct InstructionMinusReadMemoryRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				str_minus,
				str_mem,
				InexplicableXRule,
				NumberRule
			>
		{};

		struct InstructionAssignmentCompareRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				str_arrow,
				InexplicableTRule,
				ComparisonOperatorRule,
				InexplicableTRule

			>
		{};

		struct InstructionCJumpRule :
			interleaved<
				SpacesRule,
				str_cjump,
				InexplicableTRule,
				ComparisonOperatorRule,
				InexplicableTRule,
				LabelRule
			>
		{};

		struct InstructionLabelRule :
			interleaved<
				SpacesRule,
				LabelRule
			>
		{};

		struct InstructionGotoLabelRule :
			interleaved<
				SpacesRule,
				str_goto,
				LabelRule
			>
		{};

		struct InstructionFunctionCallRule :
			interleaved<
				SpacesRule,
				str_call,
				InexplicableURule,
				NumberRule
			>
		{};

		struct StdFunctionNameRule :
			sor<
				str_print,
				str_input,
				str_allocate,
				str_tuple_error,
				str_tensor_error
			>
		{};

		struct InstructionStdCallRule :
			interleaved<
				SpacesRule,
				str_call,
				StdFunctionNameRule,
				NumberRule
			>
		{};

		struct InstructionIncrementRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				seq<one<'+'>, one<'+'>>
			>
		{};

		struct InstructionDecrementRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				seq<one<'-'>, one<'-'>>
			>
		{};

		struct InstructionLeaRule :
			interleaved<
				SpacesRule,
				InexplicableWRule,
				one<'@'>,
				InexplicableWRule,
				InexplicableWRule,
				LeaFactorRule
			>
		{};

		struct InstructionRule :
			sor<
				InstructionLeaRule,
				InstructionAssignmentCompareRule,
				InstructionAssignmentRule,
				InstructionReturnRule,
				InstructionMemoryReadRule,
				InstructionMemoryWriteRule,
				InstructionArithmeticOperationRule,
				InstructionStackArgRule,
				InstructionShiftOperationRule,
				InstructionPlusWriteMemoryRule,
				InstructionMinusWriteMemoryRule,
				InstructionPlusReadMemoryRule,
				InstructionMinusReadMemoryRule,
				InstructionCJumpRule,
				InstructionLabelRule,
				InstructionGotoLabelRule,
				InstructionFunctionCallRule,
				InstructionStdCallRule,
				InstructionIncrementRule,
				InstructionDecrementRule
			>
		{};

		struct InstructionsRule :
			plus<
				seq<
					LineSeparatorsWithCommentsRule,
					bol,
					SpacesRule,
					InstructionRule,
					LineSeparatorsWithCommentsRule
				>
			>
		{};

		struct FunctionRule :
			interleaved<
				LineSeparatorsWithCommentsRule,
				seq<SpacesRule, one<'('>>,
				seq<SpacesRule, FunctionNameRule>,
				seq<SpacesRule, ArgumentNumberRule>,
				InstructionsRule,
				seq<SpacesRule, one<')'>>
			>
		{};

		struct FunctionsRule :
			list<
				FunctionRule,
				LineSeparatorsWithCommentsRule
			>
		{};

		struct ProgramRule :
			seq<
				LineSeparatorsWithCommentsRule,
				interleaved<
					LineSeparatorsWithCommentsRule,
					seq<SpacesRule, one<'('>>,
					FunctionNameRule,
					FunctionsRule,
					seq<SpacesRule, one<')'>>
				>,
				LineSeparatorsWithCommentsRule
			>
		{};

		struct EntryPointRule :	must<ProgramRule> {};

		template<typename Rule>
		struct Selector : pegtl::parse_tree::selector<
			Rule,
			pegtl::parse_tree::store_content::on<
				NumberRule,
				NameRule,
				LabelRule,
				FunctionNameRule,
				VariableRule,
				RegisterRule,
				StdFunctionNameRule,
				ArithmeticOperatorRule,
				ShiftOperatorRule,
				ComparisonOperatorRule,
				InstructionAssignmentCompareRule,
				InstructionAssignmentRule,
				InstructionReturnRule,
				InstructionMemoryReadRule,
				InstructionMemoryWriteRule,
				InstructionArithmeticOperationRule,
				InstructionStackArgRule,
				InstructionShiftOperationRule,
				InstructionPlusWriteMemoryRule,
				InstructionMinusWriteMemoryRule,
				InstructionPlusReadMemoryRule,
				InstructionMinusReadMemoryRule,
				InstructionCJumpRule,
				InstructionLabelRule,
				InstructionGotoLabelRule,
				InstructionFunctionCallRule,
				InstructionStdCallRule,
				InstructionIncrementRule,
				InstructionDecrementRule,
				InstructionLeaRule,
				InstructionsRule,
				FunctionRule,
				FunctionsRule,
				ProgramRule
			>
		> {};
	}

	struct ParseNode {
		// members
		std::vector<std::unique_ptr<ParseNode>> children;
		pegtl::internal::inputerator begin;
		pegtl::internal::inputerator end;
		const std::type_info *rule; // which rule this node matched on
		std::string_view type;// only used for displaying parse tree

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
			this->rule = &typeid(Rule);
			this->type = pegtl::demangle<Rule>();
			this->type.remove_prefix(this->type.find_last_of(':') + 1);
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

		std::unique_ptr<ParseNode> &operator[](int index) {
			return this->children.at(index);
		}

		// methods used to display the parse tree

		bool has_content() const noexcept {
			return this->end.data != nullptr;
		}

		bool is_root() const noexcept {
			return static_cast<bool>(this->rule);
		}
	};

	std::unique_ptr<L2::program::Program> parse_file(char *fileName, std::optional<std::string> parse_tree_output) {
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

			std::cout << "the first node is of type " << root->children[0]->rule->name() << "\n";
		}
		return {};
	}
	std::unique_ptr<L2::program::Program> parse_function_file(char *fileName) {
		exit(1);
	}
	std::unique_ptr<L2::program::Program> parse_spill_file(char *fileName) {
		exit(1);
	}
}

