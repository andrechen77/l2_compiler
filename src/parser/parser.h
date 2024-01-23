#pragma once

#include "program/program.h"
#include <memory>
#include <optional>

namespace L2::parser {
	std::unique_ptr<L2::program::Program> parse_file(char *fileName, std::optional<std::string> parse_tree_output);
	std::unique_ptr<L2::program::Program> parse_function_file(char *fileName);
	std::unique_ptr<L2::program::Program> parse_spill_file(char *fileName);
}