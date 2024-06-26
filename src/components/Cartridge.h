// iNES cartridge
#pragma once
#include <cstdint>
#include <vector>
#include "../Definitions.h"
#include "CPU.h"

class Cartridge {
public:
    Cartridge();
    ~Cartridge();

    uint8_t read(uint16_t address);
    void write(uint16_t address);
    int loadRom(char* cartName);

    int PRGsize;
    int CHRsize;
    int mapper;
    const char* mirroring = "horizontal";

private:
    uint8_t* PRG_ROM;
    uint8_t* CHR_ROM;
};