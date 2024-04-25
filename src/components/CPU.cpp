#include "CPU.h"
#include <stdio.h>
#include "../Emulator.h"
#include <cstring>
#include <nlohmann/json.hpp>
#include <iostream>

CPU::CPU(Emulator *emulator) {
    //hardcoded for now, normally program counter is set to the vector at 0xFFFD/0xFFFC, also some constants here are just for using nestest
    this->state.accumulator = 0;
    this->state.x_register = 0;
    this->state.y_register = 0;
    this->state.program_counter = 0x8000;
    this->state.stack_pointer = 0xFD;
    this->state.status_register = 0x24;

    this->state.remaining_cycles = 7;

    this-> absolute_address = 0x00;
    this->absolute_data = 0x00;

    //link to other parts of the emulator
    this->emulator = emulator;
}

CPU::~CPU() {
    
}

void CPU::reset() {
    printf(YELLOW "CPU: Reset\n" RESET);
    state.remaining_cycles = 7;
    //reset vector
    state.program_counter = emulator->cpuBusRead(0xFFFC) | (emulator->cpuBusRead(0xFFFD) << 8);

    state.accumulator = 0;
    state.x_register = 0;
    state.y_register = 0;
    state.stack_pointer = 0xFD;
    state.status_register = 0x20;
    cycleCount = 0;
}

CpuState *CPU::getState() {
    return &state;
}

void CPU::setFlag(uint8_t flag, bool value) {
    if (value) {
        state.status_register |= flag;
    } else {
        state.status_register &= ~flag;
    }
}

void CPU::pushStack(uint8_t data) {
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, data);
    state.stack_pointer--;
}

uint8_t CPU::pullStack() {
    state.stack_pointer++;
    return emulator->cpuBusRead(0x0100 + state.stack_pointer);
}

void CPU::updateAbsolute() {
    //absolute addresses are determined through the addressing modes, the data at this
    //location is updated here, with cases for special addressing modes
    if (accumulatorMode) {
        //uses accumulator data as argument
        absolute_data = state.accumulator;
        return;
    }

    absolute_data = emulator->cpuBusRead(absolute_address);

}

void CPU::nmi() {
    pushStack((state.program_counter >> 8) & 0xFF);
    pushStack(state.program_counter & 0xFF);

    setFlag(B_FLAG, false);
    setFlag(U_FLAG, true);
    setFlag(I_FLAG, true);
    pushStack(state.status_register);

    state.program_counter = emulator->cpuBusRead(0xFFFA) | (emulator->cpuBusRead(0xFFFB) << 8);

    state.remaining_cycles += 7;    
}

void CPU::cpuLog(OpcodeInfo opcode) {
    if (emulator->logging == false) {
        return;
    }

    // //log information for opcode that is about to run, if the log ends here the error occured in that opcode
    char logMessage[250]; 
    sprintf(logMessage, "%04X  ", state.program_counter);
    for (int i = 0; i < opcode.byteCount; i++) {
        sprintf(logMessage + strlen(logMessage), "%02X ", emulator->cpuBusRead(state.program_counter + i));
    }
    for (int i = opcode.byteCount; i < 3; i++) {
        sprintf(logMessage + strlen(logMessage), "   ");
    }
    while (strlen(logMessage) < 49) {
        sprintf(logMessage + strlen(logMessage), " ");
    }
    sprintf(logMessage + strlen(logMessage), "f%i", emulator->frameCount);
    while (strlen(logMessage) < 57) {
        sprintf(logMessage + strlen(logMessage), " ");
    }
    sprintf(logMessage + strlen(logMessage), "c%i", cycleCount);
    while (strlen(logMessage) < 70) {
        sprintf(logMessage + strlen(logMessage), " ");
    }
    sprintf(logMessage + strlen(logMessage), "i%i", emulator->instructionCount);
    while (strlen(logMessage) < 83) {
        sprintf(logMessage + strlen(logMessage), " ");
    }
    sprintf(logMessage + strlen(logMessage), "A:%02X X:%02X Y:%02X P:%02X SP:%02X v:%04X t:%04X x:0 w:%i CYC:", state.accumulator, state.x_register, state.y_register, state.status_register, state.stack_pointer, emulator->ppu->vramAddress.getValue(), emulator->ppu->tempVramAddress.getValue(), emulator->ppu->writeToggle);
    if (emulator->ppu->cycle < 10) {
        sprintf(logMessage + strlen(logMessage), " ");
    }
    if (emulator->ppu->cycle < 100) {
        sprintf(logMessage + strlen(logMessage), " ");
    }
    sprintf(logMessage + strlen(logMessage), "%i SL:%i   2002:%02X 2004:%02X 2007:%02X", emulator->ppu->cycle, emulator->ppu->scanline, emulator->ppu->PPUSTATUS.getValue(), 0x00, 0x00);

    sprintf(logMessage + strlen(logMessage), "\n");
    emulator->log(logMessage);
}

void CPU::runInstruction() {
    //decode and run opcode at program counter
    uint8_t opcodeByte = emulator->cpuBusRead(state.program_counter);
    OpcodeInfo opcode = opcodeTable[opcodeByte >> 4][opcodeByte & 0x0F];

    cpuLog(opcode);

    //add cycle counts to remaining cycles
    state.remaining_cycles += opcode.cycleCount;

    //calculate address of interest
    (this->*opcode.AddrMode)();

    //addressing mode has changed absolute_address to point to the operand, store the operand now for use in the opcodes
    updateAbsolute();

    //run the opcode
    (this->*opcode.OpFunction)();

    //extra cycle fix
    if (extraCycleCheck == 2) {
        state.remaining_cycles++;
    }
    extraCycleCheck = 0;

    cycleCount++;
    state.remaining_cycles--;

    //ensure accumulator mode is reset, because the opcodes dont reset it and they need to know
    accumulatorMode = false;

    //tell emulator
    emulator->instructionCount++;
}

void CPU::clock() {
    if (state.remaining_cycles == 0) {
        runInstruction();
    } else {
        state.remaining_cycles--;
        cycleCount++;
    }
}

//keep at bottom, cpu addressing modes and opcodes

//addressing modes, assume that the program counter is on the proceeding opcode by the end unless said otherwise
//absolute address is the location of memory to check for argument, or used directly for non-memory addressing modes

void CPU::IMPL() {
    //implied
    state.program_counter++;
    return;
}

void CPU::REL() {
    //relative
    state.program_counter++;
    int8_t offset = emulator->cpuBusRead(state.program_counter);
    state.program_counter++; //end of current instruction
    absolute_address = state.program_counter + offset; //branch instructions will branhc here sometimes, check their function

    if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
        extraCycleCheck++;
    }
}

void CPU::ABS() {
    //absolute, full address is provided
    state.program_counter++;
    absolute_address = (emulator->cpuBusRead(state.program_counter + 1) << 8) | emulator->cpuBusRead(state.program_counter);
    state.program_counter += 2;
}

void CPU::IMM() {
    //immediate, next byte is the argument
    absolute_address = state.program_counter + 1;
    state.program_counter += 2;
}

void CPU::XIND() {
    //X-indexed, indirect
    state.program_counter++;
    uint16_t baseIndex = emulator->cpuBusRead(state.program_counter);
	absolute_address = (emulator->cpuBusRead((baseIndex + state.x_register + 1) & 0xFF) << 8) | emulator->cpuBusRead((baseIndex + state.x_register) & 0xFF);
	state.program_counter++;
}

void CPU::INDY() {
    //indirect, Y-indexed
    state.program_counter++;
    uint16_t baseIndex = emulator->cpuBusRead(state.program_counter);
	absolute_address = ((emulator->cpuBusRead((baseIndex + 1) & 0xFF) << 8) | emulator->cpuBusRead((baseIndex) & 0xFF)) + state.y_register;
	state.program_counter++;

    if ((absolute_address & 0xFF00) != (emulator->cpuBusRead((baseIndex + 1) & 0xFF) << 8)) {
        extraCycleCheck++;
    }
}

void CPU::ZPG() {
    //zeropage
    state.program_counter++;
    absolute_address = emulator->cpuBusRead(state.program_counter);
    state.program_counter++;
}

void CPU::ZPGX() {
    //zeropage x-indexed
    state.program_counter++;
    absolute_address = (emulator->cpuBusRead(state.program_counter) + state.x_register) & 0xFF;
    state.program_counter++;
}

void CPU::ZPGY() {
    //zeropage y-indexed
    state.program_counter++;
    absolute_address= (emulator->cpuBusRead(state.program_counter) + state.y_register) & 0xFF;
    state.program_counter++;
}

void CPU::ABSY() {
    //absolute, y-indexed
    ABS();
    absolute_address += state.y_register;

    if ((absolute_address & 0xFF00) != ((absolute_address - state.y_register) & 0xFF00)) {
        extraCycleCheck++;
    }
}

void CPU::ACC() {
    //accumulator, use tracking variable for opcodes with accumuilator and also another addressing mode
    accumulatorMode = true;
    state.program_counter++;
}

void CPU::IND() {
    //indirect
    ABS();
    if ((absolute_address & 0x00FF) == 0x00FF)  {
        absolute_address = (emulator->cpuBusRead(absolute_address & 0xFF00) << 8) | emulator->cpuBusRead(absolute_address);
    } else {
        absolute_address = (emulator->cpuBusRead(absolute_address + 1) << 8) | emulator->cpuBusRead(absolute_address);
    }
}

void CPU::ABSX() {
    //absolute x-indexed
    ABS();
    absolute_address += state.x_register;

    if ((absolute_address & 0xFF00) != ((absolute_address - state.x_register) & 0xFF00)) {
        extraCycleCheck++;
    }
}


// opcodes, legal first

void CPU::ADC() {
    //add with carry
    uint16_t result = state.accumulator + absolute_data + (state.status_register & C_FLAG);
    setFlag(C_FLAG, result > 0xFF);
    setFlag(Z_FLAG, (result & 0xFF) == 0);
    setFlag(N_FLAG, result & 0x80);
    setFlag(V_FLAG, (~(state.accumulator ^ absolute_data) & (state.accumulator ^ result)) & 0x80);
    state.accumulator = result & 0xFF;

    extraCycleCheck++;
}

void CPU::AND() {
    //and (with accumulator)
    state.accumulator &= absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);

    extraCycleCheck++;
}

void CPU::ASL() {
    //arithmetic shift left
    if (accumulatorMode) {
        setFlag(C_FLAG, state.accumulator & 0x80);
        state.accumulator <<= 1;
        setFlag(Z_FLAG, state.accumulator == 0);
        setFlag(N_FLAG, state.accumulator & 0x80);
    } else {
        setFlag(C_FLAG, absolute_data & 0x80);
        emulator->cpuBusWrite(absolute_address, absolute_data << 1);
        updateAbsolute();
        setFlag(Z_FLAG, absolute_data == 0);
        setFlag(N_FLAG, absolute_data & 0x80);
    }
}

void CPU::BCC() {
    //branch on carry clear
    if ((state.status_register & C_FLAG) == 0x00) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BCS() {
    //branch on carry set
    if ((state.status_register & C_FLAG) == C_FLAG) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BEQ() {
    //branch on zero set
    if ((state.status_register & Z_FLAG) == Z_FLAG) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BIT() {
    //bit test
    setFlag(Z_FLAG, (absolute_data & state.accumulator) == 0);
    setFlag(N_FLAG, absolute_data & 0b10000000);
    setFlag(V_FLAG, absolute_data & 0b01000000);
}

void CPU::BMI() {
    //branch on minus (negative set)
    if ((state.status_register & N_FLAG) == N_FLAG) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BNE() {
    //branch on not equal (zero clear)
    if ((state.status_register & Z_FLAG) == 0x00) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BPL() {
    //branch on plus (negative clear)
    if ((state.status_register & N_FLAG) == 0x00) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BRK() {

    //opcode table doesnt include brk break mark byte
    state.program_counter++;

    //push next address to stack
    pushStack((state.program_counter >> 8) & 0x00FF);
    pushStack(state.program_counter & 0x00FF);

    //requires status register as well
    pushStack(state.status_register | 0x30);

    //force break
    setFlag(I_FLAG, true);

    //break vector for PC
    state.program_counter = emulator->cpuBusRead(0xFFFE) | (emulator->cpuBusRead(0xFFFF) << 8);
}

void CPU::BVC() {
    //branch on overflow clear
    if ((state.status_register & V_FLAG) == 0x00) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::BVS() {
    //branch on overflow set
    if ((state.status_register & V_FLAG) == V_FLAG) {
        if ((state.program_counter & 0xFF00) != (absolute_address & 0xFF00)) {
            state.remaining_cycles++;
        }
        state.program_counter = absolute_address;
        state.remaining_cycles++;
    }
}

void CPU::CLC() {
    //clear carry
    setFlag(C_FLAG, false);
}

void CPU::CLD() {
    //clear decimal
    setFlag(D_FLAG, false);
}

void CPU::CLI() {
    //clear interrupt disable
    setFlag(I_FLAG, false);
}

void CPU::CLV() {
    //clear overflow
    setFlag(V_FLAG, false);
}

void CPU::CMP() {
    //compare (with accumulator)

    //imagine subtracting memory from accumulator 
    uint8_t compare = state.accumulator - absolute_data;
    setFlag(C_FLAG, state.accumulator >= absolute_data);
    setFlag(Z_FLAG, compare == 0);
    setFlag(N_FLAG, compare & 0x80);

    extraCycleCheck++;
}

void CPU::CPX() {
    //compare with X

    setFlag(C_FLAG, state.x_register >= absolute_data);
    setFlag(Z_FLAG, state.x_register == absolute_data);
    setFlag(N_FLAG, (state.x_register - absolute_data) & 0x80);
}

void CPU::CPY() {
    //compare with Y

    setFlag(C_FLAG, state.y_register >= absolute_data);
    setFlag(Z_FLAG, state.y_register == absolute_data);
    setFlag(N_FLAG, (state.y_register - absolute_data) & 0x80);
}

void CPU::DEC() {
    //decrement
    emulator->cpuBusWrite(absolute_address, absolute_data - 1);

    updateAbsolute();
    setFlag(Z_FLAG, absolute_data == 0);
    setFlag(N_FLAG, absolute_data & 0x80);
}

void CPU::DEX() {
    //decrement X
    state.x_register--;
    
    setFlag(Z_FLAG, (state.x_register) == 0);
    setFlag(N_FLAG, (state.x_register) & 0x80);
}

void CPU::DEY() {
    //decrement Y
    state.y_register--;

    setFlag(Z_FLAG, (state.y_register) == 0);
    setFlag(N_FLAG, (state.y_register) & 0x80);
}

void CPU::EOR() {
    //exclusive or (with accumulator)
    state.accumulator ^= absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);

    extraCycleCheck++;
}

void CPU::INC() {
    //increment
    emulator->cpuBusWrite(absolute_address, absolute_data + 1);

    updateAbsolute();
    setFlag(Z_FLAG, absolute_data == 0);
    setFlag(N_FLAG, absolute_data & 0x80);
}

void CPU::INX() {
    //increment X
    state.x_register++;

    setFlag(Z_FLAG, state.x_register == 0);
    setFlag(N_FLAG, state.x_register & 0x80);
}

void CPU::INY() {
    //increment Y
    state.y_register++;

    setFlag(Z_FLAG, state.y_register == 0);
    setFlag(N_FLAG, state.y_register & 0x80);
}

void CPU::JMP() {
    //jump
    state.program_counter = absolute_address;
}

void CPU::JSR() {
    //jump subroutine
    //push PC one before the next instructions
    state.program_counter--;
    pushStack((state.program_counter >> 8) & 0x00FF);
    pushStack(state.program_counter & 0x00FF);
    //perfect emulation requires HSB to be read after stack push
    state.program_counter = (absolute_address & 0x00FF) | (emulator->cpuBusRead(state.program_counter) << 8);
}

void CPU::LDA() {
    //load accumulator
    state.accumulator = absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);

    extraCycleCheck++;
}

void CPU::LDX() {
    //load X
    state.x_register = absolute_data;

    setFlag(Z_FLAG, state.x_register == 0);
    setFlag(N_FLAG, state.x_register & 0x80);

    extraCycleCheck++;
}

void CPU::LDY() {
    //load Y
    state.y_register = absolute_data;

    setFlag(Z_FLAG, state.y_register == 0);
    setFlag(N_FLAG, state.y_register & 0x80);

    extraCycleCheck++;
}

void CPU::LSR() {
    //logical shift right
    if (accumulatorMode) {
        setFlag(C_FLAG, state.accumulator & 0x01);
        state.accumulator >>= 1;
        setFlag(Z_FLAG, state.accumulator == 0);
        setFlag(N_FLAG, state.accumulator & 0x80);
    } else {
        setFlag(C_FLAG, absolute_data & 0x01);
        emulator->cpuBusWrite(absolute_address, absolute_data >> 1);
        updateAbsolute();
        setFlag(Z_FLAG, absolute_data == 0);
        setFlag(N_FLAG, absolute_data & 0x80);
    }
}

void CPU::NOP() {
    //no operation
}

void CPU::NOPE() {
    //some nop's have extra cycle 
    extraCycleCheck++;

}

void CPU::ORA() {
    //or with accumulator
    state.accumulator = state.accumulator | absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);

    extraCycleCheck++;
}

void CPU::PHA() {
    //push accumulator
    pushStack(state.accumulator);
}

void CPU::PHP() {
    //push processor status (SR)
    pushStack((state.status_register | B_FLAG) | U_FLAG);
}

void CPU::PLA() {
    //pull accumulator
    state.accumulator = pullStack();

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}

void CPU::PLP() {
    //pull processor status (SR)
    state.status_register = pullStack();

    setFlag(U_FLAG, true);
    setFlag(B_FLAG, false);
}

void CPU::ROL() {
    //rotate left
    if (accumulatorMode) {
        uint8_t carry = state.status_register & C_FLAG;
        setFlag(C_FLAG, state.accumulator & 0x80);
        state.accumulator <<= 1;
        state.accumulator |= carry;
        setFlag(Z_FLAG, state.accumulator == 0);
        setFlag(N_FLAG, state.accumulator & 0x80);
    } else {
        uint8_t carry = state.status_register & C_FLAG;
        setFlag(C_FLAG, absolute_data & 0x80);
        emulator->cpuBusWrite(absolute_address, (absolute_data << 1) | carry);
        updateAbsolute();
        setFlag(Z_FLAG, absolute_data == 0);
        setFlag(N_FLAG, absolute_data & 0x80);
    }
}

void CPU::ROR() {
    //rotate right
    uint16_t rotation = (absolute_data >> 1) | ((state.status_register & C_FLAG) << 7);
    setFlag(C_FLAG, absolute_data & 0x01);
    if (accumulatorMode) {
        state.accumulator = rotation;
    } else {
        emulator->cpuBusWrite(absolute_address, rotation);
    }
    setFlag(Z_FLAG, rotation == 0);
    setFlag(N_FLAG, rotation & 0x80);
}

void CPU::RTI() {
    //return from interrupt
	state.status_register = pullStack();

	state.program_counter = pullStack();
	state.program_counter |= pullStack() << 8;

    setFlag(B_FLAG, false);
    setFlag(U_FLAG, true);
}

void CPU::RTS() {
    //return from subroutine
    state.program_counter = pullStack();
    state.program_counter |= pullStack() << 8;
    state.program_counter++;
}

void CPU::SBC() {
    //subtract with carry
    absolute_data = ~absolute_data;
    ADC();
}

void CPU::SEC() {
    //set carry
    setFlag(C_FLAG, true);
}

void CPU::SED() {
    //set decimal
    setFlag(D_FLAG, true);
}

void CPU::SEI() {
    //set interrupt disable
    setFlag(I_FLAG, true);
}

void CPU::STA() {
    //store accumulator
    emulator->cpuBusWrite(absolute_address, state.accumulator);
}

void CPU::STX() {
    //store X
    emulator->cpuBusWrite(absolute_address, state.x_register);
}

void CPU::STY() {
    //store Y
    emulator->cpuBusWrite(absolute_address, state.y_register);
}

void CPU::TAX() {
    //transfer accumulator to X
    state.x_register = state.accumulator;

    setFlag(Z_FLAG, state.x_register == 0);
    setFlag(N_FLAG, state.x_register & 0x80);
}

void CPU::TAY() {
    //transfer accumulator to Y
    state.y_register = state.accumulator;

    setFlag(Z_FLAG, state.y_register == 0);
    setFlag(N_FLAG, state.y_register & 0x80);
}

void CPU::TSX() {
    //transfer stack pointer to X
    state.x_register = state.stack_pointer;

    setFlag(Z_FLAG, state.x_register == 0);
    setFlag(N_FLAG, state.x_register & 0x80);
}

void CPU::TXA() {
    //transfer X to accumulator
    state.accumulator = state.x_register;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}

void CPU::TXS() {
    //transfer X to stack pointer
    state.stack_pointer = state.x_register;
}

void CPU::TYA() {
    //transfer Y to accumulator
    state.accumulator = state.y_register;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}




//illegal opcodes

void CPU::SLO() {
    //shift left OR
    ASL();
    updateAbsolute();
    ORA();
    extraCycleCheck = 0;
}

void CPU::ANC() {
    //and with carry
    AND();
    setFlag(C_FLAG, state.accumulator & 0x80);
    extraCycleCheck = 0;
}

void CPU::RLA() {
    //rotate left and AND
    ROL();
    updateAbsolute();
    AND();
    extraCycleCheck = 0;
}

void CPU::SRE() {
    //shift right exclusive OR
    LSR();
    updateAbsolute();
    EOR();
    extraCycleCheck = 0;
}

void CPU::ALR() {
    //and with carry, shift right
    AND();
    updateAbsolute();
    accumulatorMode = true;
    LSR();
    extraCycleCheck = 0;
}

void CPU::RRA() {
    //rotate right and add with carry
    ROR();
    updateAbsolute();
    ADC();
    extraCycleCheck = 0;
}

void CPU::ARR() {
    //and with accumulator, rotate right
    AND();
    accumulatorMode = true;
    updateAbsolute();
    ROR();
    setFlag(C_FLAG, state.accumulator & 0x40);
    setFlag(V_FLAG, (state.accumulator & 0x40) ^ ((state.accumulator & 0x20) << 1)); 
    extraCycleCheck = 0;
}

void CPU::SAX() {
    //store accumulator and X
    emulator->cpuBusWrite(absolute_address, state.accumulator & state.x_register);
}

void CPU::ANE() {
    //and accumulator with immediate, then transfer to X and Y
    state.accumulator = (state.accumulator | 0xEE) & state.x_register & absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}

void CPU::SHA() {
    //unstable
}

void CPU::TAS() {
    //unstable
}

void CPU::SHY() {
    //unstable
}

void CPU::SHX() {
    //unstable
}

void CPU::LXA() {
    //unstable
}

void CPU::LAX() {
    //load accumulator and X with memory
    state.accumulator = absolute_data;
    state.x_register = absolute_data;

    setFlag(Z_FLAG, absolute_data == 0);
    setFlag(N_FLAG, absolute_data & 0x80);

    extraCycleCheck++;
}

void CPU::DCP() {
    //decrement memory and compare
    DEC();
    updateAbsolute();
    CMP();
    extraCycleCheck = 0;
}

void CPU::ISC() {
    //increment memory and subtract from accumulator
    INC();
    updateAbsolute();
    SBC();
    extraCycleCheck = 0;
}

void CPU::LAS() {
    //load accumulator and stack pointer
    uint8_t load = absolute_data & state.stack_pointer;
    state.accumulator = load;
    state.x_register = load;
    state.stack_pointer = load;

    setFlag(Z_FLAG, load == 0);
    setFlag(N_FLAG, load & 0x80);

    extraCycleCheck++;
}

void CPU::SBX() {
    //subtract with X
    uint8_t base = state.accumulator & state.x_register;
    state.x_register = base - absolute_data;

    setFlag(C_FLAG, base >= absolute_data);
    setFlag(Z_FLAG, state.x_register == 0);
    setFlag(N_FLAG, state.x_register & 0x80);
}

void CPU::USBC() {
    //unofficial subtract with carry
    SBC();
}


//placeholder opccode

void CPU::XXX() {
    //used for jam mainly
}


void CPU::testOpcodes() {
    printf(GREEN "CPU: Testing CPU\n" RESET);

    //this function and all related code is meant to be removed once it has verified that all the opcodes are working perfectly

    using json = nlohmann::json;

    //hardcode opcode to test
    uint8_t testing = 0x00;
    bool breakOnFailure = false;

    int opcodePasses = 0;
    int opcodeFails = 0;
    int opcodeSkips = 0;

    for (int t = testing; t < 0x100; t++) {
        while (t == 0x93 || t == 0x9b || t == 0xab || t == 0x9f || t == 0x9e || t == 0x9c) //list of opcodes to skip due to instability, will implement later
        {
            printf(YELLOW "Skipping tests for opcode %X\n" RESET, t);
            opcodeSkips++;
            opcodeFails++;
            t++;
        }

        accumulatorMode = false;
        OpcodeInfo opcode = opcodeTable[t >> 4][t & 0x0F];

        //open file with tests for this opcode
        char filename[36];
        sprintf(filename, "../cpuTests/v1/%02x.json", t);
        FILE *testFile = fopen(filename, "r");

        if (testFile == NULL) {
            printf(RED "CPU: Error opening test file, %s\n" RESET, filename);
            return;
        }

        json tests = json::parse(testFile);

        //10000 tests per opcode
        int success = 0;
        int fail = 0;

        //each testfile has 10000 tests
        for (int i = 0; i < 10000; i++)
        {
            //clear ram 64kb
            for (int j = 0; j < 0x10000; j++) {
                emulator->testRam[j] = 0;
            }

            json test = tests[i];

            //set up initial cpu state
            state.accumulator = test["initial"]["a"];
            state.x_register = test["initial"]["x"];
            state.y_register = test["initial"]["y"];
            state.program_counter = test["initial"]["pc"];
            state.stack_pointer = test["initial"]["s"];
            state.status_register = test["initial"]["p"];

            //ram
            for (int j = 0; j < (uint8_t)test["initial"]["ram"].size(); j++) {
                emulator->testRam[test["initial"]["ram"][j][0].get<int>()] = test["initial"]["ram"][j][1].get<uint8_t>();
            }

            //run opcode
            (this->*opcode.AddrMode)();
            updateAbsolute();
            (this->*opcode.OpFunction)();
            accumulatorMode = false;

            //check if passed test
            if (test["final"]["a"] != state.accumulator) {
                printf(RED "CPU: Test %i failed, Accumulator was 0x%X instead of 0x%X\n" RESET, i, state.accumulator, (uint8_t)test["final"]["a"]);
                fail++;
            }
            else if (test["final"]["x"] != state.x_register) {
                printf(RED "CPU: Test %i failed, X register was 0x%X instead of 0x%X\n" RESET, i, state.x_register, (uint8_t)test["final"]["x"]);
                fail++;
            }
            else if (test["final"]["y"] != state.y_register) {
                printf(RED "CPU: Test %i failed, Y register was 0x%X instead of 0x%X\n" RESET, i, state.y_register, (uint8_t)test["final"]["y"]);
                fail++;
            }
            else if (test["final"]["pc"] != state.program_counter) {
                printf(RED "CPU: Test %i failed, Program counter was 0x%04X instead of 0x%04X\n" RESET, i, state.program_counter, (uint16_t)test["final"]["pc"]);
                fail++;
            }
            else if (test["final"]["s"] != state.stack_pointer) {
                printf(RED "CPU: Test %i failed, Stack pointer was 0x%X instead of 0x%X\n" RESET, i, state.stack_pointer, (uint8_t)test["final"]["s"]);
                fail++;
            }
            else if (test["final"]["p"] != state.status_register) {
                printf(RED "CPU: Test %i failed, Status register was 0x%X instead of 0x%X\n" RESET, i, state.status_register, (uint8_t)test["final"]["p"]);
                fail++;
            }
            else {
                success++;
            }

            for (int j = 0; j < (uint8_t)test["final"]["ram"].size(); j++) {
                int index = test["final"]["ram"][j][0];
                uint8_t value = test["final"]["ram"][j][1];
                if (emulator->testRam[index] != value) {
                    printf(RED "CPU: Test %i failed, Ram at %X was %X instead of %X\n" RESET, i, index, emulator->testRam[index], value);
                    fail++;
                }
            }

            if (fail > 0 && breakOnFailure) {
                printf("--------------------------------\n");
                printf("Initial State: \n");
                printf("A: 0x%X, X: 0x%X, Y: 0x%X, PC: 0x%04X, SP: 0x%X, P: 0x%X ", (uint8_t)test["initial"]["a"], (uint8_t)test["initial"]["x"], (uint8_t)test["initial"]["y"], (uint16_t)test["initial"]["pc"], (uint8_t)test["initial"]["s"], (uint8_t)test["initial"]["p"]);
                std::cout << test["initial"]["ram"] << std::endl;
                printf("Expected Final State: \n");
                printf("A: 0x%X, X: 0x%X, Y: 0x%X, PC: 0x%04X, SP: 0x%X, P: 0x%X ", (uint8_t)test["final"]["a"], (uint8_t)test["final"]["x"], (uint8_t)test["final"]["y"], (uint16_t)test["final"]["pc"], (uint8_t)test["final"]["s"], (uint8_t)test["final"]["p"]);
                std::cout << test["final"]["ram"] << std::endl;
                printf("Final State: \n");
                printf("A: 0x%X, X: 0x%X, Y: 0x%X, PC: 0x%X, SP: 0x%X, P: 0x%X ", state.accumulator, state.x_register, state.y_register, state.program_counter, state.stack_pointer, state.status_register);
                std::cout << "[";
                for (int i = 0; i < 0x10000; i++) {
                    if (emulator->testRam[i] != 0) {
                        std::cout << "[" << i << "," << (int)emulator->testRam[i] << "],";
                    }
                }
                std::cout << "]" << std::endl;
                printf("Correct Bus Calls: \n");
                for (int j = 0; j < (uint8_t)test["cycles"].size(); j++) {
                    std::cout << test["cycles"][j] << std::endl;
                }
                std::cout << test["name"] << std::endl;
                
                break;
            }
        }
        printf(BLUE "Total tests for opcode %02X: %i, Success: %i, Fail: %i\n" RESET, t, success + fail, success, fail);
        if (breakOnFailure && fail > 0) {
            break;
        }
        breakOnFailure = true;
        opcodePasses++;
    }
    reset();
    printf(GREEN "CPU: Opcode Tests:, %i passes, %i fails\n" RESET, opcodePasses, opcodeFails);
    printf(YELLOW "CPU: Skipped %i opcodes,count as failures\n" RESET, opcodeSkips);

}