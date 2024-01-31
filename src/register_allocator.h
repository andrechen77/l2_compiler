#pragma once
#include "program.h"

namespace L2::program::analyze {
	std::vector<const Register *> create_register_color_table(RegisterScope &register_scope);
	void allocate_and_spill(L2Function &l2_function);
}