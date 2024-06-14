#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <cpp.h>

//флаги состояния
const uint8_t FLAG_N = 0x80;  // отрицательный
const uint8_t FLAG_V = 0x40;  // переполнение
const uint8_t FLAG_Ig = 0x20; // игнора
const uint8_t FLAG_B = 0x10;  // прерывание
const uint8_t FLAG_D = 0x08;  // десятичное
const uint8_t FLAG_I = 0x04;  // запрет прерывания
const uint8_t FLAG_Z = 0x02;  // нулевой
const uint8_t FLAG_C = 0x01;  // переноса


uint8_t CPU::read(uint16_t address) {
    return memory[address];
}

void CPU::write(uint16_t address, uint8_t value) {
    memory[address] = value;
}

uint8_t CPU::fetch() {
    return read(PC++);
}

uint16_t CPU::fetch16() {
    uint16_t low = fetch();
    uint16_t high = fetch();
    return (high << 8) | low;
}

bool CPU::Getflag(uint8_t flag) {
    return (status & flag) != 0;
}

void CPU::Setflag(uint8_t flag, bool value) {
    if (value)
        status |= flag;
    else
        status &= ~flag;
}

void CPU::log_status() {
    std::cout << "PC: " << std::hex << PC << " A: " << std::hex << +A
        << " X: " << std::hex << +X << " Y: " << std::hex << +Y
        << " SP: " << std::hex << +SP << " Status: " << std::hex << +status << std::endl;
}

void CPU::reset() {
    PC = (read(0xFFFD) << 8) | read(0xFFFC); // Считывание адреса начального выполнения
    SP = 0xFD;
    A, X, Y = 0;
    running = true;
    status = 0x00;
}

void CPU::turn_off() {
    running = false;
}

void CPU::turn_on() {
    running = true;
}

void CPU::execute() {
    uint8_t opcode = fetch();
    while (running) {
        switch (opcode) {
        case 0x69:
            ADC(fetch());
            break;
        case 0x65:
            ADC(read(fetch()));
            break;
        case 0x75:
            ADC(read(fetch() + X));
            break;
        case 0x6D:
            ADC(read(fetch16()));
            break;
        case 0x7D:
            ADC(read(fetch16() + X));
            break;
        case 0x79:
            ADC(read(fetch16() + Y));
            break;
        case 0x61:
            ADC(read((fetch() + X) & 0xFF));
            break;
        case 0x71:
            ADC(read((fetch() & 0xFF) + Y));
            break;
        case 0x29:
            AND(fetch());
            break;
        case 0x25:
            AND(read(fetch()));
            break;
        case 0x35:
            AND(read(fetch() + X));
            break;
        case 0x2D:
            AND(read(fetch16()));
            break;
        case 0x3D:
            AND(read(fetch16() + X));
            break;
        case 0x39:
            AND(read(fetch16() + Y));
            break;
        case 0x21:
            AND(read((fetch() + X) & 0xFF));
            break;
        case 0x31:
            AND(read((fetch() & 0xFF) + Y));
            break;
        case 0x0A:
            ASL_accumulator();
            break;
        case 0x06:
            ASL(fetch());
            break;
        case 0x16:
            ASL(fetch() + X);
            break;
        case 0x0E:
            ASL(fetch16());
            break;
        case 0x1E:
            ASL(fetch16() + X);
            break;
        case 0x90:
            BCC(fetch());
            break;
        case 0xB0:
            BCS(fetch());
            break;
        case 0xF0:
            BEQ(fetch());
            break;
        case 0x24:
            BIT(fetch());
            break;
        case 0x2C:
            BIT(fetch16());
            break;
        case 0x30:
            BMI(fetch());
            break;
        case 0xD0:
            BNE(fetch());
            break;
        case 0x10:
            BPL(fetch());
            break;
        case 0x00:
            BRK();
            break;
        case 0x50:
            BVC(fetch());
            break;
        case 0x70:
            BVS(fetch());
            break;
        case 0x18:
            CLC();
            break;
        case 0xD8:
            CLD();
            break;
        case 0x58:
            CLI();
            break;
        case 0xB8:
            CLV();
            break;
        case 0xC9:
            CMP(fetch());
            break;
        case 0xC5:
            CMP(read(fetch()));
            break;
        case 0xD5:
            CMP(read(fetch() + X));
            break;
        case 0xCD:
            CMP(read(fetch16()));
            break;
        case 0xDD:
            CMP(read(fetch16() + X));
            break;
        case 0xD9:
            CMP(read(fetch16() + Y));
            break;
        case 0xC1:
            CMP(read((fetch() + X) & 0xFF));
            break;
        case 0xD1:
            CMP(read((fetch() & 0xFF) + Y));
            break;
        case 0xE0:
            CPX(fetch());
            break;
        case 0xE4:
            CPX(read(fetch()));
            break;
        case 0xEC:
            CPX(fetch16());
            break;
        case 0xC0:
            CPY(fetch());
            break;
        case 0xC4:
            CPY(read(fetch()));
            break;
        case 0xCC:
            CPY(fetch16());
            break;
        case 0xC6:
            DEC(fetch());
            break;
        case 0xD6:
            DEC(fetch() + X);
            break;
        case 0xCE:
            DEC(fetch16());
            break;
        case 0xDE:
            DEC(fetch16() + X);
            break;
        case 0xCA:
            DEX();
            break;
        case 0x88:
            DEY();
            break;
        case 0x49:
            EOR(fetch());
            break;
        case 0x45:
            EOR(read(fetch()));
            break;
        case 0x55:
            EOR(read(fetch() + X));
            break;
        case 0x4D:
            EOR(read(fetch16()));
            break;
        case 0x5D:
            EOR(read(fetch16() + X));
            break;
        case 0x59:
            EOR(read(fetch16() + Y));
            break;
        case 0x41:
            EOR(read((fetch() + X) & 0xFF));
            break;
        case 0x51:
            EOR(read((fetch() & 0xFF) + Y));
            break;
        case 0xE6:
            INC(fetch());
            break;
        case 0xF6:
            INC(fetch() + X);
            break;
        case 0xEE:
            INC(fetch16());
            break;
        case 0xFE:
            INC(fetch16() + X);
            break;
        case 0xE8:
            INX();
            break;
        case 0xC8:
            INY();
            break;
        case 0x4C:
            JMP(fetch16());
            break;
        case 0x6C:
            JMP_indirect(fetch16());
            break;
        case 0x20:
            JSR(fetch16());
            break;
        case 0xA9:
            LDA(fetch());
            break;
        case 0xA5:
            LDA(read(fetch()));
            break;
        case 0xB5:
            LDA(read(fetch() + X));
            break;
        case 0xAD:
            LDA(read(fetch16()));
            break;
        case 0xBD:
            LDA(read(fetch16() + X));
            break;
        case 0xB9:
            LDA(read(fetch16() + Y));
            break;
        case 0xA1:
            LDA(read((fetch() + X) & 0xFF));
            break;
        case 0xB1:
            LDA(read((fetch() & 0xFF) + Y));
            break;
        case 0xA2:
            LDX(fetch());
            break;
        case 0xA6:
            LDX(read(fetch()));
            break;
        case 0xB6:
            LDX(read(fetch() + Y));
            break;
        case 0xAE:
            LDX(fetch16());
            break;
        case 0xBE:
            LDX(fetch16() + Y));
            break;
        case 0xA0:
            LDY(fetch());
            break;
        case 0xA4:
            LDY(read(fetch()));
            break;
        case 0xB4:
            LDY(read(fetch() + X));
            break;
        case 0xAC:
            LDY(fetch16());
            break;
        case 0xBC:
            LDY(fetch16() + X));
            break;
        case 0x4A:
            LSR_accumulator();
            break;
        case 0x46:
            LSR(fetch());
            break;
        case 0x56:
            LSR(fetch() + X);
            break;
        case 0x4E:
            LSR(fetch16());
            break;
        case 0x5E:
            LSR(fetch16() + X));
            break;
        case 0x09:
            ORA(fetch());
            break;
        case 0x05:
            ORA(read(fetch()));
            break;
        case 0x15:
            ORA(read(fetch() + X));
            break;
        case 0x0D:
            ORA(read(fetch16()));
            break;
        case 0x1D:
            ORA(read(fetch16() + X));
            break;
        case 0x19:
            ORA(read(fetch16() + Y));
            break;
        case 0x01:
            ORA(read((fetch() + X) & 0xFF));
            break;
        case 0x11:
            ORA(read((fetch() & 0xFF) + Y));
            break;
        case 0x48:
            PHA();
            break;
        case 0x08:
            PHP();
            break;
        case 0x68:
            PLA();
            break;
        case 0x28:
            PLP();
            break;
        case 0x2A:
            ROL_accumulator();
            break;
        case 0x26:
            ROL(fetch());
            break;
        case 0x36:
            ROL(fetch() + X);
            break;
        case 0x2E:
            ROL(fetch16());
            break;
        case 0x3E:
            ROL(fetch16() + X));
            break;
        case 0x6A:
            ROR_accumulator();
            break;
        case 0x66:
            ROR(fetch());
            break;
        case 0x76:
            ROR(fetch() + X);
            break;
        case 0x6E:
            ROR(fetch16());
            break;
        case 0x7E:
            ROR(fetch16() + X));
            break;
        case 0x40:
            RTI();
            break;
        case 0x60:
            RTS();
            break;
        case 0xE9:
            SBC(fetch());
            break;
        case 0xE5: SBC(read(fetch()));
            break;
        case 0xF5:
            SBC(read(fetch() + X));
            break;
        case 0xED:
            SBC(read(fetch16()));
            break;
        case 0xFD:
            SBC(read(fetch16() + X));
            break;
        case 0xF9:
            SBC(read(fetch16() + Y));
            break;
        case 0xE1:
            SBC(read((fetch() + X) & 0xFF));
            break;
        case 0xF1:
            SBC(read((fetch() & 0xFF) + Y));
            break;
        case 0x38:
            SEC();
            break;
        case 0xF8:
            SED();
            break;
        case 0x78:
            SEI();
            break;
        case 0x85:
            STA(fetch()); break;
        case 0x95:
            STA(fetch() + X);
            break;
        case 0x8D:
            STA(fetch16());
            break;
        case 0x9D:
            STA(fetch16() + X);
            break;
        case 0x99:
            STA(fetch16() + Y);
            break;
        case 0x81:
            STA(read((fetch() + X) & 0xFF));
            break;
        case 0x91:
            STA(read((fetch() & 0xFF) + Y));
            break;
        case 0x86:
            STX(fetch());
            break;
        case 0x96:
            STX(fetch() + Y);
            break;
        case 0x8E:
            STX(fetch16());
            break;
        case 0x84:
            STY(fetch());
            break;
        case 0x94:
            STY(fetch() + X);
            break;
        case 0x8C:
            STY(fetch16());
            break;
        case 0xAA:
            TAX();
            break;
        case 0xA8:
            TAY();
            break;
        case 0xBA:
            TSX();
            break;
        case 0x8A:
            TXA();
            break;
        case 0x9A:
            TXS();
            break;
        case 0x98:
            TYA();
            break;

        }
    }
}

// Реализация инструкций процессора
void CPU::ADC(uint8_t operand) {
    uint16_t temp = A + operand + (Getflag(FLAG_C) ? 1 : 0);
    Setflag(FLAG_C, temp > 0xFF);
    Setflag(FLAG_Z, (temp & 0xFF) == 0);
    Setflag(FLAG_V, (~(A ^ operand) & (A ^ temp) & 0x80) != 0);
    Setflag(FLAG_N, (temp & 0x80) != 0);
    A = temp & 0xFF;
}

void CPU::AND(uint8_t operand) {
    A &= operand;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::ASL_accumulator() {
    Setflag(FLAG_C, (A & 0x80) != 0);
    A <<= 1;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::ASL(uint16_t address) {
    uint8_t value = read(address);
    Setflag(FLAG_C, (value & 0x80) != 0);
    value <<= 1;
    write(address, value);
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
}

void CPU::BCC(uint8_t offset) {
    if (!Getflag(FLAG_C))
        PC += static_cast<int8_t>(offset);
}

void CPU::BCS(uint8_t offset) {
    if (Getflag(FLAG_C))
        PC += static_cast<int8_t>(offset);
}

void CPU::BEQ(uint8_t offset) {
    if (Getflag(FLAG_Z))
        PC += static_cast<int8_t>(offset);
}

void CPU::BIT(uint16_t address) {
    uint8_t value = read(address);
    Setflag(FLAG_Z, (A & value) == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
    Setflag(FLAG_V, (value & 0x40) != 0);
}

void CPU::BMI(uint8_t offset) {
    if (Getflag(FLAG_N))
        PC += static_cast<int8_t>(offset);
}

void CPU::BNE(uint8_t offset) {
    if (!Getflag(FLAG_Z))
        PC += static_cast<int8_t>(offset);
}

void CPU::BPL(uint8_t offset) {
    if (!Getflag(FLAG_N))
        PC += static_cast<int8_t>(offset);
}

void CPU::BRK() {
    Setflag(FLAG_I, true);
    stack_push16(PC + 1);
    stack_push(P | 0x30);
    PC = read16(0xFFFE);
}

void CPU::BVC(uint8_t offset) {
    if (!Getflag(FLAG_V))
        PC += static_cast<int8_t>(offset);
}

void CPU::BVS(uint8_t offset) {
    if (Getflag(FLAG_V))
        PC += static_cast<int8_t>(offset);
}

void CPU::CLC() {
    Setflag(FLAG_C, false);
}

void CPU::CLD() {
    Setflag(FLAG_D, false);
}

void CPU::CLI() {
    Setflag(FLAG_I, false);
}

void CPU::CLV() {
    Setflag(FLAG_V, false);
}

void CPU::CMP(uint8_t operand) {
    uint16_t temp = A - operand;
    Setflag(FLAG_C, A >= operand);
    Setflag(FLAG_Z, (temp & 0xFF) == 0);
    Setflag(FLAG_N, (temp & 0x80) != 0);
}

void CPU::CPX(uint8_t operand) {
    uint16_t temp = X - operand;
    Setflag(FLAG_C, X >= operand);
    Setflag(FLAG_Z, (temp & 0xFF) == 0);
    Setflag(FLAG_N, (temp & 0x80) != 0);
}

void CPU::CPY(uint8_t operand) {
    uint16_t temp = Y - operand;
    Setflag(FLAG_C, Y >= operand);
    Setflag(FLAG_Z, (temp & 0xFF) == 0);
    Setflag(FLAG_N, (temp & 0x80) != 0);
}

void CPU::DEC(uint16_t address) {
    uint8_t value = read(address) - 1;
    write(address, value);
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
}

void CPU::DEX() {
    X--;
    Setflag(FLAG_Z, X == 0);
    Setflag(FLAG_N, (X & 0x80) != 0);
}

void CPU::DEY() {
    Y--;
    Setflag(FLAG_Z, Y == 0);
    Setflag(FLAG_N, (Y & 0x80) != 0);
}

void CPU::EOR(uint8_t operand) {
    A ^= operand;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::INC(uint16_t address) {
    uint8_t value = read(address) + 1;
    write(address, value);
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
}

void CPU::INX() {
    X++;
    Setflag(FLAG_Z, X == 0);
    Setflag(FLAG_N, (X & 0x80) != 0);
}

void CPU::INY() {
    Y++;
    Setflag(FLAG_Z, Y == 0);
    Setflag(FLAG_N, (Y & 0x80) != 0);
}

void CPU::JMP(uint16_t address) {
    PC = address;
}

void CPU::JMP_indirect(uint16_t address) {
    PC = read16(address);
}

void CPU::JSR(uint16_t address) {
    stack_push16(PC - 1);
    PC = address;
}

void CPU::LDA(uint8_t operand) {
    A = operand;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::LDX(uint8_t operand) {
    X = operand;
    Setflag(FLAG_Z, X == 0);
    Setflag(FLAG_N, (X & 0x80) != 0);
}

void CPU::LDY(uint8_t operand) {
    Y = operand;
    Setflag(FLAG_Z, Y == 0);
    Setflag(FLAG_N, (Y & 0x80) != 0);
}

void CPU::LSR_accumulator() {
    Setflag(FLAG_C, (A & 0x01) != 0);
    A >>= 1;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::LSR(uint16_t address) {
    uint8_t value = read(address);
    Setflag(FLAG_C, (value & 0x01) != 0);
    value >>= 1;
    write(address, value);
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
}

void CPU::ORA(uint8_t operand) {
    A |= operand;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::PHA() {
    stack_push(A);
}

void CPU::PHP() {
    stack_push(P | 0x30);
}

void CPU::PLA() {
    A = stack_pop();
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::PLP() {
    P = stack_pop() & ~0x30;
}

void CPU::ROL_accumulator() {
    uint8_t carry = Getflag(FLAG_C) ? 1 : 0;
    Setflag(FLAG_C, (A & 0x80) != 0);
    A = (A << 1) | carry;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::ROL(uint16_t address) {
    uint8_t value = read(address);
    uint8_t carry = Getflag(FLAG_C) ? 1 : 0;
    Setflag(FLAG_C, (value & 0x80) != 0);
    value = (value << 1) | carry;
    write(address, value);
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
}

void CPU::ROR_accumulator() {
    uint8_t carry = Getflag(FLAG_C) ? 0x80 : 0;
    Setflag(FLAG_C, (A & 0x01) != 0);
    A = (A >> 1) | carry;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::ROR(uint16_t address) {
    uint8_t value = read(address);
    uint8_t carry = Getflag(FLAG_C) ? 0x80 : 0;
    Setflag(FLAG_C, (value & 0x01) != 0);
    value = (value >> 1) | carry;
    write(address, value);
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, (value & 0x80) != 0);
}

void CPU::RTI() {
    P = stack_pop();
    PC = stack_pop16();
}

void CPU::RTS() {
    PC = stack_pop16() + 1;
}

void CPU::SBC(uint8_t operand) {
    uint16_t temp = A - operand - (Getflag(FLAG_C) ? 0 : 1);
    Setflag(FLAG_C, temp < 0x100);
    Setflag(FLAG_Z, (temp & 0xFF) == 0);
    Setflag(FLAG_V, ((A ^ operand) & (A ^ temp) & 0x80) != 0);
    Setflag(FLAG_N, (temp & 0x80) != 0);
    A = temp & 0xFF;
}

void CPU::SEC() {
    Setflag(FLAG_C, true);
}

void CPU::SED() {
    Setflag(FLAG_D, true);
}

void CPU::SEI() {
    Setflag(FLAG_I, true);
}

void CPU::STA(uint16_t address) {
    write(address, A);
}

void CPU::STX(uint16_t address) {
    write(address, X);
}

void CPU::STY(uint16_t address) {
    write(address, Y);
}

void CPU::TAX() {
    X = A;
    Setflag(FLAG_Z, X == 0);
    Setflag(FLAG_N, (X & 0x80) != 0);
}

void CPU::TAY() {
    Y = A;
    Setflag(FLAG_Z, Y == 0);
    Setflag(FLAG_N, (Y & 0x80) != 0);
}

void CPU::TSX() {
    X = S;
    Setflag(FLAG_Z, X == 0);
    Setflag(FLAG_N, (X & 0x80) != 0);
}

void CPU::TXA() {
    A = X;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::TXS() {
    S = X;
}

void CPU::TYA() {
    A = Y;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}