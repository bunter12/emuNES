#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>

class Cartridge {
public:
    
    enum MIRROR {
         HORIZONTAL,
         VERTICAL,
         FOUR_SCREEN,
         ONESCREEN_LO,
         ONESCREEN_HI,
     } mirror = HORIZONTAL;
    
    Cartridge(const std::string& filename);

    bool cpu_read(uint16_t address, uint8_t& data);
    bool cpu_write(uint16_t address, uint8_t data);
    
    bool ppu_read(uint16_t address, uint8_t& data);
    bool ppu_write(uint16_t address, uint8_t data);

    bool irq_asserted() const;
    
private:
    std::vector<uint8_t> prg_memory;
    std::vector<uint8_t> chr_memory;
    std::vector<uint8_t> prg_ram;
    
    uint8_t mapper_id = 0;
    uint8_t prg_banks = 0;
    uint8_t chr_banks = 0;

    // Mapper 1 (MMC1) state
    uint8_t mmc1_shift = 0x10;
    uint8_t mmc1_control = 0x0C;
    uint8_t mmc1_chr_bank0 = 0x00;
    uint8_t mmc1_chr_bank1 = 0x00;
    uint8_t mmc1_prg_bank = 0x00;

    // Mapper 2 (UxROM) state
    uint8_t mapper2_prg_bank = 0x00;

    // Mapper 3 (CNROM) state
    uint8_t mapper3_chr_bank = 0x00;

    // Mapper 4 (MMC3) state
    uint8_t mmc3_bank_select = 0x00;
    uint8_t mmc3_bank_regs[8] = {0};
    uint8_t mmc3_irq_latch = 0x00;
    uint8_t mmc3_irq_counter = 0x00;
    bool mmc3_irq_reload = false;
    bool mmc3_irq_enabled = false;
    bool mmc3_irq_pending = false;
    bool mmc3_prev_a12 = false;

    void update_mirroring_from_mmc1();
    size_t map_mmc3_prg(uint16_t address) const;
    size_t map_mmc3_chr(uint16_t address) const;
    void mmc3_clock_irq(uint16_t ppu_address);
};


#endif //CARTRIDGE_H
