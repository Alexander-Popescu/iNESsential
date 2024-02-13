// iNES cartridge
#pragma once
#include <cstdint>
#include <vector>

class Cartridge {
public:
    Cartridge();
    ~Cartridge();

    uint8_t read(uint16_t address);
    uint8_t write(uint16_t address);
    int loadRom(const char* gamePath);

    uint8_t *getPRGROM();
    uint8_t *getCHRROM();

private:
    uint8_t* PRG_ROM;
    uint8_t* CHR_ROM;
    int mapper;
};