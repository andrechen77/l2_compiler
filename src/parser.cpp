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

using namespace pegtl;

namespace L2 {
	// TODO add comment explaining the visitor-with-dispatcher pattern

	struct ParseNode;

	struct ParseNodeVisitor {
		virtual void visit_name(ParseNode &n) = 0;
		virtual void visit_number(ParseNode &n) = 0;
		virtual void visit_program(ParseNode &n) = 0;
	};

	struct GrammarRule {
		GrammarRule() {
			std::cout << "constructed grammar_rule\n";
		}

		virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) = 0;
	};

	struct NameRule : GrammarRule, one<'#'> {
		NameRule() {
			std::cout << "constructed name\n";
		}

		virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override {
			v.visit_name(n);
		}
	};

	struct NumberRule: GrammarRule, one<'1'> {
		NumberRule() {
			std::cout << "constructed number\n";
		}

		virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override {
			v.visit_number(n);
		}
	};

	struct ProgramRule : GrammarRule, star<
		sor<
			NameRule,
			NumberRule
		>
	> {
		ProgramRule() {
			std::cout << "constructed program\n";
		}

		virtual void dispatch(ParseNodeVisitor &v, ParseNode &n) override {
			v.visit_program(n);
		}
	};

	struct EntryPointRule : must<ProgramRule> {};

	struct ParseNode /* : pegtl::parse_tree::node */ {
		// members
		std::vector<std::unique_ptr<ParseNode>> children;
		pegtl::internal::inputerator begin;
		pegtl::internal::inputerator end;
		std::unique_ptr<GrammarRule> dispatcher;
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

		// methods used to display the parse tree

		bool has_content() const noexcept {
			return this->end.data != nullptr;
		}

		bool is_root() const noexcept {
			return static_cast<bool>(this->dispatcher);
		}
	};

	template<typename Rule>
	struct Selector : pegtl::parse_tree::selector<
		Rule,
		pegtl::parse_tree::store_content::on<
			NameRule,
			NumberRule,
			ProgramRule
		>
	> {};

	struct ParseTreeProcessor : ParseNodeVisitor {
		virtual void visit_name(ParseNode &x) override {
			std::cout << "parser is visiting a NAME\n";
		}
		virtual void visit_number(ParseNode &x) override {
			std::cout << "parser is visiting a NUMBER\n";
		}
		virtual void visit_program(ParseNode &x) override {
			std::cout << "parser is visiting ENTRY POINT\n";
		}
	};


	// int f(parse_node *node) {
	// 	Visitor v;
	// 	node->accept(&v);
	// }

	std::unique_ptr<Program> parse_file(char *fileName, std::optional<std::string> parse_tree_output) {
		// Check the grammar for some possible issues.
		// TODO move this to a separate file bc it's performance-intensive
		if (pegtl::analyze<EntryPointRule>() != 0) {
			std::cerr << "There are problems with the grammar" << std::endl;
			exit(1);
		}

		// Parse
		file_input<> fileInput(fileName);
		auto root = pegtl::parse_tree::parse<EntryPointRule, ParseNode, Selector>(fileInput);
		if (root) {
			if (parse_tree_output.has_value()) {
				std::ofstream output_fstream(*parse_tree_output);
				if (output_fstream.is_open()) {
					parse_tree::print_dot(output_fstream, *root);
					output_fstream.close();
				}
			}

			ParseTreeProcessor processor;
			root->children[0]->accept_visitor(processor);
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

