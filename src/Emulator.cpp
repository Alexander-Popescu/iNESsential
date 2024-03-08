#include "Emulator.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

Emulator::Emulator() {
    printf(GREEN "Emulator: Started\n" RESET);

    this->cpu = new CPU(this);
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
    
    //check if log was ever written to and delete file if blank
    fseek(logFile, 0, SEEK_END);
    if (ftell(logFile) == 0) {
        printf(YELLOW "Emulator: Log unused, deleting logfile\n" RESET);
        if (remove(filename) == 0) {
            printf(GREEN "Emulator: Logfile deleted\n" RESET);
        } else {
            printf(RED "Emulator: Error deleting logfile\n" RESET);
        }
    } else {
        printf(GREEN "Emulator: Keeping logfile\n" RESET);
    }
    fclose(logFile);
}

//testing opcodes
void Emulator::testOpcodes() {
    TestingMode = true;

    cpu->testOpcodes();
    
    TestingMode = false;
}

void Emulator::log(const char* message) {
    //assumes logging was checked already
    fprintf(logFile, message);
}

int Emulator::runUntilBreak(int instructionRequest) {
    //this function will return to allow the frontend to render the frame or debug information
    //realtime means it breaks after every frame is finished generating, a specified instruction count will run that many

    int instructionStart = instructionCount;

    while ((realtime || (instructionCount < instructionStart + instructionRequest)) && pushFrame == false) {
        runSingleInstruction();

        //reset pushframe for next frame, remove when ppu does this once per frame
        if (realtime){
            pushFrame = true;
        }
        
    }

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
    while (cpu->clock() == false) {};

    //run corrosponding opcode on cpu, and log cpustate if enabled, along with the opcode that was run
    instructionCount++;

}

CpuState *Emulator::getCpuState() {
    return cpu->getState();
}

uint8_t Emulator::cpuBusRead(uint16_t address) {
    if (TestingMode) {
        printf("Test Read: %i\n", address);
        return testRam[address];
    }
    //cpubus, start with cpuram
    if ( 0x0000 < address && address < 0x1FFF)
    {
        //cpuram, AND with physical ram size because of mirroring
        return ram[address && 0x07FF];
    } 
    else if (address > 0x4020)
    {
        return cartridge->read(address);
    }
    return 0;
}

void Emulator::cpuBusWrite(uint16_t address, uint8_t data) {
    if (TestingMode) {
        printf("Test Write: %i, Data: %i\n", address, data);
        testRam[address] = data;
        return;
    }
    if ( 0x0000 < address && address < 0x1FFF)
    {
        //cpuram, AND with physical ram size because of mirroring
        ram[address & 0x07FF] = data;
    }
}

int *Emulator::getCycleCount() {
    return &cpu->cycleCount;
}