#ifndef BUS_H
#define BUS_H
#include <cpu.h>
#include <apu.h>
#include <ppu.h>
#include <cartridge.h>
#include <controller.h>

class Bus{
public:
    Bus();
    void clock();

    void cpu_write(uint16_t address, uint8_t data);
    uint8_t cpu_read(uint16_t address);
    
    bool ppu_read(uint16_t address, uint8_t& data);
    bool ppu_write(uint16_t address, uint8_t data);
    
    void insert_cartridge(Cartridge* cartridge);
    CPU cpu;
    PPU ppu;
    APU apu;
    Cartridge* cart = nullptr;
    Controller controller[2];
    
    void nmi(bool defer_one_instruction = false);
    void set_irq_line(bool asserted);
    void set_cartridge_irq_line(bool asserted);
    
private:
    std::array<uint8_t, 2048> cpu_ram;
    uint64_t system_clock_counter = 0;

    bool dma_transfer = false;
    bool dma_dummy = true;
    uint8_t dma_page = 0x00;
    uint8_t dma_addr = 0x00;
    uint8_t dma_data = 0x00;

    bool apu_irq_line = false;
    bool cartridge_irq_line = false;
    
};

#endif //BUS_H
