#pragma once
#include <cstdint>
#include <vector>
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Definitions.h"

class Emulator {
public:
    Emulator();
    ~Emulator();

    int runUntilBreak(int instructionRequest);
    bool loadCartridge(const char* gamePath = "../testRoms/nestest.nes");
    void reset();
    
    void runSingleInstruction();
    CpuState *getCpuState();

    bool cartridgeLoaded = false;

    //toggles realtime emulation between instruction by instruction 
    bool realtime = false;

    //debug information
    int instructionCount = 0;
    int cycleCount = 0;

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

    //set to true to break
    bool pushFrame = false;
    
};