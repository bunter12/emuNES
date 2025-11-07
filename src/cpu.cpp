#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <cpu.h>
#include <bus.h>

static const uint8_t cycles[256] = {
    // 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
       7, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 0, 4, 6, 0, // 0x
       2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 1x
       6, 6, 0, 0, 3, 3, 5, 0, 4, 2, 2, 0, 4, 4, 6, 0, // 2x
       2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 3x
       6, 6, 0, 0, 0, 3, 5, 0, 3, 2, 2, 0, 3, 4, 6, 0, // 4x
       2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 5x
       6, 6, 0, 0, 0, 3, 5, 0, 4, 2, 2, 0, 5, 4, 6, 0, // 6x
       2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // 7x
       0, 6, 0, 0, 3, 3, 3, 0, 2, 0, 2, 0, 4, 4, 4, 0, // 8x
       2, 6, 0, 0, 4, 4, 4, 0, 2, 5, 2, 0, 0, 5, 0, 0, // 9x
       2, 6, 2, 0, 3, 3, 3, 0, 2, 2, 2, 0, 4, 4, 4, 0, // Ax
       2, 5, 0, 0, 4, 4, 4, 0, 2, 4, 2, 0, 4, 4, 4, 0, // Bx
       2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, // Cx
       2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0, // Dx
       2, 6, 0, 0, 3, 3, 5, 0, 2, 2, 2, 0, 4, 4, 6, 0, // Ex
       2, 5, 0, 0, 0, 4, 6, 0, 2, 4, 0, 0, 0, 4, 7, 0  // Fx
};

uint16_t CPU::read16_zeropage(uint16_t address) {
    uint8_t addr_low = address & 0x00FF;
    uint8_t low_byte = read(addr_low);
    uint8_t addr_high = (addr_low + 1) & 0x00FF;
    uint8_t high_byte = read(addr_high);

    return (high_byte << 8) | low_byte;
}

uint16_t CPU::addr_abs_x(int& cycles) {
    uint16_t base_addr = fetch16();
    uint16_t final_addr = base_addr + X;

    if ((base_addr & 0xFF00) != (final_addr & 0xFF00)) {
        cycles++;
    }
    return final_addr;
}

uint16_t CPU::addr_abs_y(int& cycles) {
    uint16_t base_addr = fetch16();
    uint16_t final_addr = base_addr + Y;

    if ((base_addr & 0xFF00) != (final_addr & 0xFF00)) {
        cycles++;
    }
    return final_addr;
}

uint16_t CPU::addr_ind_y(int& cycles) {
    uint8_t zp_addr = fetch();
    uint16_t base_addr = read16_zeropage(zp_addr);
    uint16_t final_addr = base_addr + Y;

    if ((base_addr & 0xFF00) != (final_addr & 0xFF00)) {
        cycles++;
    }
    return final_addr;
}

uint8_t CPU::read(uint16_t address) {
    return bus->cpu_read(address);
}

void CPU::write(uint16_t address, uint8_t value) {
    bus->cpu_write(address, value);
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
        << " SP: " << std::hex << +SP << " Status: " << std::hex << +status <<std::endl;
}

void CPU::reset() {
//    PC = (read(0xFFFD) << 8) | read(0xFFFC);
    PC=0xc000;
    
    SP = 0xFD;
    A = X = Y = 0;
    running = true;
    status = 0x00;
}

void CPU::turn_off() {
    running = false;
}

void CPU::turn_on() {
    running = true;
}

void CPU::nmi() {
    stack_push16(PC);
    stack_push(status);
    
    Setflag(FLAG_I, true);
    
    PC = read16(0xFFFA);
}

int CPU::clock() {
    uint8_t opcode = fetch();
    int instruction_cycles = cycles[opcode];
    int8_t offset=0;
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
        ADC(read(addr_abs_x(instruction_cycles)));
        break;
    case 0x79:
        ADC(read(addr_abs_y(instruction_cycles)));
        break;
    case 0x61:
        ADC(read((fetch() + X) & 0xFF));
        break;
    case 0x71:
        ADC(read(addr_ind_y(instruction_cycles)));
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
        AND(read(addr_abs_x(instruction_cycles)));
        break;
    case 0x39:
        AND(read(addr_abs_y(instruction_cycles)));
        break;
    case 0x21:
        AND(read((fetch() + X) & 0xFF));
        break;
    case 0x31:
        AND(read(addr_ind_y(instruction_cycles)));
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
        offset = fetch();
        if (!Getflag(FLAG_C)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0xB0:
        offset = fetch();
        if (Getflag(FLAG_C)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0xF0:
        offset = fetch();
        if (Getflag(FLAG_Z)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0x24:
        BIT(fetch());
        break;
    case 0x2C:
        BIT(fetch16());
        break;
    case 0x30:
        offset = fetch();
        if (Getflag(FLAG_N)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0xD0:
        offset = fetch();
        if (!Getflag(FLAG_Z)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0x10:
        offset = fetch();
        if (!Getflag(FLAG_N)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0x00:
        BRK();
        break;
    case 0x50:
        offset = fetch();
        if (!Getflag(FLAG_V)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
        break;
    case 0x70:
        offset = fetch();
        if (Getflag(FLAG_V)) {
            instruction_cycles++;
            uint16_t target_addr = PC + offset;
            if ((PC & 0xFF00) != (target_addr & 0xFF00)) {
                instruction_cycles++;
            }
            PC = target_addr;
        }
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
        CMP(read(addr_abs_x(instruction_cycles)));
        break;
    case 0xD9:
        CMP(read(addr_abs_y(instruction_cycles)));
        break;
    case 0xC1:
        CMP(read((fetch() + X) & 0xFF));
        break;
    case 0xD1:
        CMP(read(addr_ind_y(instruction_cycles)));
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
        EOR(read(addr_abs_x(instruction_cycles)));
        break;
    case 0x59:
        EOR(read(addr_abs_y(instruction_cycles)));
        break;
    case 0x41:
        EOR(read((fetch() + X) & 0xFF));
        break;
    case 0x51:
        EOR(read(addr_ind_y(instruction_cycles)));
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
        LDA(read(addr_abs_x(instruction_cycles)));
        break;
    case 0xB9:
        LDA(read(addr_abs_y(instruction_cycles)));
        break;
    case 0xA1:
        LDA(read((fetch() + X) & 0xFF));
        break;
    case 0xB1:
        LDA(read(addr_ind_y(instruction_cycles)));
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
        LDX(read(addr_abs_y(instruction_cycles)));
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
        LDY(read(addr_abs_x(instruction_cycles)));
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
        LSR(fetch16() + X);
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
        ORA(read(addr_abs_x(instruction_cycles)));
        break;
    case 0x19:
        ORA(read(addr_abs_y(instruction_cycles)));
        break;
    case 0x01:
        ORA(read((fetch() + X) & 0xFF));
        break;
    case 0x11:
        ORA(read(addr_ind_y(instruction_cycles)));
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
        ROL(fetch16() + X);
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
        ROR(fetch16() + X);
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
        SBC(read(addr_abs_x(instruction_cycles)));
        break;
    case 0xF9:
        SBC(read(addr_abs_y(instruction_cycles)));
        break;
    case 0xE1:
        SBC(read((fetch() + X) & 0xFF));
        break;
    case 0xF1:
        SBC(read(addr_ind_y(instruction_cycles)));
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
    
    return instruction_cycles;
}

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
    stack_push(status | 0x30);
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
    uint16_t low = read(address);
    uint16_t high_addr = (address & 0xFF00) | ((address + 1) & 0x00FF);
    uint16_t high = read(high_addr);
    PC = (high << 8) | low;
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
     stack_push(status);
}

void CPU::PLA() {
    A = stack_pop();
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::PLP() {
    status = stack_pop();
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
    status = stack_pop();
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
    X = SP;
    Setflag(FLAG_Z, X == 0);
    Setflag(FLAG_N, (X & 0x80) != 0);
}

void CPU::TXA() {
    A = X;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::TXS() {
    SP = X;
}

void CPU::TYA() {
    A = Y;
    Setflag(FLAG_Z, A == 0);
    Setflag(FLAG_N, (A & 0x80) != 0);
}

void CPU::stack_push(uint8_t value) {
    write(0x0100 + SP, value);
    SP--;
}

void CPU::stack_push16(uint16_t value) {
    stack_push((value >> 8) & 0xFF);  // Push high byte
    stack_push(value & 0xFF);         // Push low byte
}

uint8_t CPU::stack_pop() {
    uint8_t value = read(0x0100 + SP + 1);
    SP++;
    return value;
}

uint16_t CPU::stack_pop16() {
    uint8_t low = stack_pop();        // Pop low byte first
    uint8_t high = stack_pop();       // Pop high byte
    return (high << 8) | low;         // Combine into 16-bit value
}

uint16_t CPU::read16(uint16_t address) {
    uint8_t low = read(address);
    uint8_t high = read(address + 1);
    return (high << 8) | low;
}
