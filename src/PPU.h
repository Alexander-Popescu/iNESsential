// Picture Processing Unit
#pragma once
#include <cstdint>
#include <vector>

class Emulator;

class PPU {
public:
    PPU(Emulator *emulator);
    ~PPU();

    void reset();

    bool clock();

    //cpu interaction
    uint8_t readRegisters(uint16_t address);
    void writeRegisters(uint16_t address, uint8_t data);

    uint16_t cycle = 0;
    uint16_t scanline = 0;


    //vram
    uint8_t nameTables[2][1024];
	uint8_t	palettes[32];

    //palette table, for translating the indexes stored in the nes to rgba values
    //copied from html of https://www.nesdev.org/wiki/PPU_palettes, 2C02
    uint32_t paletteTranslationTable[0x40] = {
        0x626262FF, 0x001FB2FF, 0x2404C8FF, 0x5200B2FF, 0x730076FF, 0x800024FF, 0x730B00FF, 0x522800FF, 0x244400FF, 0x005700FF, 0x005C00FF, 0x005324FF, 0x003C76FF, 0x000000FF, 0x000000FF, 0x000000FF,
        0xABABABFF, 0x0D57FFFF, 0x4B30FFFF, 0x8A13FFFF, 0xBC08D6FF, 0xD21269FF, 0xC72E00FF, 0x9D5400FF, 0x607B00FF, 0x209800FF, 0x00A300FF, 0x009942FF, 0x007DB4FF, 0x000000FF, 0x000000FF, 0x000000FF,
        0xFFFFFFFF, 0x53AEFFFF, 0x9085FFFF, 0xD365FFFF, 0xFF57FFFF, 0xFF5DCFFF, 0xFF7757FF, 0xFA9E00FF, 0xBDC700FF, 0x7AE700FF, 0x43F611FF, 0x26EF7EFF, 0x2CD5F6FF, 0x4E4E4EFF, 0x000000FF, 0x000000FF,
        0xFFFFFFFF, 0xB6E1FFFF, 0xCED1FFFF, 0xE9C3FFFF, 0xFFBCFFFF, 0xFFBDF4FF, 0xFFC6C3FF, 0xFFD59AFF, 0xE9E681FF, 0xCEF481FF, 0xB6FB9AFF, 0xA9FAC3FF, 0xA9F0F4FF, 0xB8B8B8FF, 0x000000FF, 0x000000FF
    };

private:
    Emulator *emulator;
};