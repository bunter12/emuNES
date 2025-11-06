#include <bus.h>

void Bus::cpu_write(uint16_t address, uint8_t data) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        cpu_ram[address & 0x07FF] = data;
    } else if (address >= 0x2000 && address <= 0x3FFF) {
//        ppu.cpu_write(address & 0x0007, data);
    }
}

uint8_t Bus::cpu_read(uint16_t address) {
    uint8_t data = 0x00;
    if (address >= 0x0000 && address <= 0x1FFF) {
        data = cpu_ram[address & 0x07FF];
    } else if (address >= 0x2000 && address <= 0x3FFF) {
//        data = ppu.cpu_read(address & 0x0007);
    }
    return data;
}

Bus::Bus() {
    cpu.connect_bus(this);
}
