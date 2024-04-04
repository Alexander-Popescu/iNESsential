#include "Emulator.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

Emulator::Emulator(PixelBuffer *pixelBuffer) {
    printf(GREEN "Emulator: Started\n" RESET);

    this->cpu = new CPU(this);
    printf(GREEN "Emulator: CPU created\n" RESET);

    this->ppu = new PPU(this);
    printf(GREEN "Emulator: PPU created\n" RESET);

    this->cartridge = new Cartridge();
    printf(GREEN "Emulator: Cartridge created\n" RESET);

    this->cartridgeLoaded = (this->loadCartridge(this->cartName));
    
    //link pixel buffer
    this->pixelBuffer = pixelBuffer;

    if (cartridgeLoaded) {
        updatePatternTables();
        updatePalettes();
    }
}

Emulator::~Emulator() {
    delete cpu;
    delete ppu;
    delete cartridge;

    //check if log even exists
    if (logFile == NULL) {
        printf(YELLOW "Emulator: logfile doesnt exist\n" RESET);
        return;
    }
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
        clock();
    }

    pushFrame = false;
     
    return 0;
}

bool Emulator::loadCartridge(char *cartName)
{
    //returns 0 if cartridge was loaded correctly
    if (cartridgeLoaded) {
        delete cartridge;
    }
    cartridge = new Cartridge();
    cartridgeLoaded = (cartridge->loadRom(cartName) == 0);

    return cartridgeLoaded;
}

void Emulator::reset() {
    if(cartridgeLoaded) {
        printf(YELLOW "Emulator: Reset\n" RESET);
        cpu->reset();
        printf(YELLOW "Emulator: CPU Reset\n" RESET);
        ppu->reset();
        printf(YELLOW "Emulator: PPU Reset\n" RESET);
        emulationTicks = 0;
        instructionCount = 0;

        updatePatternTables();
        updatePalettes();
    }

    if (logging) {
        log("Emulator Reset\n");
    }
}

void Emulator::runSingleFrame() {
    while (pushFrame == false) {
        clock();
    }
}

void Emulator::runSingleCycle() {
    clock();
}

void Emulator::clock() {
    if (emulationTicks % 3 == 0) {
        cpu->clock();
    }
    pushFrame = ppu->clock();
    emulationTicks++;
}

CpuState *Emulator::getCpuState() {
    return cpu->getState();
}

uint8_t Emulator::cpuBusRead(uint16_t address) {
    if (TestingMode) {
        return testRam[address];
    }
    //cpubus, start with cpuram
    if ( 0x0000 <= address && address <= 0x1FFF)
    {
        //cpuram, AND with physical ram size because of mirroring
        return ram[address & 0x07FF];
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        //ppu registers and mirroring
        return ppu->readRegisters(address & 0x2007);
    }
    else if (address >= 0x4000 && address <= 0x401F)
    {
        //audio and input registers
        return 0;
    }
    else if (address >= 0x4020)
    {
        //cartridge space
        return cartridge->read(address);
    }

    printf(RED "Emulator: CPU Bus Read from invalid address 0x%04X\n" RESET, address);
    return 0;
}

void Emulator::cpuBusWrite(uint16_t address, uint8_t data) {
    if (TestingMode) {
        testRam[address] = data;
        return;
    }
    if ( 0x0000 <= address && address <= 0x1FFF)
    {
        //cpuram, AND with physical ram size because of mirroring
        ram[address & 0x07FF] = data;
        return;
    }
    else if (address >= 0x2000 && address <= 0x3FFF)
    {
        //control ppu registers
        ppu->writeRegisters(address & 0x2007, data);
        return;
    }
    else if (address >= 0x4000 && address <= 0x401F)
    {
        //audio and input things
        return;
    }
    //mapper 0 has no write, implement later for other mappers

    printf(RED "Emulator: CPU Bus Write to invalid address 0x%04X\n" RESET, address);
    return;
}

uint8_t Emulator::ppuBusRead(uint16_t address) {
    if (address >= 0x0000 && address <= 0x1FFF)
    {
        return cartridge->read(address);
    }
    else if (address >= 0x2000 && address <= 0x23FF)
    {
        //nametable 0
        return ppu->nameTables[0][address & 0x03FF];
    }
    else if (address >= 0x2400 && address <= 0x27FF)
    {
        //nametable 1
        return ppu->nameTables[0][address & 0x03FF];
    }
    else if (address >= 0x2800 && address <= 0x2BFF)
    {
        //nametable 2
        return ppu->nameTables[1][address & 0x03FF];
    }
    else if (address >= 0x2C00 && address <= 0x2FFF)
    {
        //nametable 3
        return ppu->nameTables[1][address & 0x03FF];
    }
    else if (address >= 0x3000 && address <= 0x3EFF)
    {
        //unused
        return 0;
    }
    else if (address >= 0x3F00 && address <= 0x3FFF)
    {
        return ppu->palettes[address & 0x001F];
    }
    printf(RED "Emulator: invalid PPU read 0x%04X\n" RESET, address);
    return 0;
}

void Emulator::ppuBusWrite(uint16_t address, uint8_t data) {
    if (address >= 0x2000 && address <= 0x23FF)
    {
        //nametable 0
        ppu->nameTables[0][address & 0x03FF] = data;
        return;
    }
    else if (address >= 0x2400 && address <= 0x27FF)
    {
        //nametable 1
        ppu->nameTables[0][address & 0x03FF] = data;
        return;
    }
    else if (address >= 0x2800 && address <= 0x2BFF)
    {
        //nametable 2
        ppu->nameTables[1][address & 0x03FF] = data;
        return;
    }
    else if (address >= 0x2C00 && address <= 0x2FFF)
    {
        //nametable 3
        ppu->nameTables[1][address & 0x03FF] = data;
        return;
    }
    else if (address >= 0x3000 && address <= 0x3EFF)
    {
        //unused
        return;
    }
    else if (address >= 0x3F00 && address <= 0x3FFF)
    {
        ppu->palettes[address & 0x001F] = data;
        return;
    }
    printf(RED "Emulator: invalid PPU write 0x%04X\n" RESET, address);
    return;
}

int *Emulator::getCycleCount() {
    return &cpu->cycleCount;
}

uint16_t Emulator::getPPUcycle() {
    return ppu->cycle;
}

uint16_t Emulator::getPPUscanline() {
    return ppu->scanline;
}

void Emulator::updatePatternTables() {
    uint32_t pixels[128 * 128];
    uint8_t demoPalette[4] = {0x01, 0x2A, 0x16, 0x3F};
    
    for (int table = 0; table < 2; table++)
    {
        for (int y = 0; y < 16; y++) {
            for (int x = 0; x < 16; x++) {
                //tile
                for (int py = 0; py < 8; py++) {
                    uint8_t lowByte = ppuBusRead((table * 0x1000) + (y * 16 + x) * 16 + py);
                    uint8_t highByte = ppuBusRead((table * 0x1000) + (y * 16 + x) * 16 + py + 8);
                    for (int px = 0; px < 8; px++) {
                        uint8_t color = ((lowByte >> (7 - px)) & 0x01) | (((highByte >> (7 - px)) & 0x01) << 1);
                        uint32_t colorValue = ppu->paletteTranslationTable[demoPalette[color]];

                        //opengl texture insists on abgr format, no idea why, quick fix flips color channels
                        colorValue = ((colorValue & 0xFF000000) >> 24) | ((colorValue & 0x00FF0000) >> 8)  | ((colorValue & 0x0000FF00) << 8)  | ((colorValue & 0x000000FF) << 24);

                        pixels[(x * 8 + px) + (y * 8 + py) * 128] = colorValue;
                    }
                }
            }
        }
        pixelBuffer->addPixelArrayToPatternTable(pixels, table);
    }
    return;
}

void Emulator::updatePalettes() {
    //front end models this a little weird because of how the mirroring works, so it appears to be off by one position
    for (int i = 0; i < 32; i++) {
        uint32_t color;
        if ((i % 4) == 0) {
            //palette mirroring
            color = ppu->paletteTranslationTable[ppu->palettes[0]];
        }
        else {
            color = ppu->paletteTranslationTable[ppu->palettes[i]];
        }
        pixelBuffer->palettes[i / 4][i % 4] = ImVec4(((color & 0xFF000000) >> 24) / 255.0f, ((color & 0x00FF0000) >> 16) / 255.0f, ((color & 0x0000FF00) >> 8) / 255.0f, 1.0f);
    }
    return;
}

void Emulator::cpuNMI() {
    cpu->nmi();
}