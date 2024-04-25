//some constants used throughout the project
#pragma once
//texture dimensions for nes window
#define DEFAULT_WIDTH 256
#define DEFAULT_HEIGHT 240

//change for larger / smaller window size
#define WINDOW_SCALE_FACTOR 4

//size of pattern tables on frontend
#define PATTERN_TABLE_SCALING_VALUE 3

//for cartridge bank parsing
#define CHR_ROM_BANKSIZE 8192
#define PRG_ROM_BANKSIZE 16384

//ansii terminal color codes
#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

//for IMGUI font
#define FONT_SCALE 2

#define C_FLAG 0b00000001 //Carry
#define Z_FLAG 0b00000010 //Zero
#define I_FLAG 0b00000100 //Interrupt disabled
#define D_FLAG 0b00001000 //Decimal
#define B_FLAG 0b00010000 //Break
#define U_FLAG 0b00100000 //Unused
#define V_FLAG 0b01000000 //Overflow
#define N_FLAG 0b10000000 //Negative

struct CpuState {
    uint8_t accumulator;
    uint8_t x_register;
    uint8_t y_register;
    uint16_t program_counter;
    uint8_t stack_pointer;
    uint8_t status_register;

    //to avoid cycle accurate emulation
    uint8_t remaining_cycles;
};

//for mid-tile ppu rendering

struct TileInfo {
    uint8_t id;
    uint8_t attribute;
    uint8_t lsb;
    uint8_t msb;
};