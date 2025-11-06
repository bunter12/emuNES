#ifndef CPU_H
#define CPU_H
#include <cstdint>
#include <unordered_map>
#include <array>

const uint8_t FLAG_N = 0x80; 
const uint8_t FLAG_V = 0x40;  
const uint8_t FLAG_Ig = 0x20; 
const uint8_t FLAG_B = 0x10; 
const uint8_t FLAG_D = 0x08; 
const uint8_t FLAG_I = 0x04; 
const uint8_t FLAG_Z = 0x02; 
const uint8_t FLAG_C = 0x01;  

class CPU {
public:
    void log_status();
    void reset();
    void turn_off();
    void turn_on();
    void execute();
    bool Getflag(uint8_t flag);
    void Setflag(uint8_t flag, bool value);
    void ADC(uint8_t operand);
    void AND(uint8_t operand);
    void ASL_accumulator();
    void ASL(uint16_t address);
    void BCC(uint8_t offset);
    void BCS(uint8_t offset);
    void BEQ(uint8_t offset);
    void BIT(uint16_t address);
    void BMI(uint8_t offset);
    void BNE(uint8_t offset);
    void BPL(uint8_t offset);
    void BRK();
    void BVC(uint8_t offset);
    void BVS(uint8_t offset);
    void CLC();
    void CLD();
    void CLI();
    void CLV();
    void CMP(uint8_t operand);
    void CPX(uint8_t operand);
    void CPY(uint8_t operand);
    void DEC(uint16_t address);
    void DEX();
    void DEY();
    void EOR(uint8_t operand);
    void INC(uint16_t address);
    void INX();
    void INY();
    void JMP(uint16_t address);
    void JMP_indirect(uint16_t address);
    void JSR(uint16_t address);
    void LDA(uint8_t operand);
    void LDX(uint8_t operand);
    void LDY(uint8_t operand);
    void LSR_accumulator();
    void LSR(uint16_t address);
    void ORA(uint8_t operand);
    void PHA();
    void PHP();
    void PLA();
    void PLP();
    void ROL_accumulator();
    void ROL(uint16_t address);
    void ROR_accumulator();
    void ROR(uint16_t address);
    void RTI();
    void RTS();
    void SBC(uint8_t operand);
    void SEC();
    void SED();
    void SEI();
    void STA(uint16_t address);
    void STX(uint16_t address);
    void STY(uint16_t address);
    void TAX();
    void TAY();
    void TSX();
    void TXA();
    void TXS();
    void TYA();

    uint16_t PC;       
    uint8_t SP;        
    uint8_t A, X, Y;  
    uint8_t status;  
    bool running;

    std::array<uint8_t, 0x10000> memory;

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    uint8_t fetch();
    uint16_t fetch16();
    void stack_push(uint8_t value);
    void stack_push16(uint16_t value);
    uint8_t stack_pop();
    uint16_t stack_pop16(); 
    uint16_t read16(uint16_t address);
};

#endif //CPU_H
