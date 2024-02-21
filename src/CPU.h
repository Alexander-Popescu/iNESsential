// 6502 cpu
#pragma once
#include <cstdint>
#include <vector>
#include "Definitions.h"

class CPU {
public:
    CPU();
    ~CPU();

    void reset();
    CpuState *getState();
    int cycleCount = 0;

    void setFlag(uint8_t flag, bool value);
    

private:
    CpuState state;
};