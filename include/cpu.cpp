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

bool CPU::Getflag(uint8_t flag) {
    return (status & flag) != 0;
}

void CPU::Setflag(uint8_t flag, bool value) {
    if (value)
        status |= flag
    else
        status &= ~flag
}

void CPU::log_status() {
    std::cout << "PC: " << std::hex << PC << " A: " << std::hex << +A
        << " X: " << std::hex << +X << " Y: " << std::hex << +Y
        << " SP: " << std::hex << +SP << " Status: " << std::hex << +status << std::endl;
}

void CPU::reset() {
    PC = (read(0xFFFD) << 8) | read(0xFFFC); // Считывание адреса начального выполнения
    SP = 0xFD;
    status = 0x00;
}

void CPU::execute() {
    uint8_t opcode = fetch();
    switch (opcode) {
    case 0x69:
        ADC(fetch());
        break;
    case 0x29:
        AND(fetch());
        break;
    case 0x0A:
        ASL_accumulator();
        break;
    case 0x06:
        ASL(fetch());
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
    case 0x30:
        BMI(fetch());
        break;
    case 0xD0:
        BNE(fetch());
        break;
    case 0x10:
        BPL(fetch());
        break;
    }
}

void CPU::ADC(uint8_t operand) {
    uint8_t temp = A + Getflag(FLAG_C) + operand;
    Setflag(FLAG_C, temp > 0xff);
    Setflag(FLAG_Z, (temp & 0xff) == 0);
    Setflag(FLAG_V, (~(A ^ operand) & (A ^ temp) & 0x80) != 0);
    Setflag(FLAG_N, temp & 0x80);
    A = temp & 0xFF;
}

void CPU::AND(uint8_t operand) {
    A = A & operand;
    Setflag(FLAG_Z, A==0);
    Setflag(FLAG_N, A & 0x80);
}

void CPU::ASL_accumulator() {
    Setflag(FLAG_C, A & 0x80);
    A <<= 1;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, A & 0x80);
}

void CPU::ASL(uint8_t address) {
    uint8_t value = read(address);
    Setflag(FLAG_C, value & 0x80);
    value <<= 1;
    Setflag(FLAG_Z, value == 0);
    Setflag(FLAG_N, value & 0x80);
    write(address, value);
}

void CPU::BCC(uint8_t offset) {
    if (!Getflag(FLAG_C))
        PC+=offset
}

void CPU::BCS(uint8_t offset) {
    if (Getflag(FLAG_C))
        PC+=offset
}

void CPU::BEQ(uint8_t offset) {
    if (Getflag(FLAG_Z))
        PC += offset;
}

void CPU::BIT(uint8_t address) {
    uint8_t value = read(address);
    uint8_t result = A & value;
    Setflag(FLAG_Z, result == 0);
    Setflag(FLAG_V, value & 0x40);
    Setflag(FLAG_N, value & 0x80);
}

void CPU::BMI(uint8_t offset) {
    if (Getflag(FLAG_N))
        PC += offset;
}

void CPU::BNE(uint8_t offset) {
    if (!Getflag(FLAG_Z))
        PC += offset;
}

void CPU::BPL(uint8_t offset) {
    if (!Getflag(FLAG_N))
        PC += offset;
}

