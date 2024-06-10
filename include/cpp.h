#pragma once
#include <iostream>


class CPU {
public:
    uint8_t A;  // ��������
    uint8_t X;  // ������ �������� X
    uint8_t Y;  // ������ �������� Y
    uint8_t SP; // ��������� �����
    uint16_t PC; // ������� ��������
    uint8_t status; // ������� ���������

    uint8_t memory[65536]; // 64KB ������
    // ������ ����������
    void reset();    // ����� ����������
    void execute();  // ���������� ����������
    uint8_t fetch(); // ��������� ���������� ����� ����������
    uint8_t read(uint16_t address); // ��������� ���������� � ������ �� ������� 
    void write(uint16_t address, uint8_t value); // ������ ���������� � ������ ������
    void Setflag(uint8_t flag, bool value); // ������������� ��� ����
    bool Getflag(uint8_t flag); // ������ ����� �� ������������ ����

    // ����������� ��� ��������
    void log_status();
    //����� ���������
    const uint8_t FLAG_N = 0x80;  // �������������
    const uint8_t FLAG_V = 0x40;  // ������������
    const uint8_t FLAG_Ig = 0x20; // ������
    const uint8_t FLAG_B = 0x10;  // ����������
    const uint8_t FLAG_D = 0x08;  // ����������
    const uint8_t FLAG_I = 0x04;  // ������ ����������
    const uint8_t FLAG_Z = 0x02;  // �������
    const uint8_t FLAG_C = 0x01;  // ��������


private:
    // ���������� ����������
    void ADC(uint8_t operand);
    void AND(uint8_t operand);
    void ASL_accumulator();
    void ASL(uint8_t address);
    void BCC(uint8_t offset);
    void BCS(uint8_t offset);
    void BEQ(uint8_t offset);
    void BIT(uint8_t address);
    void BMI(uint8_t offset);
    void BNE(uint8_t offset);
    void BPL(uint8_t offset);
};