// 6502 cpu
#pragma once
#include <cstdint>
#include <vector>

class CPU {
public:
    CPU();
    ~CPU();

    void reset();

private:
    //registers
    uint8_t accumulator;
    uint8_t x_register;
    uint8_t y_register;
    uint16_t program_counter;
    uint8_t stack_pointer;
    unsigned int status_register : 6;
};