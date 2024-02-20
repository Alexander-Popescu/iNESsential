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

    void setFlag(uint8_t flag, bool value);
    

private:
    CpuState state;
};