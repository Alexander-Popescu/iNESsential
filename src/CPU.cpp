#include "CPU.h"
#include <stdio.h>

CPU::CPU() {
    this->state.accumulator = 0;
    this->state.x_register = 0;
    this->state.y_register = 0;
    //start at C000 for nestest log
    this->state.program_counter = 0xC000;
    this->state.stack_pointer = 0;
    this->state.status_register = 0;
}

CPU::~CPU() {
    
}

void CPU::reset() {
    printf(YELLOW "CPU: Reset\n" RESET);
}

CpuState *CPU::getState() {
    return &state;
}

void CPU::setFlag(uint8_t flag, bool value) {
    if (value) {
        state.status_register |= flag;
    } else {
        state.status_register &= flag;
    }
}