#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

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

uint8_t check_flag(uint8_t flag);
void set_flag(uint8_t flag, bool value);
void interrupt_request();
void print_cpu_state();

uint16_t instruction_count = 0;

//CPU 6502, globals ah!

//Accumulator: A
uint8_t accumulator = 0x00;

//Registers: X, Y
uint8_t x_register = 0x00;
uint8_t y_register = 0x00;

//Program Counter: PC
uint16_t program_counter = 0x8000;

//Stack Pointer: SP, descending stack
uint8_t stack_pointer = 0xFD;

//Status Register: P
uint8_t status_register = 0x00;

//Flags:
static uint8_t C_flag = 0b00000001; // C carry bit flag
static uint8_t Z_flag = 0b00000010; // Z zero flag
static uint8_t I_flag = 0b00000100; // I interrupt disable
static uint8_t D_flag = 0b00001000; // D decimal mode
static uint8_t B_flag = 0b00010000; // B break command
static uint8_t U_flag = 0b00100000; // U unused
static uint8_t V_flag = 0b01000000; // V overflow flag
static uint8_t N_flag = 0b10000000; // N negative flag

//related variables
uint8_t current_opcode = 0x00;
uint8_t cycles = 0x00;

//variable to link the addressing modes with the opcodes
uint8_t data_at_absolute = 0x00;

bool accumulator_mode = false;

uint16_t absolute_address = 0x0000;
uint16_t relative_address = 0x0000;


uint8_t cpuBus[64 * 1024];
//CPU BUS SPECIFICATIONS:

//0x0000-0x07FF: 2KB Ram
//0x0800-0x0FFF: Mirrors of 0x0000-0x07FF
//0x1000-0x17FF: Mirrors of 0x0000-0x07FF
//0x1800-0x1FFF: Mirrors of 0x0000-0x07FF
//0x2000-0x2007: PPU registers
//0x2008-0x3FFF: Mirrors of PPU registers
//0x4000-0x4017: APU and I/O registers
//0x4018-0x401F: APU and I/O functionality that is normally disabled
//0x4020-0xFFFF: Cartridge space: PRG ROM, PRG RAM, and mapper registers


//for debug output
FILE* fp;

//total cycles elapsed
uint64_t total_cycles = 0;

// PPU arrays
uint8_t ppuBus[0x4000] = {0};
//PPU BUS SPECIFICATIONS:

//0x0000-0x0FFF: Pattern table 0
//0x1000-0x1FFF: Pattern table 1
//0x2000-0x23FF: Nametable 0
//0x2400-0x27FF: Nametable 1
//0x2800-0x2BFF: Nametable 2
//0x2C00-0x2FFF: Nametable 3
//0x3000-0x3EFF: Mirrors of 0x2000-0x2EFF
//0x3F00-0x3F1F: Palette RAM indexes
//0x3F20-0x3FFF: Mirrors of 0x3F00-0x3F1F


//32 color palette
uint8_t ppu_palette[0x20] = {0};

//256byte object attribute memory
uint8_t ppu_oam[0x100] = {0};

// PPU registers
uint8_t ppu_ctrl = 0x00;
uint8_t ppu_mask = 0x00;
uint8_t ppu_status = 0x00;
uint8_t oam_addr = 0x00;
uint8_t oam_data = 0x00;
uint8_t ppu_scroll = 0x00;
uint8_t ppu_addr = 0x00;
uint8_t ppu_data = 0x00;
uint8_t oam_dma = 0x00;

//PPU helpers
uint8_t address_latch = 0x00;
uint8_t ppu_data_buffer = 0x00;
uint16_t ppu_temp_address = 0x0000;


//SDL globals
#define WIDTH 256
#define HEIGHT 240

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
uint8_t r[WIDTH * HEIGHT];
uint8_t g[WIDTH * HEIGHT];
uint8_t b[WIDTH * HEIGHT];

//PPU io

//0x0000-0x0FFF: Pattern table 0
//0x1000-0x1FFF: Pattern table 1
//0x2000-0x23FF: Nametable 0
//0x2400-0x27FF: Nametable 1
//0x2800-0x2BFF: Nametable 2
//0x2C00-0x2FFF: Nametable 3
//0x3000-0x3EFF: Mirrors of 0x2000-0x2EFF
//0x3F00-0x3F1F: Palette RAM indexes
//0x3F20-0x3FFF: Mirrors of 0x3F00-0x3F1F

void ppuBus_write(uint16_t address, uint8_t data)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0x3FFF))
    {
        ppuBus[address] = data;
    }
    else
    {
        printf("Invalid memory address: %d", address);
    }
}

uint8_t ppuBus_read(uint16_t address)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0x3FFF))
    {
        return ppuBus[address];
    }
}

//memory IO
void cpuBus_write(uint16_t address, uint8_t data)
{
    // Mirroring for PPU registers
    if (address >= 0x2000 && address <= 0x3FFF) {
        address = 0x2000 + (address % 8);
    }

    // Mirroring for RAM
    if (address >= 0x0800 && address <= 0x1FFF) {
        address = address % 0x0800;
    }

    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        cpuBus[address] = data;
    }
    else
    {
        printf("Invalid memory address: %d", address);
    }
}

uint8_t cpuBus_read(uint16_t address)
{
    // Mirroring for PPU registers
    if (address >= 0x2000 && address <= 0x3FFF) {
        address = 0x2000 + (address % 8);
    }

    // Mirroring for RAM
    if (address >= 0x0800 && address <= 0x1FFF) {
        address = address % 0x0800;
    }

    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        return cpuBus[address];
    }
}


//addressing modes
uint8_t IMP()
{
    //implied
    data_at_absolute = accumulator;
    return 0;
}

uint8_t IMM()
{
    //immediate
    absolute_address = program_counter++;
    return 0;
}

uint8_t ZP0()
{
    //zero page
    absolute_address = cpuBus_read(program_counter);
    program_counter++;
    absolute_address &= 0x00FF;
    return 0;
}

uint8_t ZPX()
{
    //zero page with offset from X register
    absolute_address = (cpuBus_read(program_counter) + x_register);
    program_counter++;
    absolute_address &= 0x00FF;
    return 0;
}

uint8_t ZPY()
{
    //zero page with offset from Y register
    absolute_address = (cpuBus_read(program_counter) + y_register);
    program_counter++;
    absolute_address &= 0x00FF;
    return 0;
}

uint8_t REL()
{
    //relative for branching instructions
    relative_address = cpuBus_read(program_counter);
    program_counter++;

    //check if negative
    if (relative_address & 0x80)
    {
        relative_address |= 0xFF00;
    }
    return 0;
}

uint8_t ABS()
{
    //absolute
    //read first byte
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    //second
    uint16_t high = cpuBus_read(program_counter) << 8;
    program_counter++;

    absolute_address = high | low;
    return 0;
}

uint8_t ABX()
{
    //absolute with offset, offsets by X register after bytes are read

    //read first byte
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    //second
    uint16_t high = cpuBus_read(program_counter) << 8;
    program_counter++;

    absolute_address = high | low;
    absolute_address += x_register;

    //this is one where if the memory page changes we need more cycles
    uint16_t first_byte = absolute_address & 0xFF00;

    return (first_byte != high) ? 1 : 0;
}

uint8_t ABY()
{
    //absolute with offset

    //read first byte
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    //second
    uint16_t high = cpuBus_read(program_counter) << 8;
    program_counter++;

    absolute_address = high | low;
    absolute_address += y_register;

    //this is one where if the memory page changes we need more cycles
    uint16_t first_byte = absolute_address & 0xFF00;

    return (first_byte != high) ? 1 : 0;

    return 0;
}

uint8_t IND()
{
    uint16_t low = cpuBus_read(program_counter);
    program_counter++;
    uint16_t high = cpuBus_read(program_counter);
    program_counter++;

    uint16_t address = (high << 8) | low;

    if (low == 0x00FF)
    {
        //page crossing
        absolute_address = (cpuBus_read(address & 0xFF00) << 8) | cpuBus_read(address);
    }
    else
    {
        //normal case
        absolute_address = (cpuBus_read(address + 1) << 8) | cpuBus_read(address);
    }

    return 0;
}

uint8_t IZX()
{

    uint16_t argument = cpuBus_read(program_counter);
    program_counter++;

    uint16_t low = cpuBus_read(((uint16_t)(argument + (uint16_t)x_register)) & 0x00FF);
    uint16_t high = cpuBus_read(((uint16_t)(argument + (uint16_t)x_register + 1)) & 0x00FF);

    absolute_address = (high << 8) | low;

    return 0;
}

uint8_t IZY()
{
    //indirect indexed
    uint16_t input = cpuBus_read(program_counter);
    program_counter++;

    //get actual address and offset final by y, but also check for page change

    uint16_t low = cpuBus_read(input & 0x00FF);
    uint16_t high = cpuBus_read((input + 1) & 0x00FF) << 8;

    absolute_address = high | low;
    absolute_address += y_register;

    //check for page change
    uint16_t first_byte = absolute_address & 0xFF00;
    
    return first_byte != high ? 1 : 0;
}

//helper function to avoid writing !IMP for every memory address
uint8_t update_absolute_data()
{
    if (accumulator_mode == true)
    {
        data_at_absolute = accumulator;
        return data_at_absolute;
    }
    data_at_absolute = cpuBus_read(absolute_address);
    return data_at_absolute;
}

uint8_t ACC()
{
    //accumulator
    accumulator_mode = true;
    return 0;
}

//opcode functions
uint8_t ADC()
{
    //addition
    update_absolute_data();

    uint16_t tmp = (uint16_t)accumulator + (uint16_t)data_at_absolute + (uint16_t)check_flag(C_flag);

    //conditions for flags to be set
    set_flag(C_flag, tmp > 255);
    set_flag(Z_flag, (tmp & 0x00FF) == 0);
    set_flag(N_flag, tmp & 0x80);
    set_flag(V_flag, (~(accumulator ^ data_at_absolute) & (accumulator ^ tmp)) & 0x0080);

    accumulator = tmp & 0x00FF;

    return 1;
}

uint8_t AND()
{
    //get new data
    update_absolute_data();

    //perform opcode
    accumulator = accumulator & data_at_absolute;

    //update flags
    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    //return 1 if extra clock cycle is possible, it will check with addressing mode function
    return 1;
}

uint8_t ASL()
{
    update_absolute_data();

    uint16_t tmp = (uint16_t)data_at_absolute << 1;

    set_flag(C_flag, (tmp & 0xFF00) > 0);
    set_flag(Z_flag, (tmp & 0x00FF) == 0x00);
    set_flag(N_flag, tmp & 0x80);

    if (accumulator_mode == true)
    {
        accumulator = tmp & 0x00FF;
    }
    else
    {
        cpuBus_write(absolute_address, tmp & 0x00FF);
    }
    accumulator_mode = false;

    return 0;
}

uint8_t BCC()
{
    if(check_flag(C_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        return 1;
    }
    return 1;
}

uint8_t BCS()
{
    if(check_flag(C_flag) == true)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;

        return 1;
    }
    return 1;
}

uint8_t BEQ()
{
    if(check_flag(Z_flag) == 1)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        return 1;
    }
    return 1;
}

uint8_t BIT()
{
    //get new data
    update_absolute_data();

    //perform opcode
    uint8_t tmp = accumulator & data_at_absolute;

    //update flags
    set_flag(Z_flag, tmp == 0x00);

    //set to data from memory locations
    set_flag(N_flag, data_at_absolute & 0x80);
    set_flag(V_flag, data_at_absolute & 0x40);

    return 0;
}

uint8_t BMI()
{
    if(check_flag(N_flag) == 1)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
    }
    return 1;
}

uint8_t BNE()
{
    if(check_flag(Z_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        return 1;
    }
    return 1;
}

uint8_t BPL()
{
    if(check_flag(N_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        return 1;
    }
    return 1;
}

uint8_t BRK()
{
    //break
    interrupt_request();

    //set break flag
    set_flag(B_flag, 1);

    return 0;
}

uint8_t BVC()
{
    if(check_flag(V_flag) == 0)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        return 1;
    }
    return 1;
}

uint8_t BVS()
{
    if(check_flag(V_flag) == 1)
    {
        //branch
        cycles++;
        absolute_address = program_counter + relative_address;

        //check if address changed page
        if ((absolute_address & 0xFF00) != (program_counter & 0xFF00))
        {
            cycles++;
        }
        //update program counter since we just moved 
        program_counter = absolute_address;
        return 1;
    }
    return 1;
}

uint8_t CLC()
{
    set_flag(C_flag, 0);
    return 0;
}

uint8_t CLD()
{
    set_flag(D_flag, 0);
    return 0;
}

uint8_t CLI()
{
    set_flag(I_flag, 0);
    return 0;
}

uint8_t CLV()
{
    set_flag(V_flag, 0);
    return 0;
}

uint8_t CMP()
{
    //get new data
    update_absolute_data();

    uint16_t temp = (uint16_t)accumulator - (uint16_t)data_at_absolute;

    set_flag(C_flag, accumulator >= data_at_absolute);
    set_flag(Z_flag, (temp & 0x00FF) == 0x0000);
    set_flag(N_flag, temp & 0x0080);

    return 1;
}

uint8_t CPX()
{
    //get new data
    update_absolute_data();

    if (x_register >= data_at_absolute)
    {
        set_flag(C_flag, 1);
    }
    else
    {
        set_flag(C_flag, 0);
    }

    if (x_register == data_at_absolute)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if ((x_register - data_at_absolute) & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 0;
}

uint8_t CPY()
{
    //get new data
    update_absolute_data();

    if (y_register >= data_at_absolute)
    {
        set_flag(C_flag, 1);
    }
    else
    {
        set_flag(C_flag, 0);
    }

    if (y_register == data_at_absolute)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if ((y_register - data_at_absolute) & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 0;
}

uint8_t DEC()
{
    update_absolute_data();
    cpuBus_write(absolute_address, data_at_absolute - 1);
    update_absolute_data();

    set_flag(Z_flag, data_at_absolute == 0x00);
    set_flag(N_flag, data_at_absolute & 0x80);

    return 0;
}

uint8_t DEX()
{
    x_register--;

    //set flags
    if (x_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (x_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    } 

    return 0;
}

uint8_t DEY()
{
    y_register--;

    //set flags
    if (y_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }
    
    return 0;
}

uint8_t EOR()
{
    update_absolute_data();
    accumulator = accumulator ^ data_at_absolute;

    //set flags
    if (accumulator == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (accumulator & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 1;
}

uint8_t INC()
{
    update_absolute_data();
    cpuBus_write(absolute_address, data_at_absolute + 1);
    update_absolute_data();

    set_flag(Z_flag, data_at_absolute == 0x00);
    set_flag(N_flag, data_at_absolute & 0x80);

    return 0;
}

uint8_t INX()
{
    x_register++;

    //set flags
    if (x_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (x_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 0;
}

uint8_t INY()
{
    y_register++;

    //set flags
    if (y_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 0;
}

uint8_t JMP()
{
    program_counter = absolute_address;
    return 0;
}

uint8_t JSR()
{

    program_counter--;
    uint16_t second_half = cpuBus_read(program_counter - 1);
    uint16_t first_half = cpuBus_read(program_counter) << 8;

    uint16_t target = first_half | second_half;

    //push program counter to stack
    cpuBus_write(0x0100 + stack_pointer, program_counter >> 8);
    stack_pointer--;
    cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
    stack_pointer--;

    //set program counter
    program_counter = target;

    return 0;
}

uint8_t LDA()
{
    update_absolute_data();
    accumulator = data_at_absolute;

    //set flags
    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    return 1;
}

uint8_t LDX()
{

    update_absolute_data();
    x_register = data_at_absolute;

    //set flags
    if (x_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (x_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 1;
}

uint8_t LDY()
{

    update_absolute_data();
    y_register = data_at_absolute;

    //set flags
    if (y_register == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 1;
}

uint8_t LSR()
{
    update_absolute_data();

    uint8_t temp = data_at_absolute >> 1;

    //set flags
    set_flag(C_flag, data_at_absolute & 0x01);
    set_flag(Z_flag, temp == 0x00);
    set_flag(N_flag, temp & 0x80);

    accumulator_mode ? accumulator = temp : cpuBus_write(absolute_address, temp);
    accumulator_mode = false;

    return 0;
}

uint8_t NOP()
{
    if ((current_opcode == 0x1C) || (current_opcode == 0x3C) || (current_opcode == 0x5C) || (current_opcode == 0x7C) || (current_opcode == 0xDC) || (current_opcode == 0xFC))
    {
        return 1;
    }
    {
        return 1;
    }
    return 0;
}

uint8_t ORA()
{

    update_absolute_data();
    accumulator = accumulator | data_at_absolute;

    //set flags
    if (accumulator == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (accumulator & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

    return 1;
}

uint8_t PHA()
{
    //stack is descending and fills the 1 page
    cpuBus_write(0x0100 + stack_pointer, accumulator);
    stack_pointer--;
    return 0;
}

uint8_t PHP()
{
    //flags must be set before push
    set_flag(B_flag, 1);
    cpuBus_write(0x0100 + stack_pointer, status_register); 
    set_flag(B_flag, 0);
    set_flag(U_flag, 1);
    stack_pointer--;

    return 0;
}

uint8_t PLA()
{
    stack_pointer++;
    accumulator = cpuBus_read(0x0100 + stack_pointer);
    if (accumulator == 0x00)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if (accumulator & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }
    return 0;
}

uint8_t PLP()
{
    
    stack_pointer++;
    status_register = cpuBus_read(0x0100 + stack_pointer);
    set_flag(U_flag, 1);
    set_flag(B_flag, 0);
    return 0;
}

uint8_t ROL()
{
    update_absolute_data();

    uint16_t tmp = (uint16_t)(data_at_absolute << 1) | check_flag(C_flag);

    //set flags
    set_flag(C_flag, tmp & 0xFF00);
    set_flag(Z_flag, (tmp & 0x00FF) == 0x0000);
    set_flag(N_flag, tmp & 0x0080);

    if (accumulator_mode == true)
    {
        accumulator = tmp & 0x00FF;
    }
    else
    {
        cpuBus_write(absolute_address, tmp & 0x00FF);
    }

    accumulator_mode = false;
    
    return 0;
}

uint8_t ROR()
{

    update_absolute_data();

    uint16_t tmp = (uint16_t)(data_at_absolute >> 1) | (check_flag(C_flag) << 7);

    //set flags
    set_flag(C_flag, data_at_absolute & 0x01);
    set_flag(Z_flag, tmp == 0x00);
    set_flag(N_flag, tmp & 0x80);

    if (accumulator_mode == true)
    {
        accumulator = tmp & 0x00FF;
    }
    else
    {
        cpuBus_write(absolute_address, tmp & 0x00FF);
    }

    accumulator_mode = false;

    return 0;
}

uint8_t RTI()
{
    //returns from an inturrupt

    //read status
    stack_pointer++;
    status_register = cpuBus_read(0x0100 + stack_pointer);
    set_flag(U_flag, 1);
    set_flag(B_flag, 0);

    stack_pointer++;
    uint16_t second_half = (uint16_t)cpuBus_read(0x0100 + stack_pointer);
    stack_pointer++;
    uint16_t first_half = cpuBus_read(0x0100 + stack_pointer) << 8;

    program_counter = first_half | second_half;
    return 0;
}

uint8_t RTS()
{

    stack_pointer++;
    program_counter = cpuBus_read(0x0100 + stack_pointer);
    stack_pointer++;
    program_counter = program_counter | ((uint16_t)cpuBus_read(0x0100 + stack_pointer) << 8);
    //go to next instruction
    program_counter++;

    return 0;
}

uint8_t SBC()
{
    //16bit because of overflow
    update_absolute_data();

    uint16_t tmp = (uint16_t)accumulator + ((uint16_t)data_at_absolute ^ 0x00FF) + (uint16_t)check_flag(C_flag);

    set_flag(C_flag, tmp & 0xFF00);
    set_flag(Z_flag, (tmp & 0x00FF) == 0);
    set_flag(N_flag, tmp & 0x80);
    set_flag(V_flag, (tmp ^ (uint16_t)accumulator) & (tmp ^ ((uint16_t)data_at_absolute ^ 0x00FF)) & 0x0080);

    accumulator = tmp & 0x00FF;

    return 1;
}

uint8_t SEC()
{
    set_flag(C_flag, true);
    return 0;
}

uint8_t SED()
{
    set_flag(D_flag, true);
    return 0;
}

uint8_t SEI()
{
    set_flag(I_flag, true);
    return 0;
}

uint8_t STA()
{
    update_absolute_data();
    

    cpuBus_write(absolute_address, accumulator);

    return 0;
}

uint8_t STX()
{
    cpuBus_write(absolute_address, x_register);
    //absolute addressing
    return 0;
}

uint8_t STY()
{
    cpuBus_write(absolute_address, y_register);
    return 0;
}

uint8_t TAX()
{
    x_register = accumulator;

    set_flag(Z_flag, x_register == 0x00);
    set_flag(N_flag, x_register & 0x80);
    return 0;
}

uint8_t TAY()
{
    y_register = accumulator;

    set_flag(Z_flag, y_register == 0x00);
    set_flag(N_flag, y_register & 0x80);
    return 0;
}

uint8_t TSX()
{
    x_register = stack_pointer;

    set_flag(Z_flag, x_register == 0x00);
    set_flag(N_flag, x_register & 0x80);

    return 0;
}

uint8_t TXA()
{
    accumulator = x_register;

    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);

    return 0;
}

uint8_t TXS()
{
    stack_pointer = x_register;
    return 0;
}

uint8_t TYA()
{
    accumulator = y_register;

    set_flag(Z_flag, accumulator == 0x00);
    set_flag(N_flag, accumulator & 0x80);
    return 0;
}

uint8_t XXX()
{
    printf("\nINVALID OPCODE\n");
    printf("opcode: %02X\n", current_opcode);
    return 0;
}

//illegal opcodes

uint8_t LAX()
{
    LDA();
    LDX();
    if ((current_opcode == 0xB3 || (current_opcode == 0xB7)))
    {
        return 1;
    }
    return 0;
}

uint8_t SAX()
{
    cpuBus_write(absolute_address, accumulator & x_register);
    return 0;
}

uint8_t DCP()
{
    DEC();
    CMP();
    return 0;
}

uint8_t ISB()
{
    INC();
    SBC();
    return 0;
}

uint8_t SLO()
{
    ASL();
    ORA();
    return 0;
}

uint8_t RLA()
{
    ROL();
    AND();
    return 0;
}

uint8_t SRE()
{
    LSR();
    EOR();
    return 0;
}

uint8_t RRA()
{
    ROR();
    ADC();
    return 0;
}

//opcode matrix stuff

typedef struct opcode
{
    char *name;
    uint8_t (*opcode)(void);
    uint8_t (*addressing_mode)(void);
    uint8_t byte_size;
    uint8_t cycle_count;
} opcode;

opcode opcode_matrix[16][16] = {
//   0                         1                         2                         3                         4                         5                         6                         7                         8                         9                         A                         B                         C                         D                         E                         F
    {{ "BRK", BRK, IMP, 1, 7 },{"ORA", ORA, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SLO", SLO, IZX, 2, 8 },{"NOP", NOP, ZP0, 0, 3 },{"ORA", ORA, ZP0, 2, 3 },{"ASL", ASL, ZP0, 2, 5 },{"SLO", SLO, ZP0, 2, 5 },{"PHP", PHP, IMP, 1, 3 },{"ORA", ORA, IMM, 2, 2 },{"ASL", ASL, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"NOP", NOP, ABS, 3, 4 },{"ORA", ORA, ABS, 3, 4 },{"ASL", ASL, ABS, 3, 6 },{"SLO", SLO, ABS, 3, 6 }},
    {{ "BPL", BPL, REL, 2, 2 },{"ORA", ORA, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"SLO", SLO, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"ORA", ORA, ZPX, 2, 4 },{"ASL", ASL, ZPX, 2, 6 },{"SLO", SLO, ZPX, 2, 6 },{"CLC", CLC, IMP, 1, 2 },{"ORA", ORA, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"SLO", SLO, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"ORA", ORA, ABX, 3, 4 },{"ASL", ASL, ABX, 3, 7 },{"SLO", SLO, ABX, 3, 7 }},
    {{ "JSR", JSR, ABS, 3, 6 },{"AND", AND, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"RLA", RLA, IZX, 2, 8 },{"BIT", BIT, ZP0, 2, 3 },{"AND", AND, ZP0, 2, 3 },{"ROL", ROL, ZP0, 2, 5 },{"RLA", RLA, ZP0, 2, 5 },{"PLP", PLP, IMP, 1, 4 },{"AND", AND, IMM, 2, 2 },{"ROL", ROL, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"BIT", BIT, ABS, 3, 4 },{"AND", AND, ABS, 3, 4 },{"ROL", ROL, ABS, 3, 6 },{"RLA", RLA, ABS, 3, 6 }},
    {{ "BMI", BMI, REL, 2, 2 },{"AND", AND, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"RLA", RLA, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"AND", AND, ZPX, 2, 4 },{"ROL", ROL, ZPX, 2, 6 },{"RLA", RLA, ZPX, 2, 6 },{"SEC", SEC, IMP, 1, 2 },{"AND", AND, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"RLA", RLA, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"AND", AND, ABX, 3, 4 },{"ROL", ROL, ABX, 3, 7 },{"RLA", RLA, ABX, 3, 7 }},
    {{ "RTI", RTI, IMP, 1, 6 },{"EOR", EOR, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SRE", SRE, IZX, 2, 8 },{"NOP", NOP, ZP0, 2, 3 },{"EOR", EOR, ZP0, 2, 3 },{"LSR", LSR, ZP0, 2, 5 },{"SRE", SRE, ZP0, 2, 5 },{"PHA", PHA, IMP, 1, 3 },{"EOR", EOR, IMM, 2, 2 },{"LSR", LSR, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"JMP", JMP, ABS, 3, 3 },{"EOR", EOR, ABS, 3, 4 },{"LSR", LSR, ABS, 3, 6 },{"SRE", SRE, ABS, 3, 6 }},
    {{ "BVC", BVC, REL, 2, 2 },{"EOR", EOR, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"SRE", SRE, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"EOR", EOR, ZPX, 2, 4 },{"LSR", LSR, ZPX, 2, 6 },{"SRE", SRE, ZPX, 2, 6 },{"CLI", CLI, IMP, 1, 2 },{"EOR", EOR, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"SRE", SRE, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"EOR", EOR, ABX, 3, 4 },{"LSR", LSR, ABX, 3, 7 },{"SRE", SRE, ABX, 3, 7 }},
    {{ "RTS", RTS, IMP, 1, 6 },{"ADC", ADC, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"RRA", RRA, IZX, 2, 8 },{"NOP", NOP, ZP0, 2, 3 },{"ADC", ADC, ZP0, 2, 3 },{"ROR", ROR, ZP0, 2, 5 },{"RRA", RRA, ZP0, 2, 5 },{"PLA", PLA, IMP, 1, 4 },{"ADC", ADC, IMM, 2, 2 },{"ROR", ROR, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"JMP", JMP, IND, 3, 5 },{"ADC", ADC, ABS, 3, 4 },{"ROR", ROR, ABS, 3, 6 },{"RRA", RRA, ABS, 3, 6 }},
    {{ "BVS", BVS, REL, 2, 2 },{"ADC", ADC, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"RRA", RRA, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"ADC", ADC, ZPX, 2, 4 },{"ROR", ROR, ZPX, 2, 6 },{"RRA", RRA, ZPX, 2, 6 },{"SEI", SEI, IMP, 1, 2 },{"ADC", ADC, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"RRA", RRA, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"ADC", ADC, ABX, 3, 4 },{"ROR", ROR, ABX, 3, 7 },{"RRA", RRA, ABX, 3, 7 }},
    {{ "NOP", NOP, IMM, 2, 2 },{"STA", STA, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SAX", SAX, IZX, 2, 6 },{"STY", STY, ZP0, 2, 3 },{"STA", STA, ZP0, 2, 3 },{"STX", STX, ZP0, 2, 3 },{"SAX", SAX, ZP0, 2, 3 },{"DEY", DEY, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"TXA", TXA, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ABS, 3, 4 },{"STA", STA, ABS, 3, 4 },{"STX", STX, ABS, 3, 4 },{"SAX", SAX, ABS, 3, 4 }},
    {{ "BCC", BCC, REL, 2, 2 },{"STA", STA, IZY, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ZPX, 2, 4 },{"STA", STA, ZPX, 2, 4 },{"STX", STX, ZPY, 2, 4 },{"SAX", SAX, ZPY, 2, 4 },{"TYA", TYA, IMP, 1, 2 },{"STA", STA, ABY, 3, 5 },{"TXS", TXS, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STA", STA, ABX, 3, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 }},
    {{ "LDY", LDY, IMM, 2, 2 },{"LDA", LDA, IZX, 2, 6 },{"LDX", LDX, IMM, 2, 2 },{"LAX", LAX, IZX, 2, 6 },{"LDY", LDY, ZP0, 2, 3 },{"LDA", LDA, ZP0, 2, 3 },{"LDX", LDX, ZP0, 2, 3 },{"LAX", LAX, ZP0, 2, 3 },{"TAY", TAY, IMP, 1, 2 },{"LDA", LDA, IMM, 2, 2 },{"TAX", TAX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ABS, 3, 4 },{"LDA", LDA, ABS, 3, 4 },{"LDX", LDX, ABS, 3, 4 },{"LAX", LAX, ABS, 3, 4 }},
    {{ "BCS", BCS, REL, 2, 2 },{"LDA", LDA, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"LAX", LAX, IZY, 2, 5 },{"LDY", LDY, ZPX, 2, 4 },{"LDA", LDA, ZPX, 2, 4 },{"LDX", LDX, ZPY, 2, 4 },{"LAX", LAX, ZPY, 2, 4 },{"CLV", CLV, IMP, 1, 2 },{"LDA", LDA, ABY, 3, 4 },{"TSX", TSX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ABX, 3, 4 },{"LDA", LDA, ABX, 3, 4 },{"LDX", LDX, ABY, 3, 4 },{"LAX", LAX, ABY, 3, 4 }},
    {{ "CPY", CPY, IMM, 2, 2 },{"CMP", CMP, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"DCP", DCP, IZX, 2, 8 },{"CPY", CPY, ZP0, 2, 3 },{"CMP", CMP, ZP0, 2, 3 },{"DEC", DEC, ZP0, 2, 5 },{"DCP", DCP, ZP0, 2, 5 },{"INY", INY, IMP, 1, 2 },{"CMP", CMP, IMM, 2, 2 },{"DEX", DEX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"CPY", CPY, ABS, 3, 4 },{"CMP", CMP, ABS, 3, 4 },{"DEC", DEC, ABS, 3, 6 },{"DCP", DCP, ABS, 3, 6 }},
    {{ "BNE", BNE, REL, 2, 2 },{"CMP", CMP, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"DCP", DCP, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"CMP", CMP, ZPX, 2, 4 },{"DEC", DEC, ZPX, 2, 6 },{"DCP", DCP, ZPX, 2, 6 },{"CLD", CLD, IMP, 1, 2 },{"CMP", CMP, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"DCP", DCP, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"CMP", CMP, ABX, 3, 4 },{"DEC", DEC, ABX, 3, 7 },{"DCP", DCP, ABX, 3, 7 }},
    {{ "CPX", CPX, IMM, 2, 2 },{"SBC", SBC, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"ISB", ISB, IZX, 2, 8 },{"CPX", CPX, ZP0, 2, 3 },{"SBC", SBC, ZP0, 2, 3 },{"INC", INC, ZP0, 2, 5 },{"ISB", ISB, ZP0, 2, 5 },{"INX", INX, IMP, 1, 2 },{"SBC", SBC, IMM, 2, 2 },{"NOP", NOP, IMP, 1, 2 },{"SBC", SBC, IMM, 2, 2 },{"CPX", CPX, ABS, 3, 4 },{"SBC", SBC, ABS, 3, 4 },{"INC", INC, ABS, 3, 6 },{"ISB", ISB, ABS, 3, 6 }},
    {{ "BEQ", BEQ, REL, 2, 2 },{"SBC", SBC, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"ISB", ISB, IZY, 2, 8 },{"NOP", NOP, ZPX, 2, 4 },{"SBC", SBC, ZPX, 2, 4 },{"INC", INC, ZPX, 2, 6 },{"ISB", ISB, ZPX, 2, 6 },{"SED", SED, IMP, 1, 2 },{"SBC", SBC, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"ISB", ISB, ABY, 3, 7 },{"NOP", NOP, ABX, 3, 4 },{"SBC", SBC, ABX, 3, 4 },{"INC", INC, ABX, 3, 7 },{"ISB", ISB, ABX, 3, 7 }},
};

opcode get_opcode(uint8_t input) {
    //split into nibble halfs
    uint8_t opcode_first_half = input >> 4;
    uint8_t opcode_second_half = input & 0x0F;

    //get opcode from matrix
    opcode opcode_obj = opcode_matrix[opcode_first_half][opcode_second_half];

    return opcode_obj;
}

void clock()
{
    //needs to be run at the propor clock cycle to be accurate
    if (cycles == 0) {
        instruction_count++;
        //get next opcode
        current_opcode = cpuBus_read(program_counter);

        //print cpu state
        print_cpu_state();
        
        //flag set thing
        set_flag(U_flag, 1);

        //increment program counter
        program_counter++;

        //cycles

        opcode op_to_execute = get_opcode(current_opcode);
        cycles = op_to_execute.cycle_count;


        //execute the instruction, keep track if return 1 as that means add cycle
        uint8_t extra_cycle_addressingMode = op_to_execute.addressing_mode();
        uint8_t extra_cycle_opcode = op_to_execute.opcode();
        //add any necessary cycles
        if ((extra_cycle_opcode == 1) && (extra_cycle_addressingMode == 1))
        {
            cycles++;
        }

        //flag set thing
        set_flag(U_flag, 1);

    }
    //decrement a cycle every clock cycle, we dont have to calculate every cycle as long as the clock is synced in the main function
    cycles--;
    total_cycles++;
}

void reset()
{
    accumulator = 0x00;
    x_register = 0x00;
    y_register = 0x00;
    stack_pointer = 0xFD;
    status_register = 0x00 | (1 << 5);

    //hard coded location
    absolute_address = 0xFFFC;
    uint8_t low = cpuBus_read(absolute_address);
    uint8_t high = cpuBus_read(absolute_address + 1) << 8;

    //combine
    program_counter = high | low;

    data_at_absolute = 0x00;
    absolute_address = 0x0000;
    relative_address = 0x00;
    cycles = 7;
    //nestest specific
    status_register = 0x24;
}

void interrupt_request()
{
    if (check_flag(I_flag) != 1)
    {
        //pc to stack
        cpuBus_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
        stack_pointer--;
        cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
        stack_pointer--;

        //set flags
        set_flag(B_flag, false);
        set_flag(U_flag, true);
        set_flag(I_flag, true);

        //save flags to stack
        cpuBus_write(0x0100 + stack_pointer, status_register);

        //new pc hard coded
        absolute_address = 0xFFFE;
        uint8_t low = cpuBus_read(absolute_address);
        uint8_t high = cpuBus_read(absolute_address + 1) << 8;

        //combine
        program_counter = high | low;

        cycles = 7;

    }
}

void non_maskable_interrupt_request()
{
    //pc to stack
    cpuBus_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
    stack_pointer--;
    cpuBus_write(0x0100 + stack_pointer, program_counter & 0x00FF);
    stack_pointer--;

    //set flags
    set_flag(B_flag, false);
    set_flag(U_flag, true);
    set_flag(I_flag, true);

    //save flags to stack
    cpuBus_write(0x0100 + stack_pointer, status_register);

    //new pc hard coded
    absolute_address = 0xFFFE;
    uint8_t low = cpuBus_read(absolute_address);
    uint8_t high = cpuBus_read(absolute_address + 1) << 8;

    //combine
    program_counter = high | low;

    cycles = 7;
}

void set_flag(uint8_t flag, bool value)
{
    if (value == true)
    {
        if (check_flag(flag) == 0)
        {
            status_register += flag;
        }
    }
    else
    {
        if (check_flag(flag) == 1)
        {
            status_register -= flag;
        }
    }
}
uint8_t check_flag(uint8_t flag)
{
    //0000 0000
    //0000 0000
    //NVUB DIZC
    if ((status_register & flag) == flag)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//initializes cpu
void initialize_cpu()
{
    //sets cpu stuff to zero and ram as well
    uint8_t accumulator = 0x00;
    uint8_t x_register = 0x00;
    uint8_t y_register = 0x00;

    uint16_t program_counter = 0x8000;
    uint8_t stack_pointer = 0xFD;
    uint8_t status_register = 0x00;

    uint8_t current_opcode = 0x00;
    uint8_t cycles = 0x00;

    for (int i = 0; i < sizeof(cpuBus); i++)
    {
        //make sure ram is zerod
        cpuBus[i] = 0x00;
    }

}

void load_rom(char* filename)
{
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Error: Could not open file %s\n", filename);
        exit(1);
    }

    // read iNES header
    unsigned char header[16];
    fread(header, sizeof(unsigned char), 16, file);

    // check for valid iNES header
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
    {
        printf("Error: Invalid iNES header\n");
        exit(1);
    }

    // extract PRG ROM data
    int prg_rom_size = header[4] * 16384;
    unsigned char* prg_rom = malloc(prg_rom_size);
    fread(prg_rom, sizeof(unsigned char), prg_rom_size, file);

    // extract CHR ROM data
    int chr_rom_size = header[5] * 8192;
    unsigned char* chr_rom = malloc(chr_rom_size);
    fread(chr_rom, sizeof(unsigned char), chr_rom_size, file);

    // extract mapper number
    int mapper_num = ((header[6] >> 4) & 0x0F) | (header[7] & 0xF0);

    // extract mirroring mode
    int mirroring_mode = (header[6] & 0x01) ? 1 : 0;

    // extract battery-backed PRG RAM size
    int prg_ram_size = header[8] * 8192;

    // extract trainer data
    int trainer_size = (header[6] & 0x04) ? 512 : 0;
    unsigned char* trainer_data = malloc(trainer_size);
    if (trainer_size > 0)
    {
        fread(trainer_data, sizeof(unsigned char), trainer_size, file);
    }

    // extract palette data
    int palette_data_offset = 16 + prg_rom_size + chr_rom_size + trainer_size;
    unsigned char palette_data[32];
    fseek(file, palette_data_offset, SEEK_SET);
    fread(palette_data, sizeof(unsigned char), 32, file);

    // load PRG ROM data into memory starting at 0x8000
    for (int i = 0; i < prg_rom_size; i++)
    {
        cpuBus_write(0xC000 + i, prg_rom[i]);
    }

    // load CHR ROM data into PPU memory starting at 0x0000
    for (int i = 0; i < chr_rom_size; i++)
    {
        ppuBus_write(0x0000 + i, chr_rom[i]);
    }

    // load palette data into PPU memory starting at 0x3F00
    for (int i = 0; i < 32; i++)
    {
        ppuBus_write(0x3F00 + i, palette_data[i]);
    }

    free(prg_rom);
    free(chr_rom);
    free(trainer_data);
    fclose(file);
}


void print_cpu_state()
{
    //FORMAT: C000  JMP                    A:00 X:00 Y:00 P:24 SP:FD CYC:7
    fprintf(fp, "%04X  %s                    A:%02X X:%02X Y:%02X P:%X SP:%X CYC:%d\n", program_counter, get_opcode(current_opcode).name, accumulator, x_register, y_register, status_register, stack_pointer, total_cycles);
}

void print_ram_state(int depth, int start_position)
{
    depth = depth + start_position;
    for (int i = start_position; i < depth; i++)
    {
        printf("cpuBus[0x%x]: 0x%x\n", i, cpuBus[i]);
    }
}

uint8_t palette_colors[64][3] = {
    { 84, 84, 84 },
    { 0, 30, 116 },
    { 8, 16, 144 },
    { 48, 0, 136 },
    { 68, 0, 100 },
    { 92, 0, 48 },
    { 84, 4, 0 },
    { 60, 24, 0 },
    { 32, 42, 0 },
    { 8, 58, 0 },
    { 0, 64, 0 },
    { 0, 60, 0 },
    { 0, 50, 60 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 152, 150, 152 },
    { 8, 76, 196 },
    { 48, 50, 236 },
    { 92, 30, 228 },
    { 136, 20, 176 },
    { 160, 20, 100 },
    { 152, 34, 32 },
    { 120, 60, 0 },
    { 84, 90, 0 },
    { 40, 114, 0 },
    { 8, 124, 0 },
    { 0, 118, 40 },
    { 0, 102, 120 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 236, 238, 236 },
    { 76, 154, 236 },
    { 120, 124, 236 },
    { 176, 98, 236 },
    { 228, 84, 236 },
    { 236, 88, 180 },
    { 236, 106, 100 },
    { 212, 136, 32 },
    { 160, 170, 0 },
    { 116, 196, 0 },
    { 76, 208, 32 },
    { 56, 204, 108 },
    { 56, 180, 204 },
    { 60, 60, 60 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 236, 238, 236 },
    { 168, 204, 236 },
    { 188, 188, 236 },
    { 212, 178, 236 },
    { 236, 174, 236 },
    { 236, 174, 212 },
    { 236, 180, 176 },
    { 228, 196, 144 },
    { 204, 210, 120 },
    { 180, 222, 120 },
    { 168, 226, 144 },
    { 152, 226, 180 },
    { 160, 214, 228 },
    { 160, 162, 160 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};

void updateFrame() {
    // Update texture with new RGB data
    SDL_UpdateTexture(texture, NULL, (void*)r, WIDTH * sizeof(uint8_t));
    SDL_UpdateTexture(texture, NULL, (void*)g, WIDTH * sizeof(uint8_t));
    SDL_UpdateTexture(texture, NULL, (void*)b, WIDTH * sizeof(uint8_t));

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Get window size
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    // Calculate aspect ratio
    float aspect_ratio = (float)WIDTH / (float)HEIGHT;

    // Calculate size of rectangle to render RGB data
    int render_width = window_width * 2 / 3;
    int render_height = window_height * 2 / 3;

    if (render_height > window_height * 2 / 3) {
        render_height = window_height * 2 / 3;
        render_width = (int)(render_height * aspect_ratio);
    }

    // Calculate position of rectangle to render RGB data
    int x = 0;
    int y = 0;

    // Render RGB data to screen
    SDL_Rect rect = {x, y, render_width, render_height};
    SDL_RenderCopy(renderer, texture, NULL, &rect);

    // Render border around rectangle
    SDL_Rect border_rect = {x - 2, y - 2, render_width + 4, render_height + 4};
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &border_rect);

    // Render 8 small rectangles beneath RGB data with spacing
    int small_rect_width = render_width / 16;
    int small_rect_height = small_rect_width;
    int small_rect_y = y + render_height + 10;
    int spacing = small_rect_width / 2;
    for (int i = 0; i < 4; i++) {

        int small_rect_x = x + i * small_rect_width * 4 + spacing * (i + 1);

        //palette first row

        //first rectangle
        SDL_Rect rect = {small_rect_x, small_rect_y, small_rect_height, small_rect_height};
        //get color
        uint8_t *color = palette_colors[ppuBus_read(0x3F00 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect);

        //second rectangle
        SDL_Rect rect2 = {small_rect_x + small_rect_height, small_rect_y, small_rect_height, small_rect_height};
        color = palette_colors[ppuBus_read(0x3F01 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect2);

        //third rectangle
        SDL_Rect rect3 = {small_rect_x + small_rect_height * 2, small_rect_y, small_rect_height, small_rect_height};
        color = palette_colors[ppuBus_read(0x3F02 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect3);

        //fourth rectangle
        SDL_Rect rect4 = {small_rect_x + small_rect_height * 3, small_rect_y, small_rect_height, small_rect_height};
        color = palette_colors[ppuBus_read(0x3F03 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect4);

        //white border
        SDL_Rect small_rect = {small_rect_x, small_rect_y, small_rect_width * 4, small_rect_height};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &small_rect);
    }
    small_rect_y += small_rect_height + 10;
    for (int i = 4; i < 8; i++) {

        int small_rect_x = x + (i - 4) * small_rect_width * 4 + spacing * ((i - 4) + 1);

        //render palette second row

        //first rectangle
        SDL_Rect rect = {small_rect_x, small_rect_y, small_rect_height, small_rect_height};
        uint8_t *color = palette_colors[ppuBus_read(0x3F10 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect);

        //second rectangle
        SDL_Rect rect2 = {small_rect_x + small_rect_height, small_rect_y, small_rect_height, small_rect_height};
        color = palette_colors[ppuBus_read(0x3F11 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect2);

        //third rectangle
        SDL_Rect rect3 = {small_rect_x + small_rect_height * 2, small_rect_y, small_rect_height, small_rect_height};
        color = palette_colors[ppuBus_read(0x3F12 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect3);

        //fourth rectangle
        SDL_Rect rect4 = {small_rect_x + small_rect_height * 3, small_rect_y, small_rect_height, small_rect_height};
        color = palette_colors[ppuBus_read(0x3F13 + i * 4)];
        SDL_SetRenderDrawColor(renderer, color[0], color[1], color[2], 255);
        SDL_RenderFillRect(renderer, &rect4);
        

        //render white border
        SDL_Rect small_rect = {small_rect_x, small_rect_y, small_rect_width * 4, small_rect_height};
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &small_rect);
    }

    // Render to screen
    SDL_RenderPresent(renderer);
}

void print_ppu_registers()
{
    printf("PPUCTRL: 0x%x\n", ppu_ctrl);
    printf("PPUMASK: 0x%x\n", ppu_mask);
    printf("PPUSTATUS: 0x%x\n", ppu_status);
    printf("OAMADDR: 0x%x\n", oam_addr);
    printf("OAMDATA: 0x%x\n", oam_data);
    printf("PPUSCROLL: 0x%x\n", ppu_scroll);
    printf("PPUADDR: 0x%x\n", ppu_addr);
    printf("PPUDATA: 0x%x\n", ppu_data);
    printf("OAMDMA: 0x%x\n", oam_dma);
}

void printPalettes() {
    // Iterate over palette data in PPU memory

    //0x3F00 - background color
    //0x3F01 - 0x3F03 - background palette 0
    //0x3F05 - 0x3F07 - background palette 1
    //0x3F09 - 0x3F0B - background palette 2
    //0x3F0D - 0x3F0F - background palette 3
    //0x3F11 - 0x3F13 - sprite palette 0
    //0x3F15 - 0x3F17 - sprite palette 1
    //0x3F19 - 0x3F1B - sprite palette 2
    //0x3F1D - 0x3F1F - sprite palette 3
    for (int i = 0; i < 32; i++) {
        uint8_t value = ppuBus_read(0x3F00 + i);
        printf("Palette[%d], ppuBus[%X] = %02X\n", i, 0x3F00 + i, value);
    }
}

int main(int argc, char* argv[])
{
    //open file for debug
    fp = fopen("debug.txt", "w");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }

    //cpu and ram
    initialize_cpu();
    reset();
    //load rom at 0x8000, default location
    load_rom("nestest.nes");
    program_counter = 0xC000;
    for (int i = 0; i < 100000; i++)
    {
        clock();
    }

    printPalettes();

   SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow("Nes Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * 3, HEIGHT * 3, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Fill RGB data with example values
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        r[i] = 100;
        g[i] = 100;
        b[i] = 100;
    }

    // Create texture from RGB data
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    // Render initial frame
    updateFrame();

    // Main loop
    SDL_Event event;
    while (SDL_WaitEvent(&event)) {
        if (event.type == SDL_QUIT) {
            break;
        }

        //maintain aspect ratio

        if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
            // Calculate new window size while maintaining aspect ratio
            int newWidth = event.window.data1;
            int newHeight = (newWidth * HEIGHT) / WIDTH;
            SDL_SetWindowSize(window, newWidth, newHeight);
        }

        //detect keyboard input

        if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_SPACE) {
                // Update RGB data to random values
                for (int i = 0; i < WIDTH * HEIGHT; i++) {
                    r[i] = rand() % 255;
                    g[i] = rand() % 255;
                    b[i] = rand() % 255;
                }
                //print ppu registers
                print_ppu_registers();
                updateFrame();
            }
        }
    }

    // Clean up resources
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
    //TODO: fix pallettes and render the sprite tables
}