#include <ppu.h>
#include <bus.h>
#include <cstdio>
#include <algorithm>
#include <cstring>

static const uint32_t nes_palette[64] = {
    0x666666FF, 0x002A88FF, 0x1412A7FF, 0x3B00A4FF, 0x5C007EFF,
    0x6E0040FF, 0x6C0600FF, 0x561D00FF,
    0x333500FF, 0x0B4800FF, 0x005200FF, 0x004F08FF, 0x00404DFF,
    0x000000FF, 0x000000FF, 0x000000FF,
    0xADADADFF, 0x155FD9FF, 0x4240FFFF, 0x7527FEFF, 0xA01ACCFF,
    0xB71E7BFF, 0xB53120FF, 0x994E00FF,
    0x6B6D00FF, 0x388700FF, 0x0C9300FF, 0x008F32FF, 0x007C8DFF,
    0x000000FF, 0x000000FF, 0x000000FF,
    0xFFFEFFFF, 0x64B0FFFF, 0x9290FFFF, 0xC276FFFF, 0xF368FFFF,
    0xFF6ACCFF, 0xFF8170FF, 0xEAA536FF,
    0xC2C521FF, 0x89D800FF, 0x56E41AFF, 0x45E082FF, 0x48CDDEFF,
    0x4F4F4FFF, 0x000000FF, 0x000000FF,
    0xFFFEFFFF, 0xC0DFFFFF, 0xD3D2FFFF, 0xE8CCFFFF, 0xFBC2FFFF,
    0xFFC4EAFF, 0xFFCCA3FF, 0xF9DB90FF,
    0xE4E594FF, 0xCFEF96FF, 0xBDF4ABFF, 0xB3F3CCFF, 0xB5EBF2FF,
    0xB8B8B8FF, 0x000000FF, 0x000000FF,
};

PPU::PPU() {
    std::memset(screen, 0, sizeof(screen));
    reset();
}
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
        default:
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

    scanline = -1;   // pre-render
    cycle = 0;       // cycles go 0..340
    frame_complete = false;

    sprite_count = 0;
    sprite_zero_hit_possible = false;
    sprite_zero_being_rendered = false;

    ppu_data_buffer = 0;

    std::fill(std::begin(oam), std::end(oam), 0xFF);
    std::fill(std::begin(palette_ram), std::end(palette_ram), 0);
    std::fill(std::begin(vram), std::end(vram), 0);
    std::fill(std::begin(secondary_oam), std::end(secondary_oam), OAM_Entry{0xFF,0xFF,0xFF,0xFF});
}

void PPU::cpu_write(uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // PPUCTRL
            reg_ctrl = data;
            vram_addr_t = (vram_addr_t & 0xF3FF) | ((uint16_t)(data & 0x03) << 10);
            break;
        case 0x0001: // PPUMASK
            reg_mask = data;
            break;
        case 0x0003: // OAMADDR
            oam_addr = data;
            break;
        case 0x0004: // OAMDATA
            oam[oam_addr] = data;
            if (oam_addr >= 0 && oam_addr <= 3) {
                printf("[OAM Write] Addr: $%02X, Data: $%02X\n", oam_addr, data);
            }
            oam_addr++;
            break;
        case 0x0005: // PPUSCROLL
            if (address_latch_w == false) { // Первая запись (X scroll)
                fine_x_scroll = data & 0x07;
                vram_addr_t = (vram_addr_t & 0xFFE0) | (data >> 3);
                address_latch_w = true;
            } else { // Вторая запись (Y scroll)
                vram_addr_t = (vram_addr_t & 0x8C1F) | ((uint16_t)(data & 0x07) << 12) | ((uint16_t)(data & 0xF8) << 2);
                address_latch_w = false;
            }
            break;
        case 0x0006: // PPUADDR
            if (address_latch_w == false) { // Первая запись (старший байт)
                vram_addr_t = (vram_addr_t & 0x00FF) | ((uint16_t)(data & 0x3F) << 8);
                address_latch_w = true;
            } else { // Вторая запись (младший байт)
                vram_addr_t = (vram_addr_t & 0xFF00) | data;
                vram_addr_v = vram_addr_t; // Сразу копируем в 'v'
                address_latch_w = false;
            }
            break;
        case 0x0007: // PPUDATA
            ppu_write(vram_addr_v, data);
            vram_addr_v += (reg_ctrl & 0x04) ? 32 : 1;
            break;
        default:
            break;
    }
}

uint8_t PPU::ppu_read(uint16_t address, bool read_only) {
    uint8_t data = 0x00;
    address &= 0x3FFF;

    if (bus && bus->ppu_read(address, data)) {
        return data;
    }

    if (address >= 0x2000 && address <= 0x3EFF) {
        uint16_t addr = address & 0x0FFF;
        auto mirror_mode = bus->cart->mirror;

        if (mirror_mode == Cartridge::FOUR_SCREEN) {
            data = vram[addr];
        } else if (mirror_mode == Cartridge::VERTICAL) {
            if (addr >= 0x0800) data = vram[addr - 0x0800];
            else data = vram[addr];
        } else { // HORIZONTAL
            if (addr >= 0x0C00) data = vram[addr - 0x0800];
            else if (addr >= 0x0400 && addr < 0x0800) data = vram[addr - 0x0400];
            else data = vram[addr];
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) {
        uint16_t pal = address & 0x001F;
        if (pal == 0x0010) pal = 0x0000;
        if (pal == 0x0014) pal = 0x0004;
        if (pal == 0x0018) pal = 0x0008;
        if (pal == 0x001C) pal = 0x000C;
        data = palette_ram[pal];
    }

    return data;
}

void PPU::ppu_write(uint16_t address, uint8_t data) {
    address &= 0x3FFF;

    if (bus && bus->ppu_write(address, data)) {
        return;
    }

    if (address >= 0x2000 && address <= 0x3EFF) {
        uint16_t addr = address & 0x0FFF;
        auto mirror_mode = bus->cart->mirror;
        if (mirror_mode == Cartridge::VERTICAL) {
            if (addr >= 0x0800) vram[addr - 0x0800] = data;
            else vram[addr] = data;
        } else if (mirror_mode == Cartridge::HORIZONTAL) {
            if (addr >= 0x0C00) vram[addr - 0x0800] = data;
            else if (addr >= 0x0400 && addr < 0x0800) vram[addr - 0x0400] = data;
            else vram[addr] = data;
        } else {
            vram[addr] = data;
        }
    }
    else if (address >= 0x3F00 && address <= 0x3FFF) {
        uint16_t pal = address & 0x001F;
        if (pal == 0x0010) pal = 0x0000;
        if (pal == 0x0014) pal = 0x0004;
        if (pal == 0x0018) pal = 0x0008;
        if (pal == 0x001C) pal = 0x000C;
        palette_ram[pal] = data;
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
    if (reg_mask & 0x08) {
        bg_shifter_pattern_hi <<= 1;
        bg_shifter_pattern_lo <<= 1;
        bg_shifter_attrib_lo <<= 1;
        bg_shifter_attrib_hi <<= 1;
    }
}

void PPU::clock() {
    if (scanline == -1 && cycle == 1) {
        reg_status &= ~0xE0;
        sprite_zero_hit_possible = false;
        sprite_zero_being_rendered = false;
    }

    if (scanline >= -1 && scanline < 240) {

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
                    bg_next_tile_msb = ppu_read(((uint16_t)(reg_ctrl & 0x10) << 8) + ((uint16_t)bg_next_tile_id << 4) + ((vram_addr_v >> 12) & 0x07) + 8);
                    break;
                case 7:
                    if (reg_mask & 0x18) increment_scroll_x();
                    break;
            }
        }

        if (cycle >= 1 && cycle <= 256) {
            if (reg_mask & 0x10) {
                for (int i = 0; i < sprite_count; i++) {
                    if (secondary_oam[i].x > 0) {
                        secondary_oam[i].x--;
                    } else {
                        sprite_shifter_pattern_lo[i] <<= 1;
                        sprite_shifter_pattern_hi[i] <<= 1;
                    }
                }
            }
        }

        if (cycle == 257) {
            if (reg_mask & 0x10) {
                for (auto &entry : secondary_oam) entry = {0xFF, 0xFF, 0xFF, 0xFF};
                sprite_count = 0;
                sprite_zero_hit_possible = false;

                uint8_t oam_entry_count = 0;
                bool sprite_overflow = false;

                for (uint8_t i = 0; i < 64; i++) {
                    int16_t diff = scanline - (int16_t)oam[i * 4 + 0];
                    uint8_t sprite_height = (reg_ctrl & 0x20) ? 16 : 8;
                    if (i == 0) {
                    }
                    if (diff >= 0 && diff < sprite_height) {
                        if (oam_entry_count < 8) {
                            if (i == 0) {
                                 sprite_zero_hit_possible = true;
                             }
                            secondary_oam[oam_entry_count] = *(OAM_Entry*)&oam[i * 4];
                            oam_entry_count++;
                        } else {
                            sprite_overflow = true;
                        }
                    }
                }
                sprite_count = oam_entry_count;
                if (sprite_overflow) reg_status |= 0x20;
            }
        }

        if (cycle == 256 && (reg_mask & 0x18)) {
            increment_scroll_y();
        }
        if (cycle == 257 && (reg_mask & 0x18)) {
            load_background_shifters();
            transfer_address_x();
        }
        if (scanline == -1 && cycle >= 280 && cycle < 305) {
            if (reg_mask & 0x18) transfer_address_y();
        }

        if (cycle == 340) {
            for (uint8_t i = 0; i < sprite_count; i++) {
                uint16_t addr_lo, addr_hi;
                uint8_t sprite_height = (reg_ctrl & 0x20) ? 16 : 8;
                bool flip_vert = (secondary_oam[i].attribute & 0x80);
                bool flip_horz = (secondary_oam[i].attribute & 0x40);

                uint8_t row_in_sprite = scanline - secondary_oam[i].y;
                if (flip_vert) {
                    row_in_sprite = (sprite_height - 1) - row_in_sprite;
                }

                if (sprite_height == 8) {
                    uint16_t pattern_table = (reg_ctrl & 0x08) ? 0x1000 : 0x0000;
                    addr_lo = pattern_table + (secondary_oam[i].tile_id * 16) + row_in_sprite;
                } else {
                    uint16_t pattern_table = (secondary_oam[i].tile_id & 0x01) ? 0x1000 : 0x0000;
                    uint8_t tile_index = secondary_oam[i].tile_id & 0xFE;
                    if (row_in_sprite > 7) {
                        tile_index++;
                        row_in_sprite -= 8;
                    }
                    addr_lo = pattern_table + (tile_index * 16) + row_in_sprite;
                }
                addr_hi = addr_lo + 8;

                uint8_t bits_lo = ppu_read(addr_lo);
                uint8_t bits_hi = ppu_read(addr_hi);

                if (flip_horz) {
                    auto reverse_bits = [](uint8_t b) {
                        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
                        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
                        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
                        return b;
                    };
                    bits_lo = reverse_bits(bits_lo);
                    bits_hi = reverse_bits(bits_hi);
                }

                sprite_shifter_pattern_lo[i] = bits_lo;
                sprite_shifter_pattern_hi[i] = bits_hi;
            }
        }
    }

    if (scanline == 241 && cycle == 1) {
        reg_status |= 0x80;
        if (reg_ctrl & 0x80) {
            if (bus) bus->nmi();
        }
    }

    uint8_t bg_pixel = 0, bg_palette = 0;
    if (reg_mask & 0x08) {
        uint16_t bit_mux = 0x8000 >> fine_x_scroll;
        bg_pixel = (((bg_shifter_pattern_hi & bit_mux) > 0) << 1) | ((bg_shifter_pattern_lo & bit_mux) > 0);
        bg_palette = (((bg_shifter_attrib_hi & bit_mux) > 0) << 1) | ((bg_shifter_attrib_lo & bit_mux) > 0);
    }

    uint8_t sprite_pixel = 0, sprite_palette = 0, sprite_priority = 0;
    sprite_zero_being_rendered = false;
    if (reg_mask & 0x10) {
        for (uint8_t i = 0; i < sprite_count; i++) {
            if (secondary_oam[i].x == 0) {
                uint8_t pixel_val = ((((sprite_shifter_pattern_hi[i] & 0x80) > 0) << 1) | ((sprite_shifter_pattern_lo[i] & 0x80) > 0));
                if (pixel_val != 0) {
                    sprite_pixel = pixel_val;
                    sprite_palette = (secondary_oam[i].attribute & 0x03) + 4;
                    sprite_priority = (secondary_oam[i].attribute & 0x20) == 0;
                    if (i == 0) {
                        sprite_zero_being_rendered = true;
                    }
                    break;
                }
            }
        }
    }

    uint8_t final_pixel = 0, final_palette = 0;
    if (bg_pixel == 0 && sprite_pixel == 0) { final_pixel = 0; final_palette = 0; }
    else if (bg_pixel == 0 && sprite_pixel > 0) { final_pixel = sprite_pixel; final_palette = sprite_palette; }
    else if (bg_pixel > 0 && sprite_pixel == 0) { final_pixel = bg_pixel; final_palette = bg_palette; }
    else {
        if (sprite_priority) { final_pixel = sprite_pixel; final_palette = sprite_palette; }
        else { final_pixel = bg_pixel; final_palette = bg_palette; }

        if (sprite_zero_hit_possible && sprite_zero_being_rendered && (reg_mask & 0x18) == 0x18) {
            if (cycle - 1 >= 8 && cycle - 1 < 255) {
                reg_status |= 0x40; // sprite zero hit
            }
        }
    }

    if (cycle - 1 >= 0 && cycle - 1 < 256 && scanline >= 0 && scanline < 240) {
        uint16_t palette_address = 0x3F00 + (final_palette << 2) + final_pixel;
        if (final_pixel == 0) palette_address = 0x3F00;
        uint8_t color_index = ppu_read(palette_address);
        screen[scanline * 256 + (cycle - 1)] = nes_palette[color_index & 0x3F];
    }

    cycle++;
    if (cycle > 340) {
        cycle = 0;
        scanline++;
        if (scanline > 260) {
            scanline = -1;
            frame_complete = true;
        }
    }
}

void PPU::log_status() {
    printf(" PPU:%3d,%3d", scanline, cycle);
}
