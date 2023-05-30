#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

//addressing modes
uint8_t IMP()
{
    //implied
    printf("test");
    return 0;
}

uint8_t IMM()
{
    //immediate
    return 0;
}

uint8_t ZP0()
{
    //zero page
    return 0;
}

uint8_t ZPX()
{
    //zero page with offset
    return 0;
}

uint8_t ZPY()
{
    //zero page with offset
    return 0;
}

uint8_t REL()
{
    //relative for branching
    return 0;
}

uint8_t ABS()
{
    //absolute
    return 0;
}

uint8_t ABX()
{
    //absolute with offset
    return 0;
}

uint8_t ABY()
{
    //absolute with offset
    return 0;
}

uint8_t IND()
{
    //indirect
    return 0;
}

uint8_t IZX()
{
    //indexed indirect
    return 0;
}

uint8_t IZY()
{
    //indirect indexed
    return 0;
}

uint8_t ACC()
{
    //accumulator
    return 0;
}

//opcode functions
uint8_t ADC()
{
    return 0;
}

uint8_t AND()
{
    return 0;
}

uint8_t ASL()
{
    return 0;
}

uint8_t BCC()
{
    return 0;
}

uint8_t BCS()
{
    return 0;
}

uint8_t BEQ()
{
    return 0;
}

uint8_t BIT()
{
    return 0;
}

uint8_t BMI()
{
    return 0;
}

uint8_t BNE()
{
    return 0;
}

uint8_t BPL()
{
    return 0;
}

uint8_t BRK()
{
    return 0;
}

uint8_t BVC()
{
    return 0;
}

uint8_t BVS()
{
    return 0;
}

uint8_t CLC()
{
    return 0;
}

uint8_t CLD()
{
    return 0;
}

uint8_t CLI()
{
    return 0;
}

uint8_t CLV()
{
    return 0;
}

uint8_t CMP()
{
    return 0;
}

uint8_t CPX()
{
    return 0;
}

uint8_t CPY()
{
    return 0;
}

uint8_t DEC()
{
    return 0;
}

uint8_t DEX()
{
    return 0;
}

uint8_t DEY()
{
    return 0;
}

uint8_t EOR()
{
    return 0;
}

uint8_t INC()
{
    return 0;
}

uint8_t INX()
{
    return 0;
}

uint8_t INY()
{
    return 0;
}

uint8_t JMP()
{
    return 0;
}

uint8_t JSR()
{
    return 0;
}

uint8_t LDA()
{
    return 0;
}

uint8_t LDX()
{
    return 0;
}

uint8_t LDY()
{
    return 0;
}

uint8_t LSR()
{
    return 0;
}

uint8_t NOP()
{
    return 0;
}

uint8_t ORA()
{
    return 0;
}

uint8_t PHA()
{
    return 0;
}

uint8_t PHP()
{
    return 0;
}

uint8_t PLA()
{
    return 0;
}

uint8_t PLP()
{
    return 0;
}

uint8_t ROL()
{
    return 0;
}

uint8_t ROR()
{
    return 0;
}

uint8_t RTI()
{
    return 0;
}

uint8_t RTS()
{
    return 0;
}

uint8_t SBC()
{
    return 0;
}

uint8_t SEC()
{
    return 0;
}

uint8_t SED()
{
    return 0;
}

uint8_t SEI()
{
    return 0;
}

uint8_t STA()
{
    return 0;
}

uint8_t STX()
{
    return 0;
}

uint8_t STY()
{
    return 0;
}

uint8_t TAX()
{
    return 0;
}

uint8_t TAY()
{
    return 0;
}

uint8_t TSX()
{
    return 0;
}

uint8_t TXA()
{
    return 0;
}

uint8_t TXS()
{
    return 0;
}

uint8_t TYA()
{
    return 0;
}

uint8_t XXX()
{
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
        current_opcode = mem_read(program_counter, false);
        //increment program counter of course
        program_counter++;

        opcode opcode_object = get_opcode(current_opcode);


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
    (get_opcode(0x0000).addressing_mode)();
    return 0;

    //TODO: test the functions already written and fix them up. make cpu and bus global, move everthing into header files, and then start working on the addressing modes and opcodes, and write unit tests?
}