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

static constexpr uint8_t NMI_DELAY_VBLANK_EDGE = 14;
static constexpr uint8_t NMI_DELAY_IMMEDIATE_ENABLE = 7;
static constexpr uint8_t NMI_LATCH_WINDOW = 12;

static inline bool sprite_matches_scanline(uint8_t sprite_y, int16_t target_scanline, uint8_t sprite_height) {
    int16_t row = target_scanline - (int16_t)sprite_y - 1;
    return row >= 0 && row < sprite_height;
}

static inline uint8_t reverse_bits_u8(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

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
            data = (reg_status & 0xE0) | (ppu_data_buffer & 0x1F);
            if (scanline == 241 && cycle == 1) {
                suppress_vblank_set = true;
            }
            reg_status &= ~0x80;
            nmi_occured = false;
            update_nmi_state();
            address_latch_w = false;
            break;
        case 0x0004: // OAMDATA
            data = oam[oam_addr];
            if ((oam_addr & 0x03) == 0x02) {
                data &= 0xE3;
            }
            break;
        case 0x0007: // PPUDATA
            data = ppu_data_buffer;
            if (vram_addr_v >= 0x3F00) {
                data = ppu_read(vram_addr_v);
                ppu_data_buffer = ppu_read(vram_addr_v - 0x1000);
            } else {
                ppu_data_buffer = ppu_read(vram_addr_v);
            }
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
    odd_frame = false;

    nmi_output = false;
    nmi_occured = false;
    nmi_previous = false;
    nmi_delay = 0;
    nmi_delay_immediate = false;
    nmi_latched = false;
    overflow_set_cycle = -1;
    overflow_set_pending = false;
    suppress_vblank_set = false;
    odd_frame_cycle_skip_pending = false;

    sprite_count = 0;
    sprite_count_next = 0;
    sprite_zero_hit_possible = false;
    sprite_zero_hit_possible_next = false;
    sprite_zero_being_rendered = false;

    ppu_data_buffer = 0;

    std::fill(std::begin(oam), std::end(oam), 0xFF);
    std::fill(std::begin(palette_ram), std::end(palette_ram), 0);
    std::fill(std::begin(vram), std::end(vram), 0);
    std::fill(std::begin(secondary_oam), std::end(secondary_oam), OAM_Entry{0xFF,0xFF,0xFF,0xFF});
    std::fill(std::begin(secondary_oam_next), std::end(secondary_oam_next), OAM_Entry{0xFF,0xFF,0xFF,0xFF});
    std::fill(std::begin(sprite_shifter_pattern_lo), std::end(sprite_shifter_pattern_lo), 0);
    std::fill(std::begin(sprite_shifter_pattern_hi), std::end(sprite_shifter_pattern_hi), 0);
    std::fill(std::begin(sprite_shifter_pattern_lo_next), std::end(sprite_shifter_pattern_lo_next), 0);
    std::fill(std::begin(sprite_shifter_pattern_hi_next), std::end(sprite_shifter_pattern_hi_next), 0);
}

void PPU::cpu_write(uint16_t address, uint8_t data) {
    switch (address) {
        case 0x0000: // PPUCTRL
        {
            bool old_nmi_output = nmi_output;
            reg_ctrl = data;
            nmi_output = (reg_ctrl & 0x80) != 0;
            vram_addr_t = (vram_addr_t & 0xF3FF) | ((uint16_t)(data & 0x03) << 10);
            bool immediate_enable = (!old_nmi_output && nmi_output && nmi_occured);
            update_nmi_state(immediate_enable);
            break;
        }
        case 0x0001: // PPUMASK
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
        auto mirror_mode = (bus && bus->cart) ? bus->cart->mirror : Cartridge::HORIZONTAL;
        uint16_t mapped_addr = 0x0000;

        switch (mirror_mode) {
        case Cartridge::VERTICAL:
            mapped_addr = addr & 0x07FF;
            break;
        case Cartridge::HORIZONTAL:
            mapped_addr = ((addr & 0x0800) >> 1) | (addr & 0x03FF);
            break;
        case Cartridge::ONESCREEN_LO:
            mapped_addr = addr & 0x03FF;
            break;
        case Cartridge::ONESCREEN_HI:
            mapped_addr = 0x0400 | (addr & 0x03FF);
            break;
        case Cartridge::FOUR_SCREEN:
        default:
            mapped_addr = addr & 0x0FFF;
            break;
        }

        data = vram[mapped_addr & 0x0FFF];
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
        auto mirror_mode = (bus && bus->cart) ? bus->cart->mirror : Cartridge::HORIZONTAL;
        uint16_t mapped_addr = 0x0000;

        switch (mirror_mode) {
        case Cartridge::VERTICAL:
            mapped_addr = addr & 0x07FF;
            break;
        case Cartridge::HORIZONTAL:
            mapped_addr = ((addr & 0x0800) >> 1) | (addr & 0x03FF);
            break;
        case Cartridge::ONESCREEN_LO:
            mapped_addr = addr & 0x03FF;
            break;
        case Cartridge::ONESCREEN_HI:
            mapped_addr = 0x0400 | (addr & 0x03FF);
            break;
        case Cartridge::FOUR_SCREEN:
        default:
            mapped_addr = addr & 0x0FFF;
            break;
        }

        vram[mapped_addr & 0x0FFF] = data;
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

void PPU::update_nmi_state(bool immediate_enable) {
    bool nmi_now = nmi_output && nmi_occured;
    if (nmi_now && !nmi_previous) {
        nmi_delay = immediate_enable ? NMI_DELAY_IMMEDIATE_ENABLE : NMI_DELAY_VBLANK_EDGE;
        nmi_delay_immediate = immediate_enable;
    } else if (!nmi_now && nmi_previous) {
        if (nmi_delay > NMI_LATCH_WINDOW) {
            nmi_delay = 0;
            nmi_delay_immediate = false;
            nmi_latched = false;
        } else {
            nmi_latched = true;
        }
    }
    nmi_previous = nmi_now;
}

void PPU::clock() {
    if (overflow_set_pending) {
        reg_status |= 0x20;
        overflow_set_pending = false;
    }

    if (nmi_delay > 0) {
        nmi_delay--;
        if (nmi_delay == 0 && bus && ((nmi_output && nmi_occured) || nmi_latched)) {
            bus->nmi(nmi_delay_immediate);
            nmi_delay_immediate = false;
            nmi_latched = false;
        }
    }

    if (scanline == -1 && cycle == 0) {
        reg_status &= ~0x60;
        nmi_occured = false;
        update_nmi_state();
    }

    if (scanline == -1 && cycle == 1) {
        reg_status &= ~0x80;
        sprite_zero_hit_possible = false;
        sprite_zero_being_rendered = false;
    }

    if (cycle == 0 && scanline >= -1 && scanline < 240) {
        overflow_set_cycle = -1;
        overflow_set_pending = false;
        if (reg_mask & 0x18) {
            uint8_t sprite_height = (reg_ctrl & 0x20) ? 16 : 8;
            int16_t eval_scanline = scanline + 1;
            int eval_cycle = 65;
            int sprites_found = 0;
            int eighth_sprite_index = -1;

            for (int i = 0; i < 64 && eval_cycle <= 256; i++) {
                bool in_range = sprite_matches_scanline(oam[i * 4 + 0], eval_scanline, sprite_height);
                if (sprites_found < 8) {
                    if (in_range) {
                        sprites_found++;
                        if (sprites_found == 8) {
                            eighth_sprite_index = i;
                        }
                        eval_cycle += 8;
                    } else {
                        eval_cycle += 2;
                    }
                }
            }

            if (sprites_found == 8 && eighth_sprite_index >= 0) {
                int phase = 0;
                for (int i = eighth_sprite_index + 1; i < 64 && eval_cycle <= 256; i++) {
                    uint8_t candidate_y = oam[i * 4 + phase];
                    if (sprite_matches_scanline(candidate_y, eval_scanline, sprite_height)) {
                        overflow_set_cycle = eval_cycle;
                        break;
                    }
                    eval_cycle += 2;
                    phase = (phase + 1) & 0x03;
                }
            }
        }

        sprite_count = sprite_count_next;
        sprite_zero_hit_possible = sprite_zero_hit_possible_next;
        for (uint8_t i = 0; i < 8; i++) {
            secondary_oam[i] = secondary_oam_next[i];
            sprite_shifter_pattern_lo[i] = sprite_shifter_pattern_lo_next[i];
            sprite_shifter_pattern_hi[i] = sprite_shifter_pattern_hi_next[i];
        }
    }

    if (overflow_set_cycle >= 0 && cycle == overflow_set_cycle) {
        overflow_set_pending = true;
    }

    if (scanline >= -1 && scanline < 240) {

        if ((cycle >= 2 && cycle < 258) || (cycle >= 322 && cycle < 338)) {
            update_shifters();
        }

        if ((cycle >= 1 && cycle < 257) || (cycle >= 321 && cycle < 337)) {
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

        if (cycle == 255) {
            for (auto &entry : secondary_oam_next) entry = {0xFF, 0xFF, 0xFF, 0xFF};
            sprite_count_next = 0;
            sprite_zero_hit_possible_next = false;
            std::fill(std::begin(sprite_shifter_pattern_lo_next), std::end(sprite_shifter_pattern_lo_next), 0);
            std::fill(std::begin(sprite_shifter_pattern_hi_next), std::end(sprite_shifter_pattern_hi_next), 0);

            if (reg_mask & 0x18) {
                uint8_t sprite_height = (reg_ctrl & 0x20) ? 16 : 8;
                int16_t eval_scanline = scanline + 1;

                for (uint8_t i = 0; i < 64; i++) {
                    if (!sprite_matches_scanline(oam[i * 4 + 0], eval_scanline, sprite_height)) {
                        continue;
                    }

                    if (sprite_count_next < 8) {
                        if (i == 0) {
                            sprite_zero_hit_possible_next = true;
                        }
                        secondary_oam_next[sprite_count_next] = *(OAM_Entry*)&oam[i * 4];
                        sprite_count_next++;
                    }
                }

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
            for (uint8_t i = 0; i < sprite_count_next; i++) {
                uint16_t addr_lo, addr_hi;
                uint8_t sprite_height = (reg_ctrl & 0x20) ? 16 : 8;
                bool flip_vert = (secondary_oam_next[i].attribute & 0x80);
                bool flip_horz = (secondary_oam_next[i].attribute & 0x40);
                int16_t fetch_scanline = scanline + 1;

                int16_t row_in_sprite = fetch_scanline - secondary_oam_next[i].y - 1;
                if (row_in_sprite < 0 || row_in_sprite >= sprite_height) {
                    sprite_shifter_pattern_lo_next[i] = 0;
                    sprite_shifter_pattern_hi_next[i] = 0;
                    continue;
                }
                if (flip_vert) {
                    row_in_sprite = (sprite_height - 1) - row_in_sprite;
                }

                if (sprite_height == 8) {
                    uint16_t pattern_table = (reg_ctrl & 0x08) ? 0x1000 : 0x0000;
                    addr_lo = pattern_table + (secondary_oam_next[i].tile_id * 16) + (uint16_t)row_in_sprite;
                } else {
                    uint16_t pattern_table = (secondary_oam_next[i].tile_id & 0x01) ? 0x1000 : 0x0000;
                    uint8_t tile_index = secondary_oam_next[i].tile_id & 0xFE;
                    if (row_in_sprite > 7) {
                        tile_index++;
                        row_in_sprite -= 8;
                    }
                    addr_lo = pattern_table + (tile_index * 16) + (uint16_t)row_in_sprite;
                }
                addr_hi = addr_lo + 8;

                uint8_t bits_lo = ppu_read(addr_lo);
                uint8_t bits_hi = ppu_read(addr_hi);

                if (flip_horz) {
                    bits_lo = reverse_bits_u8(bits_lo);
                    bits_hi = reverse_bits_u8(bits_hi);
                }

                sprite_shifter_pattern_lo_next[i] = bits_lo;
                sprite_shifter_pattern_hi_next[i] = bits_hi;
            }
        }
    }

    if (scanline == 241 && cycle == 1) {
        if (!suppress_vblank_set) {
            reg_status |= 0x80;
            nmi_occured = true;
        }
        suppress_vblank_set = false;
        update_nmi_state();
    }

    uint8_t bg_pixel = 0, bg_palette = 0;
    if (reg_mask & 0x08) {
        uint16_t bit_mux = 0x8000 >> fine_x_scroll;
        bg_pixel = (((bg_shifter_pattern_hi & bit_mux) > 0) << 1) | ((bg_shifter_pattern_lo & bit_mux) > 0);
        bg_palette = (((bg_shifter_attrib_hi & bit_mux) > 0) << 1) | ((bg_shifter_attrib_lo & bit_mux) > 0);
    }
    if (!(reg_mask & 0x02) && cycle >= 1 && cycle <= 8) {
        bg_pixel = 0;
        bg_palette = 0;
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
    if (!(reg_mask & 0x04) && cycle >= 1 && cycle <= 8) {
        sprite_pixel = 0;
    }

    uint8_t final_pixel = 0, final_palette = 0;
    if (bg_pixel == 0 && sprite_pixel == 0) { final_pixel = 0; final_palette = 0; }
    else if (bg_pixel == 0 && sprite_pixel > 0) { final_pixel = sprite_pixel; final_palette = sprite_palette; }
    else if (bg_pixel > 0 && sprite_pixel == 0) { final_pixel = bg_pixel; final_palette = bg_palette; }
    else {
        if (sprite_priority) { final_pixel = sprite_pixel; final_palette = sprite_palette; }
        else { final_pixel = bg_pixel; final_palette = bg_palette; }

        if (sprite_zero_hit_possible && sprite_zero_being_rendered && (reg_mask & 0x18) == 0x18) {
            bool left_edge_enabled = (reg_mask & 0x02) && (reg_mask & 0x04);
            if ((left_edge_enabled && cycle >= 1 && cycle <= 255) ||
                (!left_edge_enabled && cycle >= 9 && cycle <= 255)) {
                reg_status |= 0x40;
            }
        }
    }

    if (cycle - 1 >= 0 && cycle - 1 < 256 && scanline >= 0 && scanline < 240) {
        uint16_t palette_address = 0x3F00 + (final_palette << 2) + final_pixel;
        if (final_pixel == 0) palette_address = 0x3F00;
        uint8_t color_index = ppu_read(palette_address);
        screen[scanline * 256 + (cycle - 1)] = nes_palette[color_index & 0x3F];
    }

    if (cycle >= 1 && cycle <= 256 && (reg_mask & 0x10)) {
        for (int i = 0; i < sprite_count; i++) {
            if (secondary_oam[i].x > 0) {
                secondary_oam[i].x--;
            } else {
                sprite_shifter_pattern_lo[i] <<= 1;
                sprite_shifter_pattern_hi[i] <<= 1;
            }
        }
    }

    if (scanline == -1 && cycle == 338) {
        odd_frame_cycle_skip_pending = odd_frame && ((reg_mask & 0x18) != 0);
    }

    if (scanline == -1 && cycle == 339 && odd_frame_cycle_skip_pending) {
        // On odd frames with rendering enabled, pre-render dot 340 is skipped.
        cycle = 340;
        odd_frame_cycle_skip_pending = false;
    }

    cycle++;
    if (cycle > 340) {
        cycle = 0;
        scanline++;
        if (scanline > 260) {
            scanline = -1;
            frame_complete = true;
            odd_frame = !odd_frame;
        }
    }
}

void PPU::log_status() {
    printf(" PPU:%3d,%3d", scanline, cycle);
}
