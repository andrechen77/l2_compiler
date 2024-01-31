#pragma once
#include "program.h"

namespace L2::program::spiller {

    void spill(L2Function &function, const Variable *var, std::string prefix, int spill_calls=0);
	void spill_all(L2Function &function, std::string prefix);
    std::string printDaSpiller(L2Function &function, int spill_calls=1);
}