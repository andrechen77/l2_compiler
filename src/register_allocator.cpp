#include "register_allocator.h"
#include "interference_graph.h"

namespace L2::program::analyze {
	std::vector<const Register *> create_register_color_table(RegisterScope &register_scope) {
		static const std::vector<std::string> register_order = {
			"rax", "rdi", "rsi", "rdx", "rcx",
			"r8", "r9", "r10", "r11", "r12",
			"r13", "r14", "r15", "rbx", "rbp"
		};

		std::vector<const Register *> color_table;
		for (const std::string &reg_name : register_order) {
			if (std::optional<Register *> reg = register_scope.get_item_maybe(reg_name); reg) {
				color_table.push_back(*reg);
			}
		}
		return color_table;
	}

	void allocate_and_spill(L2Function &l2_function) {
		std::vector<const Register *> register_color_table = create_register_color_table(l2_function.agg_scope.register_scope);
		// while (true) {
			InstructionsAnalysisResult liveness_results = analyze_instructions(l2_function);
			VariableGraph graph = generate_interference_graph(l2_function, liveness_results, register_color_table);
			std::vector<const Variable *> spills = attempt_color_graph(graph, register_color_table);
		// 	if (!spills.empty()) {
		// 		spiller::spill(l2_function, spills[spills.size() - 1], prefix);
		// 	}
		// }
		// Print the results
		for (const Variable *spill : spills) {
			std::cout << "spilling " << spill->to_string() << "\n";
		}
		for (const auto &[node, i] : graph.get_node_map()) {
			std::cout << node->to_string() << " got colored " << *graph.get_node_info(i).color << "\n";
		}
		std::cout << graph.to_string() << std::endl;
	}
}