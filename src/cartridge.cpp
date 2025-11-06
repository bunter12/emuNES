#include "cartridge.h"
#include <fstream>

Cartridge::Cartridge(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (file.is_open()) {
        file.seekg(16, std::ios::cur);

        prg_memory.resize(16384);
        file.read((char*)prg_memory.data(), prg_memory.size());

        chr_memory.resize(8192);
        file.read((char*)chr_memory.data(), chr_memory.size());
        
        file.close();
    }
}

bool Cartridge::cpu_read(uint16_t address, uint8_t& data) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        data = prg_memory[address & 0x3FFF];
        return true;
    }
    return false;
}

bool Cartridge::cpu_write(uint16_t address, uint8_t data) {
    return false;
}
