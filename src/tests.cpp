#include <cassert>
#include <iostream>
#include "../include/cpp.h"
#include "../include/ppu.h"

class CPUTest {
private:
    CPU cpu;

    void SetUp() {
        cpu.reset();
    }

    void TestADC() {
        std::cout << "Testing ADC instruction..." << std::endl;
        
        cpu.A = 0x05;
        cpu.Setflag(FLAG_C, false);
        cpu.ADC(0x05);
        assert(cpu.A == 0x0A && "ADC: Simple addition failed");
        assert(!cpu.Getflag(FLAG_C) && "ADC: Carry flag incorrectly set");
        assert(!cpu.Getflag(FLAG_V) && "ADC: Overflow flag incorrectly set");
        assert(!cpu.Getflag(FLAG_N) && "ADC: Negative flag incorrectly set");
        assert(!cpu.Getflag(FLAG_Z) && "ADC: Zero flag incorrectly set");

        cpu.A = 0xFF;
        cpu.Setflag(FLAG_C, false);
        cpu.ADC(0x01);
        assert(cpu.A == 0x00 && "ADC: Addition with carry failed");
        assert(cpu.Getflag(FLAG_C) && "ADC: Carry flag not set");
        assert(!cpu.Getflag(FLAG_V) && "ADC: Overflow flag incorrectly set");
        assert(!cpu.Getflag(FLAG_N) && "ADC: Negative flag incorrectly set");
        assert(cpu.Getflag(FLAG_Z) && "ADC: Zero flag not set");

        cpu.A = 0x7F;
        cpu.Setflag(FLAG_C, false);
        cpu.ADC(0x01);
        assert(cpu.A == 0x80 && "ADC: Overflow addition failed");
        assert(!cpu.Getflag(FLAG_C) && "ADC: Carry flag incorrectly set");
        assert(cpu.Getflag(FLAG_V) && "ADC: Overflow flag not set");
        assert(cpu.Getflag(FLAG_N) && "ADC: Negative flag not set");
        assert(!cpu.Getflag(FLAG_Z) && "ADC: Zero flag incorrectly set");
    }

    void TestAND() {
        std::cout << "Testing AND instruction..." << std::endl;
        
        cpu.A = 0xFF;
        cpu.AND(0x0F);
        assert(cpu.A == 0x0F && "AND: Basic operation failed");
        assert(!cpu.Getflag(FLAG_N) && "AND: Negative flag incorrectly set");
        assert(!cpu.Getflag(FLAG_Z) && "AND: Zero flag incorrectly set");

        cpu.A = 0xFF;
        cpu.AND(0x00);
        assert(cpu.A == 0x00 && "AND: Zero result failed");
        assert(!cpu.Getflag(FLAG_N) && "AND: Negative flag incorrectly set");
        assert(cpu.Getflag(FLAG_Z) && "AND: Zero flag not set");

        cpu.A = 0xFF;
        cpu.AND(0x80);
        assert(cpu.A == 0x80 && "AND: Negative result failed");
        assert(cpu.Getflag(FLAG_N) && "AND: Negative flag not set");
        assert(!cpu.Getflag(FLAG_Z) && "AND: Zero flag incorrectly set");
    }

    void TestLDA() {
        std::cout << "Testing LDA instruction..." << std::endl;
        
        cpu.LDA(0x42);
        assert(cpu.A == 0x42 && "LDA: Load positive number failed");
        assert(!cpu.Getflag(FLAG_N) && "LDA: Negative flag incorrectly set");
        assert(!cpu.Getflag(FLAG_Z) && "LDA: Zero flag incorrectly set");

        cpu.LDA(0x00);
        assert(cpu.A == 0x00 && "LDA: Load zero failed");
        assert(!cpu.Getflag(FLAG_N) && "LDA: Negative flag incorrectly set");
        assert(cpu.Getflag(FLAG_Z) && "LDA: Zero flag not set");

        cpu.LDA(0x80);
        assert(cpu.A == 0x80 && "LDA: Load negative number failed");
        assert(cpu.Getflag(FLAG_N) && "LDA: Negative flag not set");
        assert(!cpu.Getflag(FLAG_Z) && "LDA: Zero flag incorrectly set");
    }

    void TestBranches() {
        std::cout << "Testing branch instructions..." << std::endl;
        
        cpu.PC = 0x0100;
        cpu.Setflag(FLAG_Z, true);
        cpu.BEQ(0x10);
        assert(cpu.PC == 0x0110 && "BEQ: Forward branch failed");

        cpu.PC = 0x0100;
        cpu.Setflag(FLAG_Z, false);
        cpu.BEQ(0x10);
        assert(cpu.PC == 0x0100 && "BEQ: Branch taken when it shouldn't");

        cpu.PC = 0x0100;
        cpu.Setflag(FLAG_Z, false);
        cpu.BNE(0x10);
        assert(cpu.PC == 0x0110 && "BNE: Forward branch failed");

        cpu.PC = 0x0100;
        cpu.Setflag(FLAG_C, true);
        cpu.BCS(0xF0);
        assert(cpu.PC == 0x00F0 && "BCS: Backward branch failed");
    }

    void TestStack() {
        std::cout << "Testing stack operations..." << std::endl;
        
        cpu.SP = 0xFF;
        uint8_t original_sp = cpu.SP;
        cpu.A = 0x42;
        cpu.PHA();
        assert(cpu.SP == original_sp - 1 && "PHA: Stack pointer not decremented");
        
        cpu.A = 0x00;
        cpu.PLA();
        assert(cpu.A == 0x42 && "PLA: Retrieved value incorrect");
        assert(cpu.SP == original_sp && "PLA: Stack pointer not restored");

        cpu.SP = 0xFF;
        original_sp = cpu.SP;
        cpu.status = 0xAA;
        std::cout << "Before PHP: status = 0x" << std::hex << (int)cpu.status << std::endl;
        cpu.PHP();
        assert(cpu.SP == original_sp - 1 && "PHP: Stack pointer not decremented");
        
        cpu.status = 0x00;
        std::cout << "Before PLP: status = 0x" << std::hex << (int)cpu.status << std::endl;
        cpu.PLP();
        std::cout << "After PLP: status = 0x" << std::hex << (int)cpu.status << std::endl;
        assert(cpu.status == 0xAA && "PLP: Retrieved status incorrect");
        assert(cpu.SP == original_sp && "PLP: Stack pointer not restored");
    }

    void TestASL() {
        std::cout << "Testing ASL instruction..." << std::endl;
        
        // Тест ASL аккумулятораx
        cpu.A = 0x01;
        cpu.ASL_accumulator();
        assert(cpu.A == 0x02 && "ASL: Simple shift left failed");
        assert(!cpu.Getflag(FLAG_C) && "ASL: Carry flag incorrectly set");
        
        cpu.A = 0x80;
        cpu.ASL_accumulator();
        assert(cpu.A == 0x00 && "ASL: Shift with carry failed");
        assert(cpu.Getflag(FLAG_C) && "ASL: Carry flag not set");
        assert(cpu.Getflag(FLAG_Z) && "ASL: Zero flag not set");
    }

    void TestLSR() {
        std::cout << "Testing LSR instruction..." << std::endl;
        
        cpu.A = 0x02;
        cpu.LSR_accumulator();
        assert(cpu.A == 0x01 && "LSR: Simple shift right failed");
        assert(!cpu.Getflag(FLAG_C) && "LSR: Carry flag incorrectly set");
        
        cpu.A = 0x01;
        cpu.LSR_accumulator();
        assert(cpu.A == 0x00 && "LSR: Shift with carry failed");
        assert(cpu.Getflag(FLAG_C) && "LSR: Carry flag not set");
        assert(cpu.Getflag(FLAG_Z) && "LSR: Zero flag not set");
    }

    void TestROL() {
        std::cout << "Testing ROL instruction..." << std::endl;
        
        cpu.A = 0x01;
        cpu.Setflag(FLAG_C, false);
        cpu.ROL_accumulator();
        assert(cpu.A == 0x02 && "ROL: Simple rotate left failed");
        assert(!cpu.Getflag(FLAG_C) && "ROL: Carry flag incorrectly set");
        
        cpu.A = 0x80;
        cpu.Setflag(FLAG_C, true);
        cpu.ROL_accumulator();
        assert(cpu.A == 0x01 && "ROL: Rotate with carry failed");
        assert(cpu.Getflag(FLAG_C) && "ROL: Carry flag not set");
    }

    void TestROR() {
        std::cout << "Testing ROR instruction..." << std::endl;
        
        cpu.A = 0x02;
        cpu.Setflag(FLAG_C, false);
        cpu.ROR_accumulator();
        assert(cpu.A == 0x01 && "ROR: Simple rotate right failed");
        assert(!cpu.Getflag(FLAG_C) && "ROR: Carry flag incorrectly set");
        
        cpu.A = 0x01;
        cpu.Setflag(FLAG_C, true);
        cpu.ROR_accumulator();
        assert(cpu.A == 0x80 && "ROR: Rotate with carry failed");
        assert(cpu.Getflag(FLAG_C) && "ROR: Carry flag not set");
    }

public:
    void RunAllTests() {
        SetUp();
        TestADC();
        TestAND();
        TestLDA();
        TestBranches();
        TestStack();
        TestASL();
        TestLSR();
        TestROL();
        TestROR();
        std::cout << "All CPU tests passed successfully!" << std::endl;
    }
};

class PPUTest {
private:
    PPU ppu;

    void SetUp() {
        ppu.reset();
    }

    void TestReset() {
        std::cout << "Testing PPU reset..." << std::endl;
        ppu.reset();
        assert(ppu.get_frame_complete() == false && "PPU reset: Frame completed flag not reset");
        assert(ppu.get_screen_buffer()[0] == 0 && "PPU reset: Screen buffer not cleared");
    }

    void TestStep() {
        std::cout << "Testing PPU step..." << std::endl;
        ppu.step();
        assert(ppu.get_frame_complete() == false && "PPU step: Frame incorrectly marked as completed");
    }

    void TestRenderPixel() {
        std::cout << "Testing PPU render_pixel..." << std::endl;
        ppu.render_pixel();
    }

public:
    void RunTests() {
        SetUp();
        TestReset();
        TestStep();
        TestRenderPixel();
        std::cout << "All PPU tests passed successfully!" << std::endl;
    }
};

int main() {

    PPUTest ppuTest;
    ppuTest.RunTests();

    return 0;
}
