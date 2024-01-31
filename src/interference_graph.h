#pragma once
#include "program.h"
#include "liveness.h"
#include "utils.h"
#include <assert.h>
#include <map>
#include <vector>
#include <tuple>
#include <algorithm>
#include <iterator>

namespace L2::program::analyze {

	// does not explicitly prohibit self-edges, but if a node has a color, then
	// it cannot connect to itself
	template<typename Node>
	class ColoringGraph {
		public:

		using Edge = bool;
		using Color = int;

		private:

		struct NodeInfo {
			Node node;
			std::vector<std::size_t> adj_vec;
			std::optional<Color> color;
			bool is_enabled = false;
		};

		std::map<Node, std::size_t> node_map;
		std::vector<NodeInfo> data;

		public:

		ColoringGraph(std::vector<Node> nodes) : node_map {}, data {} {
			this->data.resize(nodes.size());
			for (std::size_t i = 0; i < nodes.size(); ++i) {
				this->node_map.insert(std::make_pair(nodes[i], i));
				this->data[i].node = nodes[i];
			}
		}

		std::optional<Color> get_color(Node node) const {
			std::size_t u = this->node_map.at(node);
			return this->data[u].color;
		}

		// Checks whether two nodes conflict. Both must be enabled for them to
		// conflict.
		bool check_color_conflict(Node node_a, Node node_b) const {
			std::size_t u = this->node_map.at(node_a);
			std::size_t v = this->node_map.at(node_b);
			return this->check_color_conflict(u, v);
		}
		bool check_color_conflict(std::size_t u, std::size_t v) {
			auto &u_info = this->data[u];
			auto &v_info = this->data[v];
			return u_info.is_enabled && v_info.is_enabled
				&& u_info.color && v_info.color
				&& *u_info.color == v_info.color;
		}

		// Checks whether a node conflicts with any of its enabled neighbors.
		bool check_color_conflict(Node node) const {
			std::size_t u = this->node_map.at(node);
			return this->check_color_conflict(u);
		}
		bool check_color_conflict(std::size_t u) {
			if (!this->data[u].is_enabled) {
				return false;
			}

			for (std::size_t v : this->data[u].adj_vec) {
				if (this->check_color_conflict(v, u)) {
					return true;
				}
			}
			return false;
		}

		void add_edge(Node node_a, Node node_b) {
			std::size_t u = this->node_map.at(node_a);
			std::size_t v = this->node_map.at(node_b);
			return this->add_edge(u, v);
		}
		void add_edge(std::size_t u, std::size_t v) {
			if (this->check_color_conflict(u, v)) {
				std::cerr << "Cannot add an edge between two nodes of the same color\n";
				exit(1);
			}
			const auto [ui, uj] = std::equal_range(
				this->data[u].adj_vec.begin(),
				this->data[u].adj_vec.end(),
				v
			);
			if (ui == uj) {
				// u's adj list does not have v
				this->data[u].adj_vec.insert(ui, v);

				// by symmetry, v's adj list must not have u
				if (u != v) {
					const auto [vi, vj] = std::equal_range(
						this->data[v].adj_vec.begin(),
						this->data[v].adj_vec.end(),
						u
					);
					assert(vi == vj);
					this->data[v].adj_vec.insert(vi, u);
				}
			}
		}

		void add_clique(const utils::set<Node> &nodes) {
			for (auto it_a = nodes.begin(); it_a != nodes.end(); ++it_a) {
				auto it_b = it_a;
				++it_b;
				for (; it_b != nodes.end(); ++it_b) {
					this->add_edge(*it_a, *it_b);
				}
			}
		}

		// adds all possible edges between a node in group_a and a node in group_b
		// avoids adding self-edges
		void add_total_bipartite(const utils::set<Node> &group_a, const utils::set<Node> &group_b) {
			for (Node node_a : group_a) {
				for (Node node_b : group_b) {
					if (node_a != node_b) {
						this->add_edge(node_a, node_b);
					}
				}
			}
		}

		std::string to_string() const {
			std::string result;
			for (const NodeInfo &node_info : this->data) {
				result += node_info.node->to_string() + " ";
				for (std::size_t neighbor_index : node_info.adj_vec) {
					result += this->data[neighbor_index].node->to_string() + " ";
				}
				result += "\n";
			}
			return result;
		}
	};

	ColoringGraph<const Variable *> generate_interference_graph(
		const L2Function *l2_function,
		const InstructionsAnalysisResult &inst_analysis
	);
}