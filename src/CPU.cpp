#include "CPU.h"

CPU::CPU() {
    this->accumulator = 0;
    this->x_register = 0;
    this->y_register = 0;
    this->program_counter = 0;
    this->stack_pointer = 0;
    this->status_register = 0;
}

CPU::~CPU() {
    
}

void CPU::reset() {
    
}