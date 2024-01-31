#pragma once
#include "program.h"

namespace L2::program::spiller {

    void spill(Function &function, Variable *var, Variable *prefix);
    void printDaSpiller();
}