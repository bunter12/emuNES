#ifndef PPU_H
#define PPU_H
#include <iostream>
#include <cstdint>

class Bus;

class PPU {
public:
    
    PPU();
    ~PPU();
    
    void connect_bus(Bus* b) { bus = b; }
    void clock();

    uint8_t cpu_read(uint16_t address);
    void cpu_write(uint16_t address, uint8_t data);
    
    uint32_t* get_screen();
    bool frame_complete = false;
    
    int scanline = 0;
    int cycle = 0;
    
    void log_status();
    void reset();
    uint8_t oam[256];
    
private:
    
    uint8_t oam_addr = 0x00;
    
    uint8_t vram[4096];
    uint8_t palette_ram[32];
    
    uint8_t reg_ctrl = 0x00;
    uint8_t reg_mask = 0x00;
    uint8_t reg_status = 0x00;
    
    uint16_t vram_addr_v = 0x0000;
    uint16_t vram_addr_t = 0x0000;
    uint8_t fine_x_scroll = 0;
    bool address_latch_w = false;
    
    struct OAM_Entry {
        uint8_t y;
        uint8_t tile_id;
        uint8_t attribute;
        uint8_t x;
    } secondary_oam[8];
    OAM_Entry secondary_oam_next[8];
        
    uint8_t sprite_count = 0;
    uint8_t sprite_count_next = 0;
        
    uint8_t sprite_shifter_pattern_lo[8];
    uint8_t sprite_shifter_pattern_hi[8];
    uint8_t sprite_shifter_pattern_lo_next[8];
    uint8_t sprite_shifter_pattern_hi_next[8];
    
    bool sprite_zero_hit_possible = false;
    bool sprite_zero_hit_possible_next = false;
    bool sprite_zero_being_rendered = false;
    
    uint16_t bg_shifter_pattern_lo = 0x0000;
    uint16_t bg_shifter_pattern_hi = 0x0000;
    uint16_t bg_shifter_attrib_lo = 0x0000;
    uint16_t bg_shifter_attrib_hi = 0x0000;
    
    uint8_t bg_next_tile_id = 0x00;
    uint8_t bg_next_tile_attrib = 0x00;
    uint8_t bg_next_tile_lsb = 0x00;
    uint8_t bg_next_tile_msb = 0x00;
    

    uint16_t vram_addr = 0x0000;
    uint8_t ppu_data_buffer = 0x00;
    
    uint32_t screen[256 * 240];
    
    
    uint8_t ppu_read(uint16_t address, bool read_only = false);
    void ppu_write(uint16_t address, uint8_t data);
    void increment_scroll_x();
    void increment_scroll_y();
    void transfer_address_x();
    void transfer_address_y();
    void load_background_shifters();
    void update_shifters();
    void update_nmi_state(bool immediate_enable = false);
    
    Bus* bus = nullptr;

    bool odd_frame = false;
    bool nmi_output = false;
    bool nmi_occured = false;
    bool nmi_previous = false;
    uint8_t nmi_delay = 0;
    bool nmi_delay_immediate = false;
    bool nmi_latched = false;
    int overflow_set_cycle = -1;
    bool overflow_set_pending = false;
    bool suppress_vblank_set = false;
    bool odd_frame_cycle_skip_pending = false;

};

#endif //PPU_H
