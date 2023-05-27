#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
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
    uint8_t a; //accumulator
    uint8_t x; //x register
    uint8_t y; //y register
    uint8_t p; //status flags
    uint8_t sp; //stack pointer
    uint16_t pc; //program counter
} cpu;

//P register flags:
//C carry bit flag
//Z zero flag
//I interrupt disable
//D decimal mode
//B break command
//U unused
//V overflow flag
//N negative flag

void set_flag(cpu *nes_cpu, uint8_t flag, bool value)
{
    nes_cpu->p = value ? nes_cpu->p | (1 << flag) : nes_cpu->p & ~(1 << flag);
}
bool check_flag(cpu *nes_cpu, uint8_t flag)
{
    return (nes_cpu->p & (1 << flag)) > 0;
}



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
    cpu *nes_cpu = malloc(sizeof(cpu));
    //create bus
    bus *nes_bus = malloc(sizeof(bus));
    initialize_cpu(nes_cpu);
    initialize_bus(nes_bus);
    printf("%i", check_flag(nes_cpu, 0));
    return 0;
}