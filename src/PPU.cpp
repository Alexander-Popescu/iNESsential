#include "PPU.h"
#include "Emulator.h"
#include <cstdlib>

PPU::PPU(Emulator *emulator) {
    this->emulator = emulator;
}

PPU::~PPU() {
    
}

void PPU::reset() {
    
}

bool PPU::clock() {
    //returns true if frame is ready to be updated on the screen

    //noise for now
    if (scanline >= 0 && scanline < 240 && cycle < 256) {
        emulator->pixelBuffer->writeBufferPixel(cycle, scanline, rand() % 2 ? 0xFFFFFFFF : 0x00000000);
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