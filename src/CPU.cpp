#include "CPU.h"
#include <stdio.h>
#include "Emulator.h"
#include <cstring>

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
}

CpuState *CPU::getState() {
    return &state;
}

void CPU::setFlag(uint8_t flag, bool value) {
    if (value) {
        state.status_register |= flag;
    } else {
        state.status_register &= flag;
    }
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

void CPU::cpuLog(OpcodeInfo opcode) {
    if (emulator->logging == false) {
        return;
    }

    //log information for opcode that is about to run, if the log ends here the error occured in that opcode
    char logMessage[100]; 
    sprintf(logMessage, "Opcode: %s",opcode.mnemonic);
    for (int i = 0; i < opcode.byteCount; i++) {
        sprintf(logMessage + strlen(logMessage), " %X", emulator->cpuBusRead(state.program_counter + i));
    }
    sprintf(logMessage + strlen(logMessage), " A: 0x%X X: 0x%X Y: 0x%X SP: 0x%X PC: 0x%X P: 0x%X CYC: %i\n", state.accumulator, state.x_register, state.y_register, state.stack_pointer, state.program_counter, state.status_register, cycleCount);
    emulator->log(logMessage);
}

void CPU::runInstruction() {
    //decode and run opcode at program counter
    uint8_t opcodeByte = emulator->cpuBusRead(state.program_counter);
    OpcodeInfo opcode = opcodeTable[opcodeByte >> 4][opcodeByte & 0x0F];
    printf(BLUE "CPU: Running opcode: 0x%X, %s\n" RESET, opcodeByte, opcode.mnemonic);

    cpuLog(opcode);

    //add cycle counts to remaining cycles
    state.remaining_cycles += opcode.cycleCount;

    //calculate address of interest
    (this->*opcode.AddrMode)();

    //addressing mode has changed absolute_address to point to the operand, store the operand now for use in the opcodes
    updateAbsolute();

    //run the opcode
    (this->*opcode.OpFunction)();

    //ensure accumulator mode is reset, because the opcodes dont reset it and they need to know
    accumulatorMode = false;
}

bool CPU::clock() {
    //run instruction if remaining cycles is zero, otherwise run instruction which will set the remaining cycles
    //true indicates instruction was run on this call

    if (state.remaining_cycles == 0) {
        runInstruction();
        cycleCount++;
        return true;
    } else {
        state.remaining_cycles--;
        cycleCount++;
        return false;
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
}

void CPU::ABS() {
    //absolute, full address is provided
    state.program_counter++;

    //pull each byte of address
    uint16_t low = emulator->cpuBusRead(state.program_counter);
    state.program_counter++;

    uint16_t high = emulator->cpuBusRead(state.program_counter) << 8;
    state.program_counter++;

    absolute_address = high | low;
}

void CPU::IMM() {
    //immediate, next byte is the argument
    absolute_address = state.program_counter + 1;
    state.program_counter += 2;
}

void CPU::XIND() {
    //X-indexed, indirect
    state.program_counter++;
    absolute_address = (emulator->cpuBusRead(state.program_counter + state.x_register + 1) << 8) | emulator->cpuBusRead(state.program_counter + state.x_register);
    state.program_counter++;
}

void CPU::INDY() {
    //indirect, Y-indexed
    state.program_counter++;
    absolute_address = ((emulator->cpuBusRead(state.program_counter + 1) + state.y_register ) << 8) | (emulator->cpuBusRead(state.program_counter) + state.y_register);
    state.program_counter++;
}

void CPU::ZPG() {
    //zeropage
    state.program_counter++;
    absolute_address= emulator->cpuBusRead(state.program_counter);
    state.program_counter++;
}

void CPU::ZPGX() {
    //zeropage x-indexed
    state.program_counter++;
    absolute_address= emulator->cpuBusRead(state.program_counter) + state.x_register;
    state.program_counter++;
}

void CPU::ZPGY() {
    //zeropage y-indexed
    state.program_counter++;
    absolute_address= emulator->cpuBusRead(state.program_counter) + state.y_register;
    state.program_counter++;
}

void CPU::ABSY() {
    //absolute, y-indexed
    ABS();
    absolute_address += state.y_register;
}

void CPU::ACC() {
    //accumulator, use tracking variable for opcodes with accumuilator and also another addressing mode
    accumulatorMode = true;
}

void CPU::IND() {
    //indirect
    ABS();
    absolute_address = (emulator->cpuBusRead(absolute_address + 1) << 8) & emulator->cpuBusRead(absolute_address);
}

void CPU::ABSX() {
    //absolute x-indexed
    ABS();
    absolute_address += state.x_register;
}





// opcodes, legal first

void CPU::ADC() {
    //add with carry
    state.accumulator += absolute_data + (state.status_register & C_FLAG);

    setFlag(C_FLAG, state.accumulator > 0xFF);
    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
    setFlag(V_FLAG, (~(absolute_data ^ state.accumulator) & (absolute_data ^ state.accumulator) & 0x80));
}

void CPU::AND() {
    //and (with accumulator)
    state.accumulator &= absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
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
        state.program_counter = absolute_address;
    }
}

void CPU::BCS() {
    //branch on carry set
    if ((state.status_register & C_FLAG) == C_FLAG) {
        state.program_counter = absolute_address;
    }
}

void CPU::BEQ() {
    //branch on zero set
    if ((state.status_register & Z_FLAG) == Z_FLAG) {
        state.program_counter = absolute_address;
    }
}

void CPU::BIT() {
    //bit test
    setFlag(Z_FLAG, (absolute_data & state.accumulator) == 0);
    setFlag(N_FLAG, absolute_data & 0b01000000);
    setFlag(V_FLAG, absolute_data & 0b00100000);
}

void CPU::BMI() {
    //branch on minus (negative set)
    if ((state.status_register & N_FLAG) == N_FLAG) {
        state.program_counter = absolute_address;
    }
}

void CPU::BNE() {
    //branch on not equal (zero clear)
    if ((state.status_register & Z_FLAG) == 0x00) {
        state.program_counter = absolute_address;
    }
}

void CPU::BPL() {
    //branch on plus (negative clear)
    if ((state.status_register & N_FLAG) == 0x00) {
        state.program_counter = absolute_address;
    }
}

void CPU::BRK() {
    //force break
    setFlag(I_FLAG, true);

    //opcode table doesnt include brk break mark byte
    state.program_counter++;

    //push next instruction to run onto the stack, already incremented earlier
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, (state.program_counter >> 8) & 0x00FF);
    state.stack_pointer--;
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, state.program_counter & 0x00FF);
    state.stack_pointer--;

    //requires status register as well
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, state.status_register);
    state.stack_pointer--;

    //break vector for PC
    state.program_counter = emulator->cpuBusRead(0xFFFE) | (emulator->cpuBusRead(0xFFFF) << 8);
}

void CPU::BVC() {
    //branch on overflow clear
    if ((state.status_register & V_FLAG) == 0x00) {
        state.program_counter = absolute_address;
    }
}

void CPU::BVS() {
    //branch on overflow set
    if ((state.status_register & V_FLAG) == V_FLAG) {
        state.program_counter = absolute_address;
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
    setFlag(C_FLAG, state.accumulator >= absolute_data);
    setFlag(Z_FLAG, state.accumulator == absolute_data);
    setFlag(N_FLAG, (state.accumulator - absolute_data) & 0x80);
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
}

void CPU::DEX() {
    //decrement X
    state.x_register--;
}

void CPU::DEY() {
    //decrement Y
    state.y_register--;
}

void CPU::EOR() {
    //exclusive or (with accumulator)
    state.accumulator ^= absolute_data;
}

void CPU::INC() {
    //increment
    emulator->cpuBusWrite(absolute_address, absolute_data + 1);
}

void CPU::INX() {
    //increment X
    state.x_register++;
}

void CPU::INY() {
    //increment Y
    state.y_register++;
}

void CPU::JMP() {
    //jump
    state.program_counter = absolute_address;
}

void CPU::JSR() {
    //jump subroutine

    emulator->cpuBusWrite(0x0100 + state.stack_pointer, (state.program_counter >> 8) & 0x00FF);
    state.stack_pointer--;
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, state.program_counter & 0x00FF);
    state.stack_pointer--;

    state.program_counter = absolute_address;
}

void CPU::LDA() {
    //load accumulator
    state.accumulator = absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}

void CPU::LDX() {
    //load X
    state.x_register = absolute_data;

    setFlag(Z_FLAG, state.x_register == 0);
    setFlag(N_FLAG, state.x_register & 0x80);
}

void CPU::LDY() {
    //load Y
    state.y_register = absolute_data;

    setFlag(Z_FLAG, state.y_register == 0);
    setFlag(N_FLAG, state.y_register & 0x80);
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

void CPU::ORA() {
    //or with accumulator
    state.accumulator |= absolute_data;

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}

void CPU::PHA() {
    //push accumulator
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, state.accumulator);
    state.stack_pointer--;
}

void CPU::PHP() {
    //push processor status (SR)
    emulator->cpuBusWrite(0x0100 + state.stack_pointer, (state.status_register | B_FLAG) | U_FLAG);
}

void CPU::PLA() {
    //pull accumulator
    state.stack_pointer++;
    state.accumulator = emulator->cpuBusRead(0x0100 + state.stack_pointer);

    setFlag(Z_FLAG, state.accumulator == 0);
    setFlag(N_FLAG, state.accumulator & 0x80);
}

void CPU::PLP() {
    //pull processor status (SR)
    state.stack_pointer++;
    state.status_register = emulator->cpuBusRead(0x0100 + state.stack_pointer);
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
    if (accumulatorMode) {
        uint8_t carry = state.status_register & C_FLAG;
        setFlag(C_FLAG, state.accumulator & 0x01);
        state.accumulator >>= 1;
        state.accumulator |= carry << 7;
        setFlag(Z_FLAG, state.accumulator == 0);
        setFlag(N_FLAG, state.accumulator & 0x80);
    } else {
        uint8_t carry = state.status_register & C_FLAG;
        setFlag(C_FLAG, absolute_data & 0x01);
        emulator->cpuBusWrite(absolute_address, (absolute_data >> 1) | (carry << 7));
        updateAbsolute();
        setFlag(Z_FLAG, absolute_data == 0);
        setFlag(N_FLAG, absolute_data & 0x80);
    }
}

void CPU::RTI() {
    //return from interrupt
    state.stack_pointer++;
    state.status_register = emulator->cpuBusRead(0x0100 + state.stack_pointer);
    state.stack_pointer++;
    state.program_counter = emulator->cpuBusRead(0x0100 + state.stack_pointer);
    state.stack_pointer++;

    state.program_counter |= emulator->cpuBusRead(0x0100 + state.stack_pointer) << 8;
    state.stack_pointer++;

    setFlag(B_FLAG, false);
}

void CPU::RTS() {
    //return from subroutine
    state.stack_pointer++;
    state.program_counter = emulator->cpuBusRead(0x0100 + state.stack_pointer);
    state.stack_pointer++;
    state.program_counter |= emulator->cpuBusRead(0x0100 + state.stack_pointer) << 8;
    state.program_counter++;
}

void CPU::SBC() {
    //subtract with carry
    state.accumulator -= absolute_data - (1 - (state.status_register & C_FLAG));
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
}

void CPU::TXS() {
    //transfer X to stack pointer
    state.stack_pointer = state.x_register;

    setFlag(Z_FLAG, state.stack_pointer == 0);
    setFlag(N_FLAG, state.stack_pointer & 0x80);
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
}

void CPU::ANC() {
    //and with carry
}

void CPU::RLA() {
    //rotate left and AND
}

void CPU::SRE() {
    //shift right exclusive OR
}

void CPU::ALR() {
    //and with carry, shift right
}

void CPU::RRA() {
    //rotate right and add with carry
}

void CPU::ARR() {
    //and with accumulator, rotate right
}

void CPU::SAX() {
    //store accumulator AND X
}

void CPU::ANE() {
    //and accumulator with immediate, then transfer to X and Y
}

void CPU::SHA() {
    //and X with high byte of address, then store accumulator
}

void CPU::TAS() {
    //transfer accumulator to stack pointer
}

void CPU::SHY() {
    //and Y with high byte of address, then store X
}

void CPU::SHX() {
    //and X with high byte of address, then store X
}

void CPU::LXA() {
    //load X with immediate and AND with accumulator
}

void CPU::LAX() {
    //load accumulator and X with memory
}

void CPU::DCP() {
    //decrement memory and compare
}

void CPU::ISC() {
    //increment memory and subtract from accumulator
}

void CPU::LAS() {
    //load accumulator and stack pointer
}

void CPU::SBX() {
    //subtract with X
}

void CPU::USBC() {
    //unofficial subtract with carry
}


//placeholder opccode

void CPU::XXX() {
    
}