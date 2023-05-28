#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "opcodes.h"
//6502
//A: Accumulator
//X: Register
//Y: Register
//PC: Program Counter
//SP: Stack Pointer
//P: Status Register

//cpu method:
// 1 read byte at PC
// 2 -> index array to find addressing mode and cycle count
// 3 read any additional bytes
// 4 execute instruction
// 5 wait, count clock cycles, until complete



//CPU 6502, globals ah!

//Accumulator: A
uint8_t accumulator = 0x00;

//Registers: X, Y
uint8_t x_register = 0x00;
uint8_t y_register = 0x00;

//Program Counter: PC
uint16_t program_counter = 0x0000;

//Stack Pointer: SP
uint8_t stack_pointer = 0x00;

//Status Register: P
uint8_t status_register = 0x00;

//Flags:
//C carry bit flag
//Z zero flag
//I interrupt disable
//D decimal mode
//B break command
//U unused
//V overflow flag
//N negative flag

//related variables
uint8_t current_opcode = 0x00;
uint8_t cycles = 0x00;

//memory
uint8_t ram[64 * 1024];


//memory IO
void mem_write(uint16_t address, uint8_t data)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        ram[address] = data;
    }
}

uint8_t mem_read(uint16_t address, bool ReadOnly)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        return ram[address];
    }
}

//needs refactoring and testing
opcode get_opcode(uint8_t input) {
    for (int i = 0; i < sizeof(opcode_matrix) / sizeof(opcode_matrix[0]); i++) {
        if (opcode_matrix[i].name == input) {
            return opcode_matrix[i];
        }
    }
}

void clock()
{
    if (cycles == 0) {
        //get next opcode
        current_opcode = mem_read(program_counter, false);
        //increment program counter of course
        program_counter++;

        //check if more cycles need to be added
        

    }
    //decrement a cycle every clock cycle, we dont have to calculate every cycle as long as the clock is synced in the main function
    cycles--;
}

void reset()
{

}

void interrupt_request()
{

}

void non_maskable_interrupt_request()
{

}

void set_flag(uint8_t flag, bool value)
{
    status_register = value ? status_register | (1 << flag) : status_register & ~(1 << flag);
}
bool check_flag(uint8_t flag)
{
    return (status_register & (1 << flag)) > 0;
}

//initializes cpu
void initialize_cpu()
{
    //sets cpu stuff to zero and ram as well
    uint8_t accumulator = 0x00;
    uint8_t x_register = 0x00;
    uint8_t y_register = 0x00;

    uint16_t program_counter = 0x0000;
    uint8_t stack_pointer = 0x00;
    uint8_t status_register = 0x00;

    uint8_t current_opcode = 0x00;
    uint8_t cycles = 0x00;

    for (int i = 0; i < sizeof(ram); i++)
    {
        //make sure ram is zerod
        ram[i] = 0x00;
    }

}

int main(void)
{
    //cpu and ram
    initialize_cpu();
    return 0;

    //TODO: test the functions already written and fix them up. make cpu and bus global, move everthing into header files, and then start working on the addressing modes and opcodes, and write unit tests?
}