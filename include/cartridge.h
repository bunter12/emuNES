#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>

class Cartridge {
public:
    Cartridge(const std::string& filename);

    bool cpu_read(uint16_t address, uint8_t& data);
    bool cpu_write(uint16_t address, uint8_t data);
    
private:
    std::vector<uint8_t> prg_memory;
    std::vector<uint8_t> chr_memory;
};


#endif //CARTRIDGE_H
