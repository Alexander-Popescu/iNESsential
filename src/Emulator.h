// NES emulator
#pragma once
#include <cstdint>
#include <vector>
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

class Emulator {
public:
    Emulator();
    ~Emulator();

    int runUntilBreak();
    bool loadCartridge(const char* gamePath = "../testRoms/nestest.nes");
    void reset();

    bool cartridgeLoaded = false;

private:
    //CPU
    CPU *cpu;

    //CPU bus

    //$0000–$07FF internal ram
    uint8_t ram[0x0800];

    //$4020–$FFFF cartridge address space
    Cartridge *cartridge;

    //PPU
    PPU *ppu;
    
};