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
    
private:
    std::vector<uint8_t> prg_memory;
    std::vector<uint8_t> chr_memory;
    
    uint8_t mapper_id = 0;
    uint8_t prg_banks = 0;
    uint8_t chr_banks = 0;
};


#endif //CARTRIDGE_H
