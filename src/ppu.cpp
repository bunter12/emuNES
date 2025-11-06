#include <ppu.h>
#include <iostream>


void PPU::reset() {
    frame_completed = false;
    scanline = 0;
    cycle = 0;
    odd_frame = false;
    vram_addr = 0;
    temp_vram_addr = 0;
    write_toggle = false;
    fine_x = 0;
    for (int i = 0; i < 256 * 240; i++) {    
        screen[i] = 0;
    }
} 
void PPU::step() {
    if (scanline < 240) {
        if (cycle < 256) {
            render_pixel();
        } else if (cycle == 256) {
            increment_vram_addr();
        } else if (cycle == 257) {
            load_background_shifters();
        } else if (cycle == 321) {            
            evaluate_sprites();
        } else if (cycle == 340) {
            increment_vram_addr();
        } else if (cycle == 341) {
            load_background_shifters();
        }
    } else if (scanline == 261) {
        if (cycle == 1) {
            frame_completed = true;
        }
    }
    cycle++;
    if (cycle > 340) {        
        cycle = 0;
        scanline++;
        if (scanline > 261) {
            scanline = 0;
            odd_frame = !odd_frame;
        }
    }
}                       

void PPU::write_register(uint16_t address, uint8_t data) {
    switch (address) {
        case 0x2000: ctrl = data; break;
        case 0x2001: mask = data; break;
        case 0x2003: oam_addr = data; break;
        case 0x2004: break;
        case 0x2005: break;
        case 0x2006: scroll_x = data; break;
        case 0x2007: scroll_y = data; break;
        case 0x2008: break;
        case 0x2009: break;
        case 0x200A: break;
        case 0x200B: break;
        case 0x200C: break;
        case 0x200D: break;
        case 0x200E: break;
        case 0x200F: break;
        default: std::cout << "Invalid PPU write address: " << std::hex << address << std::endl;
    } 
}

void PPU::write_vram(uint16_t address, uint8_t data) {
    vram[address] = data;
} 

uint8_t PPU::read_register(uint16_t address) {
    switch (address) {                
        case 0x2002: return status; break;
        case 0x2004: return oam[oam_addr]; break;
        case 0x2007: return scroll_y; break;
        default: std::cout << "Invalid PPU read address: " << std::hex << address << std::endl; return 0;
    }
}

uint8_t PPU::read_vram(uint16_t address) {    
    return vram[address];
}  

bool PPU::get_frame_complete(){
    return frame_completed;
} 

void PPU::increment_vram_addr() {    
    if (ctrl & 0x10) {
        vram_addr += 32;
    } else {
        vram_addr += 1;
    }
    if (vram_addr >= 0x4000) {
        vram_addr -= 0x4000;
    }
}

void PPU::load_background_shifters() {
    pattern_shift_reg[0] = pattern_shift_reg[1];
    palette_shift_reg[0] = palette_shift_reg[1];
    pattern_shift_reg[1] = read_vram(vram_addr);
    palette_shift_reg[1] = read_vram(vram_addr + 0x1000);
    vram_addr += 1;
    if (vram_addr >= 0x4000) {
        vram_addr -= 0x4000;
    } 
}

void PPU::update_shifters() {    
    pattern_shift_reg[0] = (pattern_shift_reg[0] >> 1) | (pattern_shift_reg[1] << 7);
    palette_shift_reg[0] = (palette_shift_reg[0] >> 1) | (palette_shift_reg[1] << 7);
    pattern_shift_reg[1] = pattern_shift_reg[1] >> 1;
    palette_shift_reg[1] = palette_shift_reg[1] >> 1;
}

void PPU::evaluate_sprites() {
    for (int i = 0; i < 8; i++) {
        uint8_t sprite = oam[i * 4];
        sprite_buffer[i].y = oam[i * 4 + 1];
        sprite_buffer[i].tile_index = oam[i * 4 + 2];
        sprite_buffer[i].attribute = oam[i * 4 + 3];
        sprite_buffer[i].x = oam[i * 4 + 3];
    }
}

uint32_t* PPU::get_screen_buffer() {
    return screen.data();
}

void PPU::render_pixel() {
    uint8_t pattern = pattern_shift_reg[0] & 1;
    uint8_t palette = palette_shift_reg[0] & mask;
    uint8_t color = palette & 3;
    uint8_t brightness = (palette >> 6) & 3;
    if (brightness == 0) {
        screen[scanline * 256 + cycle] = 0;
    } else {
        screen[scanline * 256 + cycle] = palette;
    }
    update_shifters();
}
