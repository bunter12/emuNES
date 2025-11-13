#include "cartridge.h"
#include <fstream>
#include <iostream>
#include <cstdio>

Cartridge::Cartridge(const std::string& filename) {
    
    struct Header {
        char     name[4];
        uint8_t  prg_rom_chunks;
        uint8_t  chr_rom_chunks;
        uint8_t  mapper1;
        uint8_t  mapper2;
        uint8_t  prg_ram_size;
        uint8_t  tv_system1;
        uint8_t  tv_system2;
        uint8_t  unused[5];
    } header;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM file: " << filename << std::endl;
        return;
    }

    file.read((char*)&header, sizeof(Header));
    
    if (std::memcmp(header.name, "NES\x1a", 4) != 0) {
        std::cerr << "Error: Not a valid iNES file." << std::endl;
        file.close();
        return;
    }
    
    mapper_id = ((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4);
    
    if (header.mapper1 & 0x08) {
        mirror = FOUR_SCREEN;
    } else if (header.mapper1 & 0x01) {
        mirror = VERTICAL;
    } else {
        mirror = HORIZONTAL;
    }
    
    if (header.mapper1 & 0x04) {
        file.seekg(512, std::ios_base::cur);
    }

    prg_banks = header.prg_rom_chunks;
    prg_memory.resize(prg_banks * 16384);
    std::cout<<"Size prg memory: "<<prg_memory.size();
    file.read((char*)prg_memory.data(), prg_memory.size());

    chr_banks = header.chr_rom_chunks;
    if (chr_banks == 0) {
        chr_memory.resize(8192);
    } else {
        chr_memory.resize(chr_banks * 8192);
        file.read((char*)chr_memory.data(), chr_memory.size());
    }

    file.close();

    std::cout << "ROM Loaded. Mapper: " << (int)mapper_id
              << ", PRG Banks: " << (int)prg_banks
              << ", CHR Banks: " << (int)chr_banks
              << ", Mirroring: " << (mirror == VERTICAL ? "Vertical" : "Horizontal") << std::endl;
}

bool Cartridge::cpu_read(uint16_t address, uint8_t& data) {
    if (address >= 0x8000 && address <= 0xFFFF) {
        uint16_t mapped_addr = address - 0x8000;
        
        if (prg_banks == 1) {
            mapped_addr &= 0x3FFF;
        }

        if (mapped_addr < prg_memory.size()) {
            data = prg_memory[mapped_addr];
            return true;
        }
    }
    return false;
}

bool Cartridge::cpu_write(uint16_t address, uint8_t data) {
    return false;
}

bool Cartridge::ppu_read(uint16_t address, uint8_t& data) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        data = chr_memory[address]; 
        return true;
    }
    return false;
}

bool Cartridge::ppu_write(uint16_t address, uint8_t data) {
    if (address >= 0x0000 && address <= 0x1FFF) {
        if (chr_banks == 0) {
            chr_memory[address] = data;
            return true;
        }
    }
    return false;
}
