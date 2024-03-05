// 6502 cpu
#pragma once
#include <cstdint>
#include <vector>
#include "Definitions.h"

class Emulator;

class CPU {
public:
    CPU(Emulator *emulator);
    ~CPU();

    void reset();
    CpuState *getState();
    int cycleCount = 0;
    void runInstruction();    
    bool clock();

private:
    CpuState state;

    Emulator *emulator;

    void setFlag(uint8_t flag, bool value);

    //addressing modes, in order of their documentation on https://www.masswerk.at/6502/6502_instruction_set.html
    void IMPL(); //Implied
    void REL(); //Relative
    void ABS(); //Absolute
    void IMM(); //Immediate
    void XIND(); //X-indexed, indirect
    void INDY(); //indirect, Y-indexed
    void ZPG(); //zeropage
    void ZPGX(); //zeropage x-indexed
    void ZPGY(); //zeropage y-indexed
    void ABSY(); //absolute, y-indeexed
    void ACC(); //accumulator
    void IND(); //indirect
    void ABSX(); //absolute x-indexed

    //actual opcode functions
    
    void ADC();
    void AND();
    void ASL();
    void BCC();
    void BCS();
    void BEQ();
    void BIT();
    void BMI();
    void BNE();
    void BPL();
    void BRK();
    void BVC();
    void BVS();
    void CLC();
    void CLD();
    void CLI();
    void CLV();
    void CMP();
    void CPX();
    void CPY();
    void DEC();
    void DEX();
    void DEY();
    void EOR();
    void INC();
    void INX();
    void INY();
    void JMP();
    void JSR();
    void LDA();
    void LDX();
    void LDY();
    void LSR();
    void NOP();
    void ORA();
    void PHA();
    void PHP();
    void PLA();
    void PLP();
    void ROL();
    void ROR();
    void RTI();
    void RTS();
    void SBC();
    void SEC();
    void SED();
    void SEI();
    void STA();
    void STX();
    void STY();
    void TAX();
    void TAY();
    void TSX();
    void TXA();
    void TXS();
    void TYA();

    //illegal opcode placeholder, like for JAM
    void XXX();

    //implemented illegal opcodes
    void SLO();
    void ANC();
    void RLA();
    void SRE();
    void ALR();
    void RRA();
    void ARR();
    void SAX();
    void ANE();
    void SHA();
    void TAS();
    void SHY();
    void SHX();
    void LXA();
    void LAX();
    void DCP();
    void ISC();
    void LAS();
    void SBX();
    void USBC();

    //opcode table with info
    struct OpcodeInfo {
        void (CPU::*OpFunction)();
        void (CPU::*AddrMode)();
        char mnemonic[5];
        uint8_t byteCount;
        uint8_t cycleCount;
    };

    //indexed [row][column]
    //also [firstNibble][secondNibble] when decoding opcodes
    const OpcodeInfo opcodeTable[16][16] = {
        //0                       //1                      //2                     //3                      //4                       //5                     //6                      //7                     //8                       //9                      //A                      //B                       //C                      //D                      //E                      //F
        {{&CPU::BRK, &CPU::IMPL, "BRK", 1, 7},{&CPU::ORA, &CPU::XIND, "ORA", 2, 6},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::SLO, &CPU::XIND, "SLO", 2, 8},{&CPU::NOP, &CPU::ZPG, "NOP", 2, 3}, {&CPU::ORA, &CPU::ZPG, "ORA", 2, 3}, {&CPU::ASL, &CPU::ZPG, "ASL", 2, 5}, {&CPU::SLO, &CPU::ZPG, "SLO", 2, 5}, {&CPU::PHP, &CPU::IMPL, "PHP", 1, 3},{&CPU::ORA, &CPU::IMM, "ORA", 2, 2}, {&CPU::ASL, &CPU::ACC, "ASL", 1, 2}, {&CPU::ANC, &CPU::IMM, "ANC", 2, 2},  {&CPU::NOP, &CPU::ABS, "NOP", 3, 4}, {&CPU::ORA, &CPU::ABS, "ORA", 3, 4}, {&CPU::ASL, &CPU::ABS, "ASL", 3, 6}, {&CPU::SLO, &CPU::ABS, "SLO", 3, 6}}, //0
        {{&CPU::BPL, &CPU::REL, "BPL", 2, 2}, {&CPU::ORA, &CPU::INDY, "ORA", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::SLO, &CPU::INDY, "SLO", 2, 8},{&CPU::NOP, &CPU::ZPGX, "NOP", 2, 4},{&CPU::ORA, &CPU::ZPGX, "ORA", 2, 4},{&CPU::ASL, &CPU::ZPGX, "ASL", 2, 6},{&CPU::SLO, &CPU::ZPGX, "SLO", 2, 6},{&CPU::CLC, &CPU::IMPL, "CLC", 1, 2},{&CPU::ORA, &CPU::ABSY, "ORA", 3, 4},{&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::SLO, &CPU::ABSY, "SLO", 3, 7}, {&CPU::NOP, &CPU::ABSX, "NOP", 3, 4},{&CPU::ORA, &CPU::ABSX, "ORA", 3, 4},{&CPU::ASL, &CPU::ABSX, "ASL", 3, 7},{&CPU::SLO, &CPU::ABSX, "SLO", 3, 7}}, //1
        {{&CPU::JSR, &CPU::ABS, "JSR", 3, 6}, {&CPU::AND, &CPU::XIND, "AND", 2, 6},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::RLA, &CPU::XIND, "RLA", 2, 8},{&CPU::BIT, &CPU::ZPG, "BIT", 2, 3}, {&CPU::AND, &CPU::ZPG, "AND", 2, 3}, {&CPU::ROL, &CPU::ZPG, "ROL", 2, 5}, {&CPU::RLA, &CPU::ZPG, "RLA", 2, 5}, {&CPU::PLP, &CPU::IMPL, "PLP", 1, 4},{&CPU::AND, &CPU::IMM, "AND", 2, 2}, {&CPU::ROL, &CPU::ACC, "ROL", 1, 2}, {&CPU::ANC, &CPU::IMM, "ANC", 2, 2},  {&CPU::BIT, &CPU::ABS, "BIT", 3, 4}, {&CPU::AND, &CPU::ABS, "AND", 3, 4}, {&CPU::ROL, &CPU::ABS, "ROL", 3, 6}, {&CPU::RLA, &CPU::ABS, "RLA", 3, 6}}, //2
        {{&CPU::BMI, &CPU::REL, "BMI", 2, 2}, {&CPU::AND, &CPU::INDY, "AND", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::RLA, &CPU::INDY, "RLA", 2, 8},{&CPU::NOP, &CPU::ZPGX, "NOP", 2, 4},{&CPU::AND, &CPU::ZPGX, "AND", 2, 4},{&CPU::ROL, &CPU::ZPGX, "ROL", 2, 6},{&CPU::RLA, &CPU::ZPGX, "RLA", 2, 6},{&CPU::SEC, &CPU::IMPL, "SEC", 1, 2},{&CPU::AND, &CPU::ABSY, "AND", 3, 4},{&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::RLA, &CPU::ABSY, "RLA", 3, 7}, {&CPU::NOP, &CPU::ABSX, "NOP", 3, 4},{&CPU::AND, &CPU::ABSX, "AND", 3, 4},{&CPU::ROL, &CPU::ABSX, "ROL", 3, 7},{&CPU::RLA, &CPU::ABSX, "RLA", 3, 7}}, //3
        {{&CPU::RTI, &CPU::IMPL, "RTI", 1, 6},{&CPU::EOR, &CPU::XIND, "EOR", 2, 6},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::SRE, &CPU::XIND, "SRE", 2, 8},{&CPU::NOP, &CPU::ZPG, "NOP", 2, 3}, {&CPU::EOR, &CPU::ZPG, "EOR", 2, 4}, {&CPU::LSR, &CPU::ZPG, "LSR", 2, 5}, {&CPU::SRE, &CPU::ZPG, "SRE", 2, 5}, {&CPU::PHA, &CPU::IMPL, "PHA", 1, 3},{&CPU::EOR, &CPU::IMM, "EOR", 2, 2}, {&CPU::LSR, &CPU::ACC, "LSR", 1, 2}, {&CPU::ALR, &CPU::IMM, "ALR", 2, 2},  {&CPU::JMP, &CPU::ABS, "JMP", 3, 3}, {&CPU::EOR, &CPU::ABS, "EOR", 3, 4}, {&CPU::LSR, &CPU::ABS, "LSR", 3, 6}, {&CPU::SRE, &CPU::ABS, "SRE", 3, 6}}, //4
        {{&CPU::BVC, &CPU::REL, "BVC", 2, 2}, {&CPU::EOR, &CPU::INDY, "EOR", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::SRE, &CPU::INDY, "SRE", 2, 8},{&CPU::NOP, &CPU::ZPGX, "NOP", 2, 4},{&CPU::EOR, &CPU::ZPGX, "EOR", 2, 4},{&CPU::LSR, &CPU::ZPGX, "LSR", 2, 6},{&CPU::SRE, &CPU::ZPGX, "SRE", 2, 6},{&CPU::CLI, &CPU::IMPL, "CLI", 1, 2},{&CPU::EOR, &CPU::ABSY, "EOR", 3, 4},{&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::SRE, &CPU::ABSY, "SRE", 3, 7}, {&CPU::NOP, &CPU::ABSX, "NOP", 3, 4},{&CPU::EOR, &CPU::ABSX, "EOR", 3, 4},{&CPU::LSR, &CPU::ABSX, "LSR", 3, 7},{&CPU::SRE, &CPU::ABSX, "SRE", 3, 7}}, //5
        {{&CPU::RTS, &CPU::IMPL, "RTS", 1, 6},{&CPU::ADC, &CPU::XIND, "ADC", 2, 6},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::RRA, &CPU::XIND, "RRA", 2, 8},{&CPU::NOP, &CPU::ZPG, "NOP", 2, 3}, {&CPU::ADC, &CPU::ZPG, "ADC", 2, 3}, {&CPU::ROR, &CPU::ZPG, "ROR", 2 , 5},{&CPU::RRA, &CPU::ZPG, "RRA", 2, 5}, {&CPU::PLA, &CPU::IMPL, "PLA", 1, 4},{&CPU::ADC, &CPU::IMM, "ADC", 2, 2}, {&CPU::ROR, &CPU::ACC, "ROR", 1, 2}, {&CPU::ARR, &CPU::IMM, "AAR", 2, 2},  {&CPU::JMP, &CPU::IND, "JMP", 3, 5}, {&CPU::ADC, &CPU::ABS, "ADC", 3, 4}, {&CPU::ROR, &CPU::ABS, "ROR", 3, 6}, {&CPU::RRA, &CPU::ABS, "RRA", 3, 6}}, //6
        {{&CPU::BVS, &CPU::REL, "BVS", 2, 2}, {&CPU::ADC, &CPU::INDY, "ADC", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::RRA, &CPU::INDY, "RRA", 2, 8},{&CPU::NOP, &CPU::ZPGX, "NOP", 2, 4},{&CPU::ADC, &CPU::ZPGX, "ADC", 2, 4},{&CPU::ROR, &CPU::ZPGX, "ROR", 2, 6},{&CPU::RRA, &CPU::ZPGX, "RRA", 2, 6},{&CPU::SEI, &CPU::IMPL, "SEI", 1, 2},{&CPU::ADC, &CPU::ABSY, "ADC", 3, 4},{&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::RRA, &CPU::ABSY, "RRA", 3, 7}, {&CPU::NOP, &CPU::ABSX, "NOP", 3, 4},{&CPU::ADC, &CPU::ABSX, "ADC", 3, 4},{&CPU::ROR, &CPU::ABSX, "ROR", 3, 7},{&CPU::RRA, &CPU::ABSX, "RRA", 3, 7}}, //7
        {{&CPU::NOP, &CPU::IMM, "NOP", 2, 2}, {&CPU::STA, &CPU::XIND, "STA", 2, 6},{&CPU::NOP, &CPU::IMM, "NOP", 2, 2},{&CPU::SAX, &CPU::XIND, "SAX", 2, 6},{&CPU::STY, &CPU::ZPG, "STY", 2, 3}, {&CPU::STA, &CPU::ZPG, "STA", 2, 3}, {&CPU::STX, &CPU::ZPG, "STX", 2, 3}, {&CPU::SAX, &CPU::ZPG, "SAX", 2, 3}, {&CPU::DEY, &CPU::IMPL, "DEY", 1, 2},{&CPU::NOP, &CPU::IMM, "NOP", 2, 2}, {&CPU::TXA, &CPU::IMPL, "TXA", 1, 2},{&CPU::ANE, &CPU::IMM, "ANE", 2, 2},  {&CPU::STY, &CPU::ABS, "STY", 3, 4}, {&CPU::STA, &CPU::ABS, "STA", 3, 4}, {&CPU::STX, &CPU::ABS, "STX", 3, 4}, {&CPU::SAX, &CPU::ABS, "SAX", 3, 4}}, //8
        {{&CPU::BCC, &CPU::REL, "BCC", 2, 2}, {&CPU::STA, &CPU::INDY, "STA", 2, 6},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::SHA, &CPU::INDY, "SHA", 2, 6},{&CPU::STY, &CPU::ZPGX, "STY", 2, 4},{&CPU::STA, &CPU::ZPGX, "STA", 2, 4},{&CPU::STX, &CPU::ZPGY, "STX", 2, 4},{&CPU::SAX, &CPU::ZPGY, "SAX", 2, 4},{&CPU::TYA, &CPU::IMPL, "TYA", 1, 2},{&CPU::STA, &CPU::ABSY, "STA", 3, 5},{&CPU::TXS, &CPU::IMPL, "TXS", 1, 2},{&CPU::TAS, &CPU::ABSY, "TAS", 3, 5}, {&CPU::SHY, &CPU::ABSX, "SHY", 3, 5},{&CPU::STA, &CPU::ABSX, "STA", 3, 5},{&CPU::SHX, &CPU::ABSY, "SHX", 3, 5},{&CPU::SHA, &CPU::ABSY, "SHA", 3, 5}}, //9
        {{&CPU::LDY, &CPU::IMM, "LDY", 2, 2}, {&CPU::LDA, &CPU::XIND, "LDA", 2, 6},{&CPU::LDX, &CPU::IMM, "LDX", 2, 2},{&CPU::LAX, &CPU::XIND, "LAX", 2, 6},{&CPU::LDY, &CPU::ZPG, "LDY", 2, 3}, {&CPU::LDA, &CPU::ZPG, "LDA", 2, 3}, {&CPU::LDX, &CPU::ZPG, "LDX", 2, 3}, {&CPU::LAX, &CPU::ZPG, "LAX", 2, 3}, {&CPU::TAY, &CPU::IMPL, "TAY", 1, 2},{&CPU::LDA, &CPU::IMM, "LDA", 2, 2}, {&CPU::TAX, &CPU::IMPL, "TAX", 1, 2},{&CPU::LXA, &CPU::IMM, "LXA", 2, 2},  {&CPU::LDY, &CPU::ABS, "LDY", 3, 4}, {&CPU::LDA, &CPU::ABS, "LDA", 3, 4}, {&CPU::LDX, &CPU::ABS, "LDX", 3, 4}, {&CPU::LAX, &CPU::ABS, "LAX", 3, 4}}, //A
        {{&CPU::BCS, &CPU::REL, "BCS", 2, 2}, {&CPU::LDA, &CPU::INDY, "LDA", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::LAX, &CPU::INDY, "LAX", 2, 5},{&CPU::LDY, &CPU::ZPGX, "LDY", 2, 4},{&CPU::LDA, &CPU::ZPGX, "LDA", 2, 4},{&CPU::LDX, &CPU::ZPGY, "LDX", 2, 4},{&CPU::LAX, &CPU::ZPGY, "LAX", 2, 4},{&CPU::CLV, &CPU::IMPL, "CLV", 1, 2},{&CPU::LDA, &CPU::ABSY, "LDA", 3, 4},{&CPU::TSX, &CPU::IMPL, "TSX", 1, 2},{&CPU::LAS, &CPU::ABSY, "LAS", 3, 4}, {&CPU::LDY, &CPU::ABSX, "LDY", 3, 4},{&CPU::LDA, &CPU::ABSX, "LDA", 3, 4},{&CPU::LDX, &CPU::ABSY, "LDX", 3, 4},{&CPU::LAX, &CPU::ABSY, "LAX", 3, 4}}, //B
        {{&CPU::CPY, &CPU::IMM, "CPY", 2, 2}, {&CPU::CMP, &CPU::XIND, "CMP", 2, 6},{&CPU::NOP, &CPU::IMM, "NOP", 2, 2},{&CPU::DCP, &CPU::XIND, "DCP", 2, 8},{&CPU::CPY, &CPU::ZPG, "CPY", 2, 3}, {&CPU::CMP, &CPU::ZPG, "CMP", 2, 3}, {&CPU::DEC, &CPU::ZPG, "DEC", 2, 5}, {&CPU::DCP, &CPU::ZPG, "DCP", 2, 5}, {&CPU::INY, &CPU::IMPL, "INY", 1, 2},{&CPU::CMP, &CPU::IMM, "CMP", 2, 2}, {&CPU::DEX, &CPU::IMPL, "DEX", 1, 2},{&CPU::SBX, &CPU::IMM, "SBX", 2, 2},  {&CPU::CPY, &CPU::ABS, "CPY", 3, 4}, {&CPU::CMP, &CPU::ABS, "CMP", 3, 4}, {&CPU::DEC, &CPU::ABS, "DEC", 3, 6}, {&CPU::DCP, &CPU::ABS, "DCP", 3, 6}}, //C
        {{&CPU::BNE, &CPU::REL, "BNE", 2, 2}, {&CPU::CMP, &CPU::XIND, "CMP", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::DCP, &CPU::INDY, "DCP", 2, 8},{&CPU::NOP, &CPU::ZPGX, "NOP", 2, 4},{&CPU::CMP, &CPU::ZPGX, "CMP", 2, 4},{&CPU::DEC, &CPU::ZPGX, "DEC", 2, 6},{&CPU::DCP, &CPU::ZPGX, "DCP", 2, 6},{&CPU::CLD, &CPU::IMPL, "CLD", 1, 2},{&CPU::CMP, &CPU::ABSY, "CMP", 3, 4},{&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::DCP, &CPU::ABSY, "DCP", 3, 7}, {&CPU::NOP, &CPU::ABSX, "NOP", 3, 4},{&CPU::CMP, &CPU::ABSX, "CMP", 3, 4},{&CPU::DEC, &CPU::ABSX, "DEC", 3, 7},{&CPU::DCP, &CPU::ABSX, "DCP", 3, 7}}, //D
        {{&CPU::CPX, &CPU::IMM, "CPX", 2, 2}, {&CPU::SEC, &CPU::XIND, "SEC", 2, 6},{&CPU::NOP, &CPU::IMM, "NOP", 2, 2},{&CPU::ISC, &CPU::XIND, "ISC", 2, 8},{&CPU::CPX, &CPU::ZPG, "CPX", 2, 3}, {&CPU::SBC, &CPU::ZPG, "SBC", 2, 3}, {&CPU::INC, &CPU::ZPG, "INC", 2, 5}, {&CPU::ISC, &CPU::ZPG, "ISC", 2, 5}, {&CPU::INX, &CPU::IMPL, "INX", 1, 2},{&CPU::SBC, &CPU::IMM, "SBC", 2, 2}, {&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::USBC,&CPU::IMM, "USBC", 2, 2}, {&CPU::CPX, &CPU::ABS, "CPX", 3, 4}, {&CPU::SBC, &CPU::ABS, "SBC", 3, 4}, {&CPU::INC, &CPU::ABS, "INC", 3, 6}, {&CPU::ISC, &CPU::ABS, "ISC", 3, 6}}, //E
        {{&CPU::BEQ, &CPU::REL, "BEQ", 2, 2}, {&CPU::SEC, &CPU::INDY, "SEC", 2, 5},{&CPU::XXX, &CPU::XXX, "JAM", 0, 0},{&CPU::ISC, &CPU::INDY, "ISC", 2, 8},{&CPU::NOP, &CPU::ZPGX, "NOP", 2, 4},{&CPU::SBC, &CPU::ZPGX, "SBC", 2, 4},{&CPU::INC, &CPU::ZPGX, "INC", 2, 6},{&CPU::ISC, &CPU::ZPGX, "ISC", 2, 6},{&CPU::SED, &CPU::IMPL, "SED", 1, 2},{&CPU::SBC, &CPU::ABSY, "SBC", 3, 4},{&CPU::NOP, &CPU::IMPL, "NOP", 1, 2},{&CPU::ISC, &CPU::ABSY, "LSC", 3, 7}, {&CPU::NOP, &CPU::ABSX, "NOP", 3, 4},{&CPU::SBC, &CPU::ABSX, "SBC", 3, 4},{&CPU::INC, &CPU::ABSX, "INC", 3, 7},{&CPU::ISC,&CPU::ABSX, "ISC", 3, 7}} //F
    };
};