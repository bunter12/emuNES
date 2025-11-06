#ifndef BUS_H
#define BUS_H
#include <cpu.h>
#include <ppu.h>
#include <cartridge.h>

class Bus{
public:
    Bus();

    void cpu_write(uint16_t address, uint8_t data);
    uint8_t cpu_read(uint16_t address);
    void insert_cartridge(Cartridge* cartridge);
    CPU cpu;
    
private:
//    PPU ppu;
    Cartridge* cart = nullptr;
    std::array<uint8_t, 2048> cpu_ram;
};

#endif //BUS_H
