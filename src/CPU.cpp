#include "CPU.h"
#include <stdio.h>

CPU::CPU() {
    this->state.accumulator = 0;
    this->state.x_register = 0;
    this->state.y_register = 0;
    //start at C000 for nestest log
    this->state.program_counter = 0xC000;
    this->state.stack_pointer = 0;
    this->state.status_register = 0;

    this->state.remaining_cycles = 0;
}

CPU::~CPU() {
    
}

void CPU::reset() {
    printf(YELLOW "CPU: Reset\n" RESET);
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

void CPU::runInstruction() {
    //decode and run opcode at program counter

    //add cycle counts to remaining cycles
}

bool CPU::clock() {
    //run instruction if remaining cycles is zero, otherwise run instruction which will set the remaining cycles
    //true indicates instruction was run on this call
    if (state.remaining_cycles == 0) {
        runInstruction();
        return true;
    } else {
        state.remaining_cycles--;
        return false;
    }
}

//keep at bottom, cpu addressing modes and opcodes

//addressing modes

void CPU::IMPL() {
    //implied
}

void CPU::REL() {
    //relative
}

void CPU::ABS() {
    //absolute
}

void CPU::IMM() {
    //immediate
}

void CPU::XIND() {
    //X-indexed, indirect
}

void CPU::INDY() {
    //indirect, Y-indexed
}

void CPU::ZPG() {
    //zeropage
}

void CPU::ZPGX() {
    //zeropage x-indexed
}

void CPU::ZPGY() {
    //zeropage y-indexed
}

void CPU::ABSY() {
    //absolute, y-indexed
}

void CPU::ACC() {
    //accumulator
}

void CPU::IND() {
    //indirect
}

void CPU::ABSX() {
    //absolute x-indexed
}





// opcodes, legal first

void CPU::ADC() {
    //add with carry
}

void CPU::AND() {
    //and (with accumulator)
}

void CPU::ASL() {
    //arithmetic shift left
}

void CPU::BCC() {
    //branch on carry clear
}

void CPU::BCS() {
    //branch on carry set
}

void CPU::BEQ() {
    //branch on equal (zero set)
}

void CPU::BIT() {
    //bit test
}

void CPU::BMI() {
    //branch on minus (negative set)
}

void CPU::BNE() {
    //branch on not equal (zero clear)
}

void CPU::BPL() {
    //branch on plus (negative clear)
}

void CPU::BRK() {
    //force break
}

void CPU::BVC() {
    //branch on overflow clear
}

void CPU::BVS() {
    //branch on overflow set
}

void CPU::CLC() {
    //clear carry
}

void CPU::CLD() {
    //clear decimal
}

void CPU::CLI() {
    //clear interrupt disable
}

void CPU::CLV() {
    //clear overflow
}

void CPU::CMP() {
    //compare (with accumulator)
}

void CPU::CPX() {
    //compare with X
}

void CPU::CPY() {
    //compare with Y
}

void CPU::DEC() {
    //decrement
}

void CPU::DEX() {
    //decrement X
}

void CPU::DEY() {
    //decrement Y
}

void CPU::EOR() {
    //exclusive or (with accumulator)
}

void CPU::INC() {
    //increment
}

void CPU::INX() {
    //increment X
}

void CPU::INY() {
    //increment Y
}

void CPU::JMP() {
    //jump
}

void CPU::JSR() {
    //jump subroutine
}

void CPU::LDA() {
    //load accumulator
}

void CPU::LDX() {
    //load X
}

void CPU::LDY() {
    //load Y
}

void CPU::LSR() {
    //logical shift right
}

void CPU::NOP() {
    //no operation
}

void CPU::ORA() {
    //or with accumulator
}

void CPU::PHA() {
    //push accumulator
}

void CPU::PHP() {
    //push processor status (SR)
}

void CPU::PLA() {
    //pull accumulator
}

void CPU::PLP() {
    //pull processor status (SR)
}

void CPU::ROL() {
    //rotate left
}

void CPU::ROR() {
    //rotate right
}

void CPU::RTI() {
    //return from interrupt
}

void CPU::RTS() {
    //return from subroutine
}

void CPU::SBC() {
    //subtract with carry
}

void CPU::SEC() {
    //set carry
}

void CPU::SED() {
    //set decimal
}

void CPU::SEI() {
    //set interrupt disable
}

void CPU::STA() {
    //store accumulator
}

void CPU::STX() {
    //store X
}

void CPU::STY() {
    //store Y
}

void CPU::TAX() {
    //transfer accumulator to X
}

void CPU::TAY() {
    //transfer accumulator to Y
}

void CPU::TSX() {
    //transfer stack pointer to X
}

void CPU::TXA() {
    //transfer X to accumulator
}

void CPU::TXS() {
    //transfer X to stack pointer
}

void CPU::TYA() {
    //transfer Y to accumulator
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