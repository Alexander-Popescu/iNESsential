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

    uint16_t cycle = 0;
    uint16_t scanline = 0;

private:
    Emulator *emulator;
};