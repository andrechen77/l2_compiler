#pragma once

#include <L2.h>
#include <memory>

namespace L2::parser {
	std::unique_ptr<Program> parse_file(char *fileName, std::optional<std::string> parse_tree_output);
	std::unique_ptr<Program> parse_function_file(char *fileName);
	std::unique_ptr<Program> parse_spill_file(char *fileName);
}