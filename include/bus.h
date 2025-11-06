#ifndef BUS_H
#define BUS_H
#include <cpu.h>
#include <ppu.h>

class Bus{
public:
    Bus();
    ~Bus();

    void cpu_write(uint16_t address, uint8_t data);
    uint8_t cpu_read(uint16_t address);
    
private:
    CPU cpu;
//    PPU ppu;
    std::array<uint8_t, 2048> cpu_ram;
};

#endif //BUS_H
