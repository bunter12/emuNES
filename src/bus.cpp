#include <bus.h>

void Bus::clock() {
    ppu.clock();
    set_cartridge_irq_line(cart && cart->irq_asserted());

    if ((system_clock_counter % 3) == 0) {
        if (dma_transfer) {
            if (dma_dummy) {
                if ((system_clock_counter % 2) == 1) {
                    dma_dummy = false;
                }
            } else {
                if ((system_clock_counter % 2) == 0) {
                    dma_data = cpu_read(((uint16_t)dma_page << 8) | dma_addr);
                } else {
                    ppu.cpu_write(0x0004, dma_data);
                    dma_addr++;
                    if (dma_addr == 0x00) {
                        dma_transfer = false;
                        dma_dummy = true;
                    }
                }
            }
        } else {
            cpu.clock();
        }

        apu.clock();
    }

    system_clock_counter++;
}

void Bus::cpu_write(uint16_t address, uint8_t data) {
    if (cart && cart->cpu_write(address, data)) {
    }
    else if (address >= 0x0000 && address <= 0x1FFF) {
        cpu_ram[address & 0x07FF] = data;
    } else if (address >= 0x2000 && address <= 0x3FFF) {
        ppu.cpu_write(address & 0x0007, data);
    }
    else if (address == 0x4014) {
        dma_page = data;
        dma_addr = 0x00;
        dma_transfer = true;
        dma_dummy = true;
    }
    else if (address == 0x4016) {
        controller[0].write(data);
        controller[1].write(data);
    }
    else if (address >= 0x4000 && address <= 0x4017) {
        apu.cpu_write(address, data);
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
    else if (address == 0x4015) {
        data = apu.cpu_read(address);
    }
    else if (address == 0x4017) {
        data = controller[1].read();
    }
    return data;
}

Bus::Bus() {
    cpu.connect_bus(this);
    ppu.connect_bus(this);
    apu.connect_bus(this);
}

void Bus::insert_cartridge(Cartridge* cartridge) {
    this->cart = cartridge;
    set_cartridge_irq_line(false);
}

void Bus::nmi(bool defer_one_instruction) {
    cpu.nmi(defer_one_instruction);
}

void Bus::set_irq_line(bool asserted) {
    apu_irq_line = asserted;
    cpu.set_irq_line(apu_irq_line || cartridge_irq_line);
}

void Bus::set_cartridge_irq_line(bool asserted) {
    cartridge_irq_line = asserted;
    cpu.set_irq_line(apu_irq_line || cartridge_irq_line);
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
