#include <ppu.h>
#include <bus.h>
#include <cstdio>

static const uint32_t nes_palette[64] = {
    0x666666FF, 0x002A88FF, 0x1412A7FF, 0x3B00A4FF, 0x5C007EFF, 0x6E0040FF, 0x6C0600FF, 0x561D00FF,
    0x333500FF, 0x0B4800FF, 0x005200FF, 0x004F08FF, 0x00404DFF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xADADADFF, 0x155FD9FF, 0x4240FFFF, 0x7527FEFF, 0xA01ACCFF, 0xB71E7BFF, 0xB53120FF, 0x994E00FF,
    0x6B6D00FF, 0x388700FF, 0x0C9300FF, 0x008F32FF, 0x007C8DFF, 0x000000FF, 0x000000FF, 0x000000FF,
    0xFFFEFFFF, 0x64B0FFFF, 0x9290FFFF, 0xC276FFFF, 0xF368FFFF, 0xFF6ACCFF, 0xFF8170FF, 0xEAA536FF,
    0xC2C521FF, 0x89D800FF, 0x56E41AFF, 0x45E082FF, 0x48CDDEFF, 0x4F4F4FFF, 0x000000FF, 0x000000FF,
    0xFFFEFFFF, 0xC0DFFFFF, 0xD3D2FFFF, 0xE8CCFFFF, 0xFBC2FFFF, 0xFFC4EAFF, 0xFFCCA3FF, 0xF9DB90FF,
    0xE4E594FF, 0xCFEF96FF, 0xBDF4ABFF, 0xB3F3CCFF, 0xB5EBF2FF, 0xB8B8B8FF, 0x000000FF, 0x000000FF,
};

PPU::PPU() {}
PPU::~PPU() {}

uint32_t* PPU::get_screen() { return screen; }

uint8_t PPU::cpu_read(uint16_t address) {
    uint8_t data = 0x00;
    switch (address) {
        case 0x0002: // PPUSTATUS
            data = reg_status;
            reg_status &= ~0x80;
            address_latch_w = false;
            break;
        case 0x0004: // OAMDATA
            data = oam[oam_addr];
            break;
        case 0x0007: // PPUDATA
            data = ppu_data_buffer;
            ppu_data_buffer = ppu_read(vram_addr_v);
            
            if (vram_addr_v >= 0x3F00) data = ppu_data_buffer;

            vram_addr_v += (reg_ctrl & 0x04) ? 32 : 1;
            break;
    }
    return data;
}

void PPU::reset() {
    reg_ctrl = 0;
    reg_mask = 0;
    reg_status = 0;
    oam_addr = 0;
    
    address_latch_w = false;
    vram_addr_v = 0;
    vram_addr_t = 0;
    fine_x_scroll = 0;
    
    scanline = 0;
    cycle = 0;
    
}

void PPU::cpu_write(uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // PPUCTRL
            reg_ctrl = data;
            vram_addr_t = (vram_addr_t & 0xF3FF) | ((uint16_t)(data & 0x03) << 10);
            break;
        case 0x0001: // PPUMASK
            printf("[PPU WRITE] CPU wrote $%02X to PPUMASK\n", data);
            reg_mask = data;
            break;
        case 0x0003: // OAMADDR
            oam_addr = data;
            break;
        case 0x0004: // OAMDATA
            oam[oam_addr] = data;
            oam_addr++;
            break;
        case 0x0005: // PPUSCROLL
            if (address_latch_w == false) {
                fine_x_scroll = data & 0x07;
                vram_addr_t = (vram_addr_t & 0xFFE0) | (data >> 3);
                address_latch_w = true;
            } else {
                vram_addr_t = (vram_addr_t & 0x8C1F) | ((uint16_t)(data & 0x07) << 12) | ((uint16_t)(data & 0xF8) << 2);
                address_latch_w = false;
            }
            break;
        case 0x0006: // PPUADDR
            if (address_latch_w == false) {
                vram_addr_t = (vram_addr_t & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
                address_latch_w = true;
            } else {
                vram_addr_t = (vram_addr_t & 0xFF00) | data;
                vram_addr_v = vram_addr_t;
                address_latch_w = false;
            }
            break;
        case 0x0007: // PPUDATA
            ppu_write(vram_addr_v, data);
            vram_addr_v += (reg_ctrl & 0x04) ? 32 : 1;
            break;
    }
}

uint8_t PPU::ppu_read(uint16_t address, bool read_only) {
    uint8_t data = 0x00;
    address &= 0x3FFF;

    if (bus->ppu_read(address, data)) {
        return data;
    }
    else if (address >= 0x2000 && address <= 0x3EFF) {
        address &= 0x0FFF;
        
        auto mirror_mode = bus->cart->mirror;

        if (mirror_mode == Cartridge::FOUR_SCREEN) {
            if (address < 0x0800) {
                data = vram[address];
            } else {
                data = vram[address];
            }
            
        } else if (mirror_mode == Cartridge::VERTICAL) {
            if (address >= 0x0800) {
                data = vram[address - 0x0800];
            } else {
                data = vram[address];
            }
        } else if (mirror_mode == Cartridge::HORIZONTAL) {
            if (address >= 0x0400 && address < 0x0800) {
                data = vram[address - 0x0400];
            } else if (address >= 0x0C00) {
                data = vram[address - 0x0800];
            } else {
                data = vram[address];
            }
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) { // Palette RAM
        address &= 0x001F;
        
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        
        data = palette_ram[address];
    }
    
    
    return data;
}

void PPU::ppu_write(uint16_t address, uint8_t data) {
    address &= 0x3FFF;

    if (bus->ppu_write(address, data)) {
        return;
    }
    else if (address >= 0x2000 && address <= 0x3EFF) {
        address &= 0x0FFF;
        
        auto mirror_mode = bus->cart->mirror;

        if (mirror_mode == Cartridge::VERTICAL) {
            if (address >= 0x0800) vram[address - 0x0800] = data;
            else vram[address] = data;
        } else if (mirror_mode == Cartridge::HORIZONTAL) {
            if (address >= 0x0400 && address < 0x0800) vram[address - 0x0400] = data;
            else if (address >= 0x0C00) vram[address - 0x0800] = data;
            else vram[address] = data;
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) { // Palette RAM
        address &= 0x001F;
        if (address == 0x0010) address = 0x0000;
        if (address == 0x0014) address = 0x0004;
        if (address == 0x0018) address = 0x0008;
        if (address == 0x001C) address = 0x000C;
        palette_ram[address] = data;
    }
}

void PPU::increment_scroll_x() {
    if (reg_mask & 0x18) {
        if ((vram_addr_v & 0x001F) == 31) {
            vram_addr_v &= ~0x001F;
            vram_addr_v ^= 0x0400;
        } else {
            vram_addr_v += 1;
        }
    }
}


void PPU::increment_scroll_y() {
    if (reg_mask & 0x18) {
        if ((vram_addr_v & 0x7000) != 0x7000) {
            vram_addr_v += 0x1000;
        } else {
            vram_addr_v &= ~0x7000;

            int y = (vram_addr_v & 0x03E0) >> 5;
            if (y == 29) {
                y = 0;
                vram_addr_v ^= 0x0800;
            } else if (y == 31) {
                y = 0;
            } else {
                y++;
            }
            vram_addr_v = (vram_addr_v & ~0x03E0) | (y << 5);
        }
    }
}

void PPU::transfer_address_x() {
    if (reg_mask & 0x18) {
        vram_addr_v = (vram_addr_v & 0xFBE0) | (vram_addr_t & 0x041F);
    }
}

void PPU::transfer_address_y() {
    if (reg_mask & 0x18) {
        vram_addr_v = (vram_addr_v & 0x841F) | (vram_addr_t & 0x7BE0);
    }
}

void PPU::load_background_shifters() {
    bg_shifter_pattern_lo = (bg_shifter_pattern_lo & 0xFF00) | bg_next_tile_lsb;
    bg_shifter_pattern_hi = (bg_shifter_pattern_hi & 0xFF00) | bg_next_tile_msb;
    bg_shifter_attrib_lo  = (bg_shifter_attrib_lo & 0xFF00) | ((bg_next_tile_attrib & 0b01) ? 0xFF : 0x00);
    bg_shifter_attrib_hi  = (bg_shifter_attrib_hi & 0xFF00) | ((bg_next_tile_attrib & 0b10) ? 0xFF : 0x00);
}

void PPU::update_shifters() {
    if (reg_mask & 0x08) { // Если рендеринг фона включен
        bg_shifter_pattern_lo <<= 1;
        bg_shifter_pattern_hi <<= 1;
        bg_shifter_attrib_lo <<= 1;
        bg_shifter_attrib_hi <<= 1;
    }
}

void PPU::clock() {
    if (scanline >= -1 && scanline < 240) {
        if (scanline == -1 && cycle >= 280 && cycle < 305) {
            if (reg_mask & 0x18) transfer_address_y();
        }

        if ((cycle >= 2 && cycle < 258) || (cycle >= 322 && cycle < 338)) {
            update_shifters();
            
            switch ((cycle - 1) % 8) {
            case 0:
                load_background_shifters();
                bg_next_tile_id = ppu_read(0x2000 | (vram_addr_v & 0x0FFF));
                break;
            case 2:
                bg_next_tile_attrib = ppu_read(0x23C0 | (vram_addr_v & 0x0C00) | ((vram_addr_v >> 4) & 0x38) | ((vram_addr_v >> 2) & 0x07));
                if (vram_addr_v & 0x0040) bg_next_tile_attrib >>= 4;
                if (vram_addr_v & 0x0002) bg_next_tile_attrib >>= 2;
                bg_next_tile_attrib &= 0x03;
                break;
            case 4:
                bg_next_tile_lsb = ppu_read(((uint16_t)(reg_ctrl & 0x10) << 8) + ((uint16_t)bg_next_tile_id << 4) + ((vram_addr_v >> 12) & 0x07));
                break;
            case 6:
                {
                    bg_next_tile_msb = ppu_read(((uint16_t)(reg_ctrl & 0x10) << 8) + ((uint16_t)bg_next_tile_id << 4) + ((vram_addr_v >> 12) & 0x07) + 8);
                    static int load_count = 0;
                    if (load_count < 8) {
                        printf("[PPU Clock] Loaded Tile ID: $%02X, LSB: $%02X, MSB: $%02X\n",
                               bg_next_tile_id, bg_next_tile_lsb, bg_next_tile_msb);
                        load_count++;
                    }
                }
                break;
            case 7:
                if (reg_mask & 0x18) increment_scroll_x();
                break;
            }
        }

        if (cycle == 256 && (reg_mask & 0x18)) {
            increment_scroll_y();
        }
        if (cycle == 257 && (reg_mask & 0x18)) {
            load_background_shifters();
            transfer_address_x();
        }
    }

    if (scanline == 241 && cycle == 1) {
        reg_status |= 0x80;
        if (reg_ctrl & 0x80) bus->nmi();
    }
    if (scanline == -1 && cycle == 1) {
        reg_status &= ~0x80;
    }

    uint8_t bg_pixel = 0, bg_palette = 0;
    if (reg_mask & 0x08) {
        uint16_t bit_mux = 0x8000 >> fine_x_scroll;
        uint8_t p0 = (bg_shifter_pattern_lo & bit_mux) > 0;
        uint8_t p1 = (bg_shifter_pattern_hi & bit_mux) > 0;
        bg_pixel = (p1 << 1) | p0;
        uint8_t pal0 = (bg_shifter_attrib_lo & bit_mux) > 0;
        uint8_t pal1 = (bg_shifter_attrib_hi & bit_mux) > 0;
        bg_palette = (pal1 << 1) | pal0;
    }

    if (cycle - 1 >= 0 && cycle - 1 < 256 && scanline >= 0 && scanline < 240) {
        uint8_t final_pixel = 0;
        uint8_t final_palette = 0;
        
        // TODO: Логика смешивания фона и спрайтов будет здесь
        final_pixel = bg_pixel;
        final_palette = bg_palette;
        
        uint16_t palette_address = 0x3F00 + (final_palette << 2) + final_pixel;
        if (final_pixel == 0) palette_address = 0x3F00;
        
        uint8_t color_index = ppu_read(palette_address);
        screen[scanline * 256 + (cycle - 1)] = nes_palette[color_index];
    }

    cycle++;
    if (cycle > 340) {
        cycle = 0;
        scanline++;
        printf("[PPU Tick] Scanline: %d, V: %04X, T: %04X, Mask: %02X\n",
               scanline, vram_addr_v, vram_addr_t, reg_mask);
        if (scanline > 261) {
            scanline = -1;
            frame_complete = true;
        }
    }
}

void PPU::log_status() {

    printf(" PPU:%3d,%3d",
           scanline,
           cycle);
}
