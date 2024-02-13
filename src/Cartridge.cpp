#include "Cartridge.h"
#include <cstdio>
#include <cstdlib>

//ansi terminal color codes
#define RED "\x1b[31m"
#define YELLOW "\x1b[33m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

Cartridge::Cartridge() {

}

Cartridge::~Cartridge() {
    delete[] PRG_ROM;
    delete[] CHR_ROM;
}

int Cartridge::loadRom(const char* gamePath) {
    //load cartridge, using iNES spec to support most NES roms

    printf(YELLOW "Cartridge: Loading ROM\n" RESET);

    //open rom file
    FILE* fp = fopen(gamePath, "rb");

    if (fp == NULL)
    {
        printf(RED "Error: Could not open file\n" RESET);
        exit(0);
    }

    //iNES header is 16 bytes
    uint8_t header[16];
    fread(&header, sizeof(uint8_t), 16, fp);

    //check if header is valid
    if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A)
    {
        printf("Error: Invalid iNES header\n");
        return 1;
    }

    //header structure:
    // 4-> PRGROM size in 16kb chunks
    // 5-> CHRROM size in 8kb chunks
    //6-15-> other flags and padding

    int PRGsize = header[4] * 16384;
    int CHRsize = header[5] * 8192;

    PRG_ROM = new uint8_t[PRGsize];
    CHR_ROM = new uint8_t[CHRsize];

    //read data into object
    fread(PRG_ROM, sizeof(uint8_t), PRGsize, fp);
    fread(CHR_ROM, sizeof(uint8_t), CHRsize, fp);

    // close the file
    fclose(fp);

    printf(GREEN "Cartridge: ROM loaded%s\n" RESET, gamePath);
    return 1;
}

uint8_t *Cartridge::getPRGROM()
{
    return PRG_ROM;
}

uint8_t *Cartridge::getCHRROM()
{
    return CHR_ROM;
}