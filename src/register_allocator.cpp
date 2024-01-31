#include "register_allocator.h"
#include "interference_graph.h"

namespace L2::program::analyze {
	void allocate_and_spill(L2Function &l2_function) {
		InstructionsAnalysisResult liveness_results = analyze_instructions(l2_function);
		VariableGraph graph = generate_interference_graph(l2_function, liveness_results);
		std::vector<const Variable *> spills = attempt_color_graph(graph, 15); // TODO magic number

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