#pragma once
#include <cstdint>
#include <vector>
#include "components/CPU.h"
#include "components/PPU.h"
#include "components/Cartridge.h"
#include "Definitions.h"
#include <iostream>
#include "frontend/PixelBuffer.h"

class Emulator {
public:
    Emulator(PixelBuffer *pixelBuffer);
    ~Emulator();

    //for ppu to directly render pixels
    PixelBuffer *pixelBuffer;

    //$4020–$FFFF cartridge address space, let debugwindow access
    Cartridge *cartridge;

    //PPU
    PPU *ppu;

    int runUntilBreak(int instructionRequest);
    bool loadCartridge(char* gamePath);
    void reset();
    void clock();
    void cpuNMI();
    
    void runSingleInstruction();
    void runSingleFrame();
    void runSingleCycle();
    CpuState *getCpuState();
    uint16_t getPPUcycle();
    uint16_t getPPUscanline();

    void updatePatternTables();
    void updatePalettes();

    uint8_t cpuBusRead(uint16_t address);
    void cpuBusWrite(uint16_t address, uint8_t data);
    uint8_t ppuBusRead(uint16_t address);
    void ppuBusWrite(uint16_t address, uint8_t data);

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

    //cartridge
    char cartName[25] = "nestest";

    //syncronize cpu and ppu
    int emulationTicks = 0;

    int frameCount = 0;

    //controller stuff
    uint8_t controller1 = 0;
    uint8_t controller1ShiftReg = 0;

    uint8_t controller2 = 0;
    uint8_t controller2ShiftReg = 0;

    //direct  memory access (DMA)
    uint8_t DMAAddr = 0;
    uint8_t DMAData = 0;
    uint8_t DMAPage = 0;
    bool DMA = false;
    bool DMASync = false;

private:
    //CPU
    CPU *cpu;

    //CPU bus

    //$0000–$07FF internal ram
    uint8_t ram[0x0800];

    //set to true to break, assuming only ppu would need to trigger this 
    bool pushFrame = false;
    
};