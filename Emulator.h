// NES emulator
#pragma once
#include "SDL.h"
#include <cstdint>
#include <vector>

class Emulator {
public:
    Emulator();
    ~Emulator();

    int runUntilBreak();

private:
    //CPU

    //registers
    uint8_t accumulator;
    uint8_t x_register;
    uint8_t y_register;
    uint16_t program_counter;
    uint8_t stack_pointer;
    unsigned int status_register : 6;
}