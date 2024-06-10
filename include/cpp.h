#pragma once
#include <iostream>


class CPU {
public:
    uint8_t A;  // сумматор
    uint8_t X;  // индекс регистра X
    uint8_t Y;  // индекс регистра Y
    uint8_t SP; // указатель стека
    uint16_t PC; // счетчик программ
    uint8_t status; // регистр состояния

    uint8_t memory[65536]; // 64KB памяти
    // методы процессора
    void reset();    // сброс процессора
    void execute();  // выполнение инструкций
    uint8_t fetch(); // Получение следующего байта инструкции
    uint8_t read(uint16_t address); // получение информации в памяти по адрессу 
    void write(uint16_t address, uint8_t value); // запись информации в ячейку памяти
    void Setflag(uint8_t flag, bool value); // устанавливаем наш флаг
    bool Getflag(uint8_t flag); // узнаем стоит ли определенный флаг

    // логирование для откладки
    void log_status();
    //флаги состояния
    const uint8_t FLAG_N = 0x80;  // отрицательный
    const uint8_t FLAG_V = 0x40;  // переполнение
    const uint8_t FLAG_Ig = 0x20; // игнора
    const uint8_t FLAG_B = 0x10;  // прерывание
    const uint8_t FLAG_D = 0x08;  // десятичное
    const uint8_t FLAG_I = 0x04;  // запрет прерывания
    const uint8_t FLAG_Z = 0x02;  // нулевой
    const uint8_t FLAG_C = 0x01;  // переноса


private:
    // реализация инструкций
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