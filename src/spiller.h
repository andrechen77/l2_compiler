#pragma once
#include "program.h"

namespace L2::program::spiller {

    void spill(L2Function &function, Variable *var, std::string prefix, int spill_calls=0);
    std::string printDaSpiller(L2Function &function, int spill_calls=0);
}