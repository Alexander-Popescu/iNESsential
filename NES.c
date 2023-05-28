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



//CPU 6502



typedef struct
{
    //registers

    uint8_t a; //accumulator
    uint8_t x; //x register
    uint8_t y; //y register
    uint8_t p; //status flags
    uint8_t sp; //stack pointer
    uint16_t pc; //program counter

    //addressing modes in addressing_modes.c

    //opcodes in opcodes.c

    //useful variables
    uint8_t fetched; //fetched data
    uint16_t absolute_address; //absolute address
    uint16_t relative_address; //relative address
    uint8_t opcode; //current opcode
    uint8_t cycles; //current number of cycles
} cpu;

//Bus of NES



typedef struct
{
    uint8_t ram[64 * 1024];
} bus;

void bus_write(bus *nes_bus, uint16_t address, uint8_t data)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        nes_bus->ram[address] = data;
    }
}

uint8_t bus_read(bus *nes_bus, uint16_t address, bool ReadOnly)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        return nes_bus->ram[address];
    }
}


//P register flags:
//C carry bit flag
//Z zero flag
//I interrupt disable
//D decimal mode
//B break command
//U unused
//V overflow flag
//N negative flag

//special cpu related functions

opcode get_opcode(uint8_t input) {
    for (int i = 0; i < sizeof(opcode_matrix) / sizeof(opcode_matrix[0]); i++) {
        if (opcode_matrix[i].name == input) {
            return opcode_matrix[i];
        }
    }
}

void clock(cpu *nes_cpu)
{
    if (nes_cpu->cycles == 0) {
        nes_cpu->opcode = bus_read(nes_bus, nes_cpu->pc, false);
        nes_cpu->pc++;
        
        
        uint8_t additional_cycle1 = get_opcode(nes_cpu->opcode).addressing_mode();
        
        uint8_t additional_cycle2 = get_opcode(nes_cpu->opcode).opcode_function();

        //add extra cycles if necessary
        nes_cpu->cycles += (additional_cycle1 & additional_cycle2);

    }
    nes_cpu->cycles--;
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

void set_flag(cpu *nes_cpu, uint8_t flag, bool value)
{
    nes_cpu->p = value ? nes_cpu->p | (1 << flag) : nes_cpu->p & ~(1 << flag);
}
bool check_flag(cpu *nes_cpu, uint8_t flag)
{
    return (nes_cpu->p & (1 << flag)) > 0;
}

//initializes cpu
void initialize_cpu(cpu *nes_cpu)
{
    //set registers to 0
    nes_cpu->a = 0x00;
    nes_cpu->x = 0x00;
    nes_cpu->y = 0x00;
    nes_cpu->p = 0b00000000;
    nes_cpu->sp = 0x00;
    nes_cpu->pc = 0x0000;

    //set other variables to defaults
    nes_cpu->fetched = 0x00;
    nes_cpu->absolute_address = 0x0000;
    nes_cpu->relative_address = 0x00;
    nes_cpu->opcode = 0x00;
    nes_cpu->cycles = 0;

}

void initialize_bus(bus *nes_bus)
{
    for (int i = 0; i < sizeof(nes_bus->ram); i++)
    {
        //make sure ram is zerod
        nes_bus->ram[i] = 0x00;
    }
}

int main(void)
{
    //create cpu
    cpu nes_cpu;
    initialize_cpu(&nes_cpu);

    //create bus
    bus nes_bus;
    initialize_bus(&nes_bus);
    return 0;

    //TODO: test the functions already written and fix them up. make cpu and bus global, move everthing into header files, and then start working on the addressing modes and opcodes
}