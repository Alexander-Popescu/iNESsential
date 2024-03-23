#pragma once
#include <cstdint>
#include <vector>
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include "Definitions.h"
#include <iostream>
#include "PixelBuffer.h"

class Emulator {
public:
    Emulator(PixelBuffer *pixelBuffer);
    ~Emulator();

    //for ppu to directly render pixels
    PixelBuffer *pixelBuffer;

    int runUntilBreak(int instructionRequest);
    bool loadCartridge(const char* gamePath = "../testRoms/nestest.nes");
    void reset();
    bool clock();
    
    void runSingleInstruction();
    void runSingleFrame();
    CpuState *getCpuState();
    uint16_t getPPUcycle();
    uint16_t getPPUscanline();

    uint8_t cpuBusRead(uint16_t address);
    void cpuBusWrite(uint16_t address, uint8_t data);
    int *getCycleCount();

    void log(const char* message);

    //for testing opcodes with Tom Harte CPU tests
    void testOpcodes();
    bool TestingMode = false;
    //64kb ram for testing
    uint8_t testRam[0x10000];

    bool cartridgeLoaded = false;

    //toggles realtime emulation between instruction by instruction 
    bool realtime = false;

    //debug information
    int instructionCount = 0;

    //toggle logging to file
    bool logging = false;

    //for output of emulator logs
    FILE* logFile;
    char filename[36];

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

    //set to true to break, assuming only ppu would need to trigger this 
    bool pushFrame = false;

    int emulationTicks = 0;
    
};