#pragma once
#include "cpu.h:
class Bus {
public:

    CPU cpu;
    PPU ppu;
    APU apu;
    Controller controller1, controller2;
    Mapper* mapper;

    // Оперативная память
    uint8_t ram[2048];

    // Чтение и запись
    uint8_t cpuRead(uint16_t addr);
    void cpuWrite(uint16_t addr, uint8_t data);

};