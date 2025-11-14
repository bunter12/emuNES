#include <bus.h>

void Bus::cpu_write(uint16_t address, uint8_t data) {
    if (cart && cart->cpu_write(address, data)) {
    }
    else if (address >= 0x0000 && address <= 0x1FFF) {
        cpu_ram[address & 0x07FF] = data;
    } else if (address >= 0x2000 && address <= 0x3FFF) {
        ppu.cpu_write(address & 0x0007, data);
    }
    else if (address == 0x4014) {
        uint16_t start_addr = (uint16_t)data << 8;
        for (int i = 0; i < 256; i++) {
            ppu.oam[i] = cpu_read(start_addr + i);
        }
    }
    else if (address == 0x4016) {
        controller[0].write(data);
        controller[1].write(data);
    }
}

uint8_t Bus::cpu_read(uint16_t address) {
    uint8_t data = 0x00;
    if (cart && cart->cpu_read(address, data)) {
    }
    else if (address >= 0x0000 && address <= 0x1FFF) {
        data = cpu_ram[address & 0x07FF];
    } else if (address >= 0x2000 && address <= 0x3FFF) {
        data = ppu.cpu_read(address & 0x0007);
    }
    else if (address == 0x4016) {
        data = controller[0].read();
    }
    else if (address == 0x4017) {
        data = controller[1].read();
    }
    return data;
}

Bus::Bus() {
    cpu.connect_bus(this);
    ppu.connect_bus(this);
}

void Bus::insert_cartridge(Cartridge* cartridge) {
    this->cart = cartridge;
}

void Bus::nmi() {
    cpu.nmi();
}

bool Bus::ppu_read(uint16_t address, uint8_t& data) {
    if (cart) {
        return cart->ppu_read(address, data);
    }
    return false;
}

bool Bus::ppu_write(uint16_t address, uint8_t data) {
    if (cart) {
        return cart->ppu_write(address, data);
    }
    return false;
}
