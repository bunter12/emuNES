#ifndef PPU_H
#define PPU_H
#include <iostream>
#include <array>
#include <cstdint>

class PPU {
public:
    void write_register(uint16_t address, uint8_t data);
    uint8_t read_register(uint16_t address);

    void write_vram(uint16_t address, uint8_t data);
    uint8_t read_vram(uint16_t address);

    void step();
    void reset();

    uint32_t* get_screen_buffer();
    bool get_frame_complete();
    void render_pixel();
private:
    std::array<uint8_t, 0x4000> vram;
    std::array<uint8_t, 0x100> oam;
    std::array<uint8_t, 0x20> palette;
    
    uint8_t ctrl;
    uint8_t mask;
    uint8_t status;
    uint8_t oam_addr;
    uint8_t scroll_x;
    uint8_t scroll_y;
    uint16_t vram_addr;
    uint8_t vram_data;
    
    uint16_t temp_vram_addr;
    bool write_toggle;
    uint8_t fine_x;
    
    int scanline;
    int cycle;
    bool odd_frame;

    uint16_t pattern_shift_reg[2];
    uint8_t palette_shift_reg[2];
    uint8_t title_data;

    struct Sprite {
        uint8_t y;
        uint8_t tile_index;
        uint8_t attribute;
        uint8_t x;
    };
    std::array<Sprite, 8> sprite_buffer;
    std::array<uint32_t, 256 * 240> screen;
    bool frame_completed;

    void increment_vram_addr();
    void update_shifters();
    void load_background_shifters();
    void evaluate_sprites();
};

#endif //PPU_H
