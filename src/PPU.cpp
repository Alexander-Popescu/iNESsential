#include "PPU.h"
#include "Emulator.h"
#include <cstdlib>

PPU::PPU(Emulator *emulator) {
    this->emulator = emulator;

    cycle = 0;
    scanline = 0;
}

PPU::~PPU() {
    
}

void PPU::reset() {
    cycle = 0;
    scanline = 0;
}

bool PPU::clock() {
    //returns true if frame is ready to be updated on the screen

    //noise for now
    if (scanline >= 0 && scanline < 240 && cycle < 256) {
        emulator->pixelBuffer->writeBufferPixel(cycle, scanline, rand() % 2 ? 0xFFFFFFFF : 0x000000FF);
    }

    cycle++;
    if (cycle >= 341) {
        cycle = 0;
        scanline++;
        if (scanline >= 261) {
            scanline = -1;
            return true;
        }
    }
    return false;
}

uint8_t PPU::readRegisters(uint16_t address) {
    //address is already mapped
    switch (address) {
        case 0x2000:
            return 0;
        case 0x2001:
            return 0;
        case 0x2002:
            return 0;
        case 0x2003:
            return 0;
        case 0x2004:
            return 0;
        case 0x2005:
            return 0;
        case 0x2006:
            return 0;
        case 0x2007:
            return 0;
        default:
            printf(RED "PPU: invalid register read 0x%04X\n" RESET, address);
            emulator->realtime = false;
            return 0;
    }
}

void PPU::writeRegisters(uint16_t address, uint8_t data) {
    //address is already mapped
    switch (address) {
        case 0x2000:
        case 0x2001:
        case 0x2002:
        case 0x2003:
        case 0x2004:
        case 0x2005:
        case 0x2006:
        case 0x2007:
        default:
            printf(RED "PPU: invalid register write 0x%04X\n" RESET, address);
            emulator->realtime = false;
    }
}