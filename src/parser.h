#pragma once

#include <L2.h>
#include <memory>

namespace L2 {
	std::unique_ptr<Program> parse_file(char *fileName, bool show_parse_tree);
	std::unique_ptr<Program> parse_function_file(char *fileName);
	std::unique_ptr<Program> parse_spill_file(char *fileName);
}