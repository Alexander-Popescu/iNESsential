#include "Cartridge.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>

Cartridge::Cartridge() {
    this->PRG_ROM = nullptr;
    this->CHR_ROM = nullptr;
    this->mapper = 0;
    this->PRGsize = 0;
    this->CHRsize = 0;
}

Cartridge::~Cartridge() {
    delete[] PRG_ROM;
    delete[] CHR_ROM;
}

int Cartridge::loadRom(char* cartName) {
    //load cartridge, using iNES spec to support most NES roms

    printf(YELLOW "Cartridge: Loading ROM\n" RESET);

    //file location from name
    char gamePath[41] = "../testRoms/";
    strcat(gamePath, cartName);
    strcat(gamePath, ".nes");

    //open rom file
    FILE* fp = fopen(gamePath, "rb");

    if (fp == NULL)
    {
        printf(RED "Cartridge: Could not open file %s\n" RESET, gamePath);
        return 1;
    }

    //iNES header is 16 bytes
    uint8_t header[16];
    fread(&header, sizeof(uint8_t), 16, fp);

    //check if header is valid
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
    {
        printf(RED "Cartridge: Invalid iNES header\n" RESET);
        return 1;
    }

    //header structure:
    // 4-> PRGROM size in 16kb chunks
    // 5-> CHRROM size in 8kb chunks
    //6-15-> other flags and padding

    PRGsize = header[4] * PRG_ROM_BANKSIZE;
    CHRsize = header[5] * CHR_ROM_BANKSIZE;

    //mapper 0
    PRG_ROM = new uint8_t[PRGsize];
    CHR_ROM = new uint8_t[CHRsize];

    //read data into object
    fread(PRG_ROM, sizeof(uint8_t), PRGsize, fp);
    fread(CHR_ROM, sizeof(uint8_t), CHRsize, fp);

    // close the file
    fclose(fp);
    printf(GREEN "Cartridge: ROM loaded%s\n" RESET, gamePath);
    printf(GREEN "Cartridge: ROM info: PRG banks: %d, CHR banks: %d\n" RESET, header[4], header[5]);
    return 0;
}

uint8_t Cartridge::read(uint16_t address)
{
     //mapper 0 mirroring
    if (address >= 0x8000 && address <= 0xFFFF)
    {
        return PRG_ROM[address & (PRGsize - 1)];
    }
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        return CHR_ROM[address];
    }
    
    printf(RED "invalid cartridge read 0x%04X\n" RESET, address);
    return 0;
}