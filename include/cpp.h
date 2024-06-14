#include <cstdint>
#include <unordered_map>

// Константы флагов состояния
const uint8_t FLAG_N = 0x80;  // отрицательный
const uint8_t FLAG_V = 0x40;  // переполнение
const uint8_t FLAG_Ig = 0x20; // игнора
const uint8_t FLAG_B = 0x10;  // прерывание
const uint8_t FLAG_D = 0x08;  // десятичное
const uint8_t FLAG_I = 0x04;  // запрет прерывания
const uint8_t FLAG_Z = 0x02;  // нулевой
const uint8_t FLAG_C = 0x01;  // перенос

class CPU {
public:
    void log_status();
    void reset();
    void execute();
    void turn_on();
    void turn_off();

private:
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    uint8_t fetch();
    uint16_t fetch16();
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
    void LSR(uint16_t address);
    void LSR_accumulator();
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
    // Регистры процессора
    uint16_t PC;       // Программный счетчик
    uint8_t SP;        // Указатель стека
    uint8_t A, X, Y;   // Регистры
    uint8_t status;    // Регистр состояния
    bool running;

    // Память
    std::array<uint8_t, 0x10000> memory;
};
