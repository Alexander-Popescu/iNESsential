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

uint8_t check_flag(uint8_t flag);
void set_flag(uint8_t flag, bool value);
void interrupt_request();


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

//memory
uint8_t ram[64 * 1024];

//for debug output
FILE* fp;

//total cycles elapsed
uint64_t total_cycles = 0;

//memory IO
void mem_write(uint16_t address, uint8_t data)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        ram[address] = data;
    }
    else
    {
        printf("Invalid memory address: %d", address);
    }
}

uint8_t mem_read(uint16_t address)
{
    //check if valid memory request
    if ((address >= 0x0000) && (address <= 0xFFFF))
    {
        return ram[address];
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
    absolute_address = program_counter;
    program_counter++;
    return 0;
}

uint8_t ZP0()
{
    //zero page
    program_counter++;
    absolute_address = mem_read(program_counter) || 0x0000;
    return 0;
}

uint8_t ZPX()
{
    //zero page with offset from X register
    program_counter++;
    absolute_address = (mem_read(program_counter) + x_register) && 0x00FF;
    return 0;
}

uint8_t ZPY()
{
    //zero page with offset from Y register
    program_counter++;
    absolute_address = (mem_read(program_counter) + y_register) && 0x00FF;
    return 0;
}

uint8_t REL()
{
    //relative for branching instructions
    relative_address = mem_read(program_counter);

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
    uint16_t low = mem_read(program_counter);
    program_counter++;
    //second
    uint16_t high = mem_read(program_counter) << 8;

    absolute_address = high | low;
    return 0;
}

uint8_t ABX()
{
    //absolute with offset, offsets by X register after bytes are read

    //read first byte
    uint16_t low = mem_read(program_counter);
    program_counter++;
    //second
    uint16_t high = mem_read(program_counter) << 8;

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
    uint16_t low = mem_read(program_counter);
    program_counter++;
    //second
    uint16_t high = mem_read(program_counter) << 8;

    absolute_address = high | low;
    absolute_address += y_register;

    //this is one where if the memory page changes we need more cycles
    uint16_t first_byte = absolute_address & 0xFF00;

    return (first_byte != high) ? 1 : 0;

    return 0;
}

uint8_t IND()
{
    //indirect, data at the given address is the actual address we want to use

    //first
    uint16_t low_input = mem_read(program_counter);
    program_counter++;
    //second
    uint16_t high_input = mem_read(program_counter) << 8;

    //combine input
    uint16_t input = high_input | low_input;

    //first
    uint16_t low_output = mem_read(input);
    //second
    uint16_t high_output = mem_read(input) << 8;

    //data at "pointer"
    absolute_address = high_output | low_output;

    return 0;
}

uint8_t IZX()
{
    //indexed indirect, 1 byte reference to 2 byte address in zero page with x register offset
    uint8_t input = mem_read(program_counter);
    program_counter++;

    //first
    uint16_t low = mem_read((uint16_t)(input + x_register));
    //second
    uint16_t high = mem_read((uint16_t)(input + x_register + 1)) << 8;

    absolute_address = high | low;

    return 0;
}

uint8_t IZY()
{
    //indirect indexed
    uint8_t input = mem_read(program_counter);
    program_counter++;

    //get actual address and offset final by y, but also check for page change

    uint16_t low = mem_read(input);
    uint16_t high = mem_read(input + 1) << 8;

    absolute_address = high | low;

    absolute_address += y_register;

    //check for page change
    uint16_t first_byte = absolute_address & 0xFF00;
    return (first_byte != high) ? 1 : 0;
}

//helper function to avoid writing !IMP for every memory address
uint8_t update_absolute_data()
{
    if (accumulator_mode == true)
    {
        data_at_absolute = accumulator;
        return data_at_absolute;
    }
    data_at_absolute = mem_read(absolute_address);
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
    accumulator = (accumulator << 1);

    //save bit 7 to carry flag
    set_flag(C_flag, accumulator & 0x80);

    //set 0 bit to 0
    accumulator = accumulator & 0xFE;

    //flags

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
        program_counter++;
        return 1;
    }
    program_counter++;
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
        program_counter++;

        return 1;
    }
    program_counter++;
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
        program_counter++;
        return 1;
    }
    program_counter++;
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
        program_counter++;
    }
    program_counter++;
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
        program_counter++;
        return 1;
    }
    program_counter++;
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
        program_counter++;
        return 1;
    }
    program_counter++;
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
        program_counter++;
        return 1;
    }
    program_counter++;
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
        program_counter++;
        return 1;
    }
    program_counter++;
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

    if (accumulator >= data_at_absolute)
    {
        set_flag(C_flag, 1);
    }
    else
    {
        set_flag(C_flag, 0);
    }

    if (accumulator == data_at_absolute)
    {
        set_flag(Z_flag, 1);
    }
    else
    {
        set_flag(Z_flag, 0);
    }

    if ((accumulator - data_at_absolute) & 0x80)
    {
        set_flag(N_flag, 1);
    }
    else
    {
        set_flag(N_flag, 0);
    }

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
    //update
    mem_write(absolute_address, ram[absolute_address]--);
    update_absolute_data();

    //set flags
    if (data_at_absolute == 0x00)
    {
        set_flag(Z_flag, 1);
    }

    if (data_at_absolute & 0x80)
    {
        set_flag(N_flag, 1);
    }

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

    if (y_register & 0x80)
    {
        set_flag(N_flag, 1);
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
    mem_write(absolute_address, ram[absolute_address] + 1);

    //set flags
    if (mem_read(absolute_address) == 0x00)
    {
        set_flag(Z_flag, 1);
    }

    if (mem_read(absolute_address) & 0x80)
    {
        set_flag(N_flag, 1);
    }

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
    uint16_t second_half = mem_read(program_counter - 1);
    uint16_t first_half = mem_read(program_counter) << 8;

    uint16_t jump_address = first_half | second_half;

    program_counter = jump_address;

    return 0;
}

uint8_t JSR()
{

    uint16_t second_half = mem_read(program_counter - 1);
    uint16_t first_half = mem_read(program_counter) << 8;

    uint16_t target = first_half | second_half;

    //push program counter to stack
    mem_write(0x0100 + stack_pointer, (program_counter >> 8) & 0xFF);
    stack_pointer--;
    mem_write(0x0100 + stack_pointer, (program_counter - 1) & 0xFF);
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

    if (accumulator_mode == true)
    {
        accumulator = accumulator >> 1;

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
        accumulator_mode = false;
    }
    else
    {
        mem_write(absolute_address, ram[absolute_address] >> 1);

        //set flags
        if (mem_read(absolute_address) == 0x00)
        {
            set_flag(Z_flag, 1);
        }

        if (mem_read(absolute_address) & 0x80)
        {
            set_flag(N_flag, 1);
        }
    }

    return 0;
}

uint8_t NOP()
{
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
    mem_write(0x0100 + stack_pointer, accumulator);
    stack_pointer--;
    return 0;
}

uint8_t PHP()
{
    //flags must be set before push
    set_flag(B_flag, 1);
    mem_write(0x0100 + stack_pointer, status_register); 
    set_flag(B_flag, 0);
    set_flag(U_flag, 1);
    stack_pointer--;

    return 0;
}

uint8_t PLA()
{
    stack_pointer++;
    accumulator = mem_read(0x0100 + stack_pointer);
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
    status_register = mem_read(0x0100 + stack_pointer);
    set_flag(U_flag, 1);
    set_flag(B_flag, 0);
    return 0;
}

uint8_t ROL()
{

    if (accumulator_mode == true)
    {
        uint8_t old = accumulator;
        accumulator = accumulator << 1;
        accumulator = accumulator | check_flag(C_flag);

        //set flags
        set_flag(C_flag, old & 0x80);
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
        accumulator_mode = false;
    }
    else
    {
        uint8_t old = mem_read(absolute_address);
        mem_write(absolute_address, ram[absolute_address] << 1);
        mem_write(absolute_address, ram[absolute_address] | (check_flag(C_flag) << 7));

        //set flags
        set_flag(C_flag, old & 0x80);
        if (mem_read(absolute_address) == 0x00)
        {
            set_flag(Z_flag, 1);
        }

        if (mem_read(absolute_address) & 0x80)
        {
            set_flag(N_flag, 1);
        }
    }

    return 0;
}

uint8_t ROR()
{

    if (accumulator_mode == true)
    {
        uint8_t old = accumulator;
        accumulator = accumulator >> 1;
        accumulator = accumulator | (check_flag(C_flag) << 7);

        //set flags
        set_flag(C_flag, old & 0x01);
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
        accumulator_mode = false;
    }
    else
    {
        uint8_t old = ram[absolute_address];
        mem_write(absolute_address, ram[absolute_address] >> 1);
        mem_write(absolute_address, ram[absolute_address] | (check_flag(C_flag) << 7));

        //set flags
        set_flag(C_flag, old & 0x01);
        if (mem_read(absolute_address) == 0x00)
        {
            set_flag(Z_flag, 1);
        }

        if (mem_read(absolute_address) & 0x80)
        {
            set_flag(N_flag, 1);
        }
    }

    return 0;
}

uint8_t RTI()
{
    //returns from an inturrupt

    //read status
    stack_pointer++;
    status_register = mem_read(0x0100 + stack_pointer);

    stack_pointer++;
    program_counter = mem_read(0x0100 + stack_pointer);
    program_counter = (program_counter << 8) | mem_read(0x0100 + stack_pointer + 1);
    stack_pointer++;
    return 0;
}

uint8_t RTS()
{

    stack_pointer++;
    program_counter = mem_read(0x0100 + stack_pointer);
    stack_pointer++;
    program_counter = program_counter | (mem_read(0x0100 + stack_pointer) << 8);
    //pc minus 1 was pushed to the stack, fix here
    program_counter++;
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
    mem_write(absolute_address, accumulator);
    return 0;
}

uint8_t STX()
{
    mem_write(absolute_address, x_register);
    return 0;
}

uint8_t STY()
{
    mem_write(absolute_address, y_register);
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
    {{ "BRK", BRK, IMP, 1, 7 },{"ORA", ORA, IND, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ORA", ORA, ZP0, 2, 3 },{"ASL", ASL, ZP0, 2, 5 },{"???", XXX, IMP, 0, 2 },{"PHP", PHP, IMP, 1, 3 },{"ORA", ORA, IMM, 2, 2 },{"ASL", ASL, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ORA", ORA, ABS, 3, 4 },{"ASL", ASL, ABS, 3, 6 },{"???", XXX, IMP, 0, 2 }},
    {{ "BPL", BPL, REL, 2, 2 },{"ORA", ORA, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ORA", ORA, ZPX, 2, 4 },{"ASL", ASL, ZPX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"CLC", CLC, IMP, 1, 2 },{"ORA", ORA, ABY, 3, 4 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ORA", ORA, ABX, 3, 4 },{"ASL", ASL, ABX, 3, 7 },{"???", XXX, IMP, 0, 2 }},
    {{ "JSR", JSR, ABS, 3, 6 },{"AND", AND, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"BIT", BIT, ZP0, 2, 3 },{"AND", AND, ZP0, 2, 3 },{"ROL", ROL, ZP0, 2, 5 },{"???", XXX, IMP, 0, 2 },{"PLP", PLP, IMP, 1, 4 },{"AND", AND, IMM, 2, 2 },{"ROL", ROL, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"BIT", BIT, ABS, 3, 4 },{"AND", AND, ABS, 3, 4 },{"ROL", ROL, ABS, 3, 6 },{"???", XXX, IMP, 0, 2 }},
    {{ "BMI", BMI, REL, 2, 2 },{"AND", AND, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"AND", AND, ZPX, 2, 4 },{"ROL", ROL, ZPX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SEC", SEC, IMP, 1, 2 },{"AND", AND, ABY, 3, 4 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"AND", AND, ABX, 3, 4 },{"ROL", ROL, ABX, 3, 7 },{"???", XXX, IMP, 0, 2 }},
    {{ "RTI", RTI, IMP, 1, 6 },{"EOR", EOR, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"EOR", EOR, ZP0, 2, 3 },{"LSR", LSR, ZP0, 2, 5 },{"???", XXX, IMP, 0, 2 },{"PHA", PHA, IMP, 1, 3 },{"EOR", EOR, IMM, 2, 2 },{"LSR", LSR, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"JMP", JMP, ABS, 3, 3 },{"EOR", EOR, ABS, 3, 4 },{"LSR", LSR, ABS, 3, 6 },{"???", XXX, IMP, 0, 2 }},
    {{ "BVC", BVC, REL, 2, 2 },{"EOR", EOR, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"EOR", EOR, ZPX, 2, 4 },{"LSR", LSR, ZPX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"CLI", CLI, IMP, 1, 2 },{"EOR", EOR, ABY, 3, 4 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"EOR", EOR, ABX, 3, 4 },{"LSR", LSR, ABX, 3, 7 },{"???", XXX, IMP, 0, 2 }},
    {{ "RTS", RTS, IMP, 1, 6 },{"ADC", ADC, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ADC", ADC, ZP0, 2, 3 },{"ROR", ROR, ZP0, 2, 5 },{"???", XXX, IMP, 0, 2 },{"PLA", PLA, IMP, 1, 4 },{"ADC", ADC, IMM, 2, 2 },{"ROR", ROR, ACC, 1, 2 },{"???", XXX, IMP, 0, 2 },{"JMP", JMP, IND, 3, 5 },{"ADC", ADC, ABS, 3, 4 },{"ROR", ROR, ABS, 3, 6 },{"???", XXX, IMP, 0, 2 }},
    {{ "BVS", BVS, REL, 2, 2 },{"ADC", ADC, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ADC", ADC, ZPX, 2, 4 },{"ROR", ROR, ZPX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SEI", SEI, IMP, 1, 2 },{"ADC", ADC, ABY, 3, 4 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"ADC", ADC, ABX, 3, 4 },{"ROR", ROR, ABX, 3, 7 },{"???", XXX, IMP, 0, 2 }},
    {{ "???", XXX, IMP, 0, 2 },{"STA", STA, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ZP0, 2, 3 },{"STA", STA, ZP0, 2, 3 },{"STX", STX, ZP0, 2, 3 },{"???", XXX, IMP, 0, 2 },{"DEY", DEY, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"TXA", TXA, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ABS, 3, 4 },{"STA", STA, ABS, 3, 4 },{"STX", STX, ABS, 3, 4 },{"???", XXX, IMP, 0, 2 }},
    {{ "BCC", BCC, REL, 2, 2 },{"STA", STA, IZY, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STY", STY, ZPX, 2, 4 },{"STA", STA, ZPX, 2, 4 },{"STX", STX, ZPY, 2, 4 },{"???", XXX, IMP, 0, 2 },{"TYA", TYA, IMP, 1, 2 },{"STA", STA, ABY, 3, 5 },{"TXS", TXS, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"STA", STA, ABX, 3, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 }},
    {{ "LDY", LDY, IMM, 2, 2 },{"LDA", LDA, IZX, 2, 6 },{"LDX", LDX, IMM, 2, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ZP0, 2, 3 },{"LDA", LDA, ZP0, 2, 3 },{"LDX", LDX, ZP0, 2, 3 },{"???", XXX, IMP, 0, 2 },{"TAY", TAY, IMP, 1, 2 },{"LDA", LDA, IMM, 2, 2 },{"TAX", TAX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ABS, 3, 4 },{"LDA", LDA, ABS, 3, 4 },{"LDX", LDX, ABS, 3, 4 },{"???", XXX, IMP, 0, 2 }},
    {{ "BCS", BCS, REL, 2, 2 },{"LDA", LDA, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ZPX, 2, 4 },{"LDA", LDA, ZPX, 2, 4 },{"LDX", LDX, ZPY, 2, 4 },{"???", XXX, IMP, 0, 2 },{"CLV", CLV, IMP, 1, 2 },{"LDA", LDA, ABY, 3, 4 },{"TSX", TSX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"LDY", LDY, ABX, 3, 4 },{"LDA", LDA, ABX, 3, 4 },{"LDX", LDX, ABY, 3, 4 },{"???", XXX, IMP, 0, 2 }},
    {{ "CPY", CPY, IMM, 2, 2 },{"CMP", CMP, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"CPY", CPY, ZP0, 2, 3 },{"CMP", CMP, ZP0, 2, 3 },{"DEC", DEC, ZP0, 2, 5 },{"???", XXX, IMP, 0, 2 },{"INY", INY, IMP, 1, 2 },{"CMP", CMP, IMM, 2, 2 },{"DEX", DEX, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"CPY", CPY, ABS, 3, 4 },{"CMP", CMP, ABS, 3, 4 },{"DEC", DEC, ABS, 3, 6 },{"???", XXX, IMP, 0, 2 }},
    {{ "BNE", BNE, REL, 2, 2 },{"CMP", CMP, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"CMP", CMP, ZPX, 2, 4 },{"DEC", DEC, ZPX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"CLD", CLD, IMP, 1, 2 },{"CMP", CMP, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"CMP", CMP, ABX, 3, 4 },{"DEC", DEC, ABX, 3, 7 },{"???", XXX, IMP, 0, 2 }},
    {{ "CPX", CPX, IMM, 2, 2 },{"SBC", SBC, IZX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"CPX", CPX, ZP0, 2, 3 },{"SBC", SBC, ZP0, 2, 3 },{"INC", INC, ZP0, 2, 5 },{"???", XXX, IMP, 0, 2 },{"INX", INX, IMP, 1, 2 },{"SBC", SBC, IMM, 2, 2 },{"NOP", NOP, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"CPX", CPX, ABS, 3, 4 },{"SBC", SBC, ABS, 3, 4 },{"INC", INC, ABS, 3, 6 },{"???", XXX, IMP, 0, 2 }},
    {{ "BEQ", BEQ, REL, 2, 2 },{"SBC", SBC, IZY, 2, 5 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"SBC", SBC, ZPX, 2, 4 },{"INC", INC, ZPX, 2, 6 },{"???", XXX, IMP, 0, 2 },{"SED", SED, IMP, 1, 2 },{"SBC", SBC, ABY, 3, 4 },{"NOP", NOP, IMP, 1, 2 },{"???", XXX, IMP, 0, 2 },{"???", XXX, IMP, 0, 2 },{"SBC", SBC, ABX, 3, 4 },{"INC", INC, ABX, 3, 7 },{"???", XXX, IMP, 0, 2 }},
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
        //get next opcode
        current_opcode = mem_read(program_counter);
        //increment program counter of course

        //cycles

        opcode op_to_execute = get_opcode(current_opcode);
        cycles = op_to_execute.cycle_count;
        
        print_cpu_state();
        program_counter++;

        //execute the instruction, keep track if return 1 as that means add cycle
        uint8_t extra_cycle_addressingMode = op_to_execute.addressing_mode();
        uint8_t extra_cycle_opcode = op_to_execute.opcode();
        //add any necessary cycles
        if ((extra_cycle_opcode == 1) && (extra_cycle_addressingMode == 1))
        {
            cycles++;
        }

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
    uint8_t low = mem_read(absolute_address);
    uint8_t high = mem_read(absolute_address + 1) << 8;

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
        mem_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
        stack_pointer--;
        mem_write(0x0100 + stack_pointer, program_counter & 0x00FF);
        stack_pointer--;

        //set flags
        set_flag(B_flag, false);
        set_flag(U_flag, true);
        set_flag(I_flag, true);

        //save flags to stack
        mem_write(0x0100 + stack_pointer, status_register);

        //new pc hard coded
        absolute_address = 0xFFFE;
        uint8_t low = mem_read(absolute_address);
        uint8_t high = mem_read(absolute_address + 1) << 8;

        //combine
        program_counter = high | low;

        cycles = 7;

    }
}

void non_maskable_interrupt_request()
{
    //pc to stack
    mem_write(0x0100 + stack_pointer, (program_counter >> 8) & 0x00FF);
    stack_pointer--;
    mem_write(0x0100 + stack_pointer, program_counter & 0x00FF);
    stack_pointer--;

    //set flags
    set_flag(B_flag, false);
    set_flag(U_flag, true);
    set_flag(I_flag, true);

    //save flags to stack
    mem_write(0x0100 + stack_pointer, status_register);

    //new pc hard coded
    absolute_address = 0xFFFE;
    uint8_t low = mem_read(absolute_address);
    uint8_t high = mem_read(absolute_address + 1) << 8;

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

    for (int i = 0; i < sizeof(ram); i++)
    {
        //make sure ram is zerod
        ram[i] = 0x00;
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

    // extract program ROM data
    int prg_rom_size = header[4] * 16384;
    unsigned char* prg_rom = malloc(prg_rom_size);
    fread(prg_rom, sizeof(unsigned char), prg_rom_size, file);

    // load program ROM data into memory starting at 0x8000
    for (int i = 0; i < prg_rom_size; i++)
    {
        mem_write(0xC000 + i, prg_rom[i]);
    }

    free(prg_rom);
    fclose(file);
}


void print_cpu_state()
{
    //FORMAT: C000  JMP                    A:00 X:00 Y:00 P:24 SP:FD CYC:7
    fprintf(fp, "%X  %s                    A:%02X X:%02X Y:%02X P:%X SP:%X CYC:%d\n", program_counter, get_opcode(current_opcode).name, accumulator, x_register, y_register, status_register, stack_pointer, total_cycles);
}

void print_ram_state(int depth, int start_position)
{
    depth = depth + start_position;
    for (int i = start_position; i < depth; i++)
    {
        printf("ram[0x%x]: 0x%x\n", i, ram[i]);
    }
}

int main(void)
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
    print_ram_state(10, 0xC5FD);
    program_counter = 0xC000;
    for (int i = 0; i < 5000; i++)
    {
        clock();
    }
    return 0;

    //TODO: get debug output formatted right and check all opcodes, and fix warnings
}