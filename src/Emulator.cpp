#include "Emulator.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"
#include <iostream>
#include "Definitions.h"

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

int Emulator::runUntilBreak(int instructionRequest) {
    //this function will return to allow the frontend to render the frame or debug information
    //realtime means it breaks after every frame is finished generating, a specified instruction count will run that many

    int instructionStart = instructionCount;

    while ((realtime || (instructionCount < instructionStart + instructionRequest)) && pushFrame == false) {
        runSingleInstruction();
    }

    //reset pushframe for next frame
    pushFrame = false;
     
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

void Emulator::runSingleInstruction() {

    //simulate end of frame for testing
    if (instructionCount % 10 == 0) {
        pushFrame = true;
        printf(GREEN "Emulator: InstructionCount: %i\n" RESET, instructionCount);
    }

    //dont forget as this is what breaks the instruction loop
    instructionCount++;
}