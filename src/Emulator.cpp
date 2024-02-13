#include "Emulator.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include <iostream>

//ansi terminal color codes
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"


Emulator::Emulator() {
    printf(GREEN "Emulator: Started\n" RESET);

    this->cpu = new CPU();
    printf(GREEN "Emulator: CPU created\n" RESET);

    this->ppu = new PPU();
    printf(GREEN "Emulator: PPU created\n" RESET);

    this->cartridge = new Cartridge();
    printf(GREEN "Emulator: Cartridge created\n" RESET);

    this->cartridgeLoaded = (this->loadCartridge() == 0);
    printf(GREEN "Emulator: Default Cartridge loaded\n" RESET);
}

Emulator::~Emulator() {
    delete cpu;
    delete ppu;
    delete cartridge;
}

int Emulator::runUntilBreak() {
    //This will run until the specified cycle count has been reached or a frame is done being constructed
    //It will break to allow SDL to render the current state for debugging or to see the frame

    return 0;
}

bool Emulator::loadCartridge(const char* gamePath)
{
    //returns 0 if cartridge was loaded correctly
    cartridgeLoaded = (cartridge->loadRom(gamePath) == 0);

    return cartridgeLoaded;
}

void Emulator::reset() {
    cpu->reset();
    ppu->reset();
}