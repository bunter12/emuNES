#include "cartridge.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& filename) {
    struct Header {
        char name[4];
        uint8_t prg_rom_chunks;
        uint8_t chr_rom_chunks;
        uint8_t mapper1;
        uint8_t mapper2;
        uint8_t prg_ram_size;
        uint8_t tv_system1;
        uint8_t tv_system2;
        uint8_t unused[5];
    } header{};

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open ROM file: " << filename << std::endl;
        return;
    }

    file.read(reinterpret_cast<char*>(&header), sizeof(Header));
    if (std::memcmp(header.name, "NES\x1a", 4) != 0) {
        std::cerr << "Error: Not a valid iNES file." << std::endl;
        return;
    }

    mapper_id = static_cast<uint8_t>(((header.mapper2 >> 4) << 4) | (header.mapper1 >> 4));

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
    prg_memory.resize(static_cast<size_t>(prg_banks) * 16384);
    file.read(reinterpret_cast<char*>(prg_memory.data()), static_cast<std::streamsize>(prg_memory.size()));

    chr_banks = header.chr_rom_chunks;
    if (chr_banks == 0) {
        chr_memory.resize(8192);
    } else {
        chr_memory.resize(static_cast<size_t>(chr_banks) * 8192);
        file.read(reinterpret_cast<char*>(chr_memory.data()), static_cast<std::streamsize>(chr_memory.size()));
    }

    uint8_t prg_ram_banks = header.prg_ram_size == 0 ? 1 : header.prg_ram_size;
    prg_ram.resize(static_cast<size_t>(prg_ram_banks) * 8192, 0x00);

    // MMC1 power-up state
    mmc1_shift = 0x10;
    mmc1_control = 0x0C;
    mmc1_chr_bank0 = 0x00;
    mmc1_chr_bank1 = 0x00;
    mmc1_prg_bank = 0x00;
    mapper2_prg_bank = 0x00;
    mapper3_chr_bank = 0x00;
    mmc3_bank_select = 0x00;
    std::fill(std::begin(mmc3_bank_regs), std::end(mmc3_bank_regs), 0);
    mmc3_bank_regs[6] = 0;
    mmc3_bank_regs[7] = 1;
    mmc3_irq_latch = 0;
    mmc3_irq_counter = 0;
    mmc3_irq_reload = false;
    mmc3_irq_enabled = false;
    mmc3_irq_pending = false;
    mmc3_prev_a12 = false;
    if (mapper_id == 1) {
        update_mirroring_from_mmc1();
    }

    const char* mirror_name = "Other";
    switch (mirror) {
    case VERTICAL:
        mirror_name = "Vertical";
        break;
    case HORIZONTAL:
        mirror_name = "Horizontal";
        break;
    case ONESCREEN_LO:
        mirror_name = "OneScreen-Low";
        break;
    case ONESCREEN_HI:
        mirror_name = "OneScreen-High";
        break;
    case FOUR_SCREEN:
        mirror_name = "FourScreen";
        break;
    }

    std::cout << "ROM Loaded. Mapper: " << static_cast<int>(mapper_id)
              << ", PRG Banks: " << static_cast<int>(prg_banks)
              << ", CHR Banks: " << static_cast<int>(chr_banks)
              << ", Mirroring: " << mirror_name
              << std::endl;
}

void Cartridge::update_mirroring_from_mmc1() {
    switch (mmc1_control & 0x03) {
    case 0:
        mirror = ONESCREEN_LO;
        break;
    case 1:
        mirror = ONESCREEN_HI;
        break;
    case 2:
        mirror = VERTICAL;
        break;
    case 3:
    default:
        mirror = HORIZONTAL;
        break;
    }
}

size_t Cartridge::map_mmc3_prg(uint16_t address) const {
    const size_t prg_bank_count = std::max<size_t>(1, prg_memory.size() / 0x2000);
    const size_t last = prg_bank_count - 1;
    const size_t second_last = (prg_bank_count > 1) ? (prg_bank_count - 2) : last;

    const size_t bank6 = mmc3_bank_regs[6] % prg_bank_count;
    const size_t bank7 = mmc3_bank_regs[7] % prg_bank_count;
    const bool prg_mode = (mmc3_bank_select & 0x40) != 0;

    size_t slot_bank = 0;
    if (address < 0xA000) {
        slot_bank = prg_mode ? second_last : bank6;
    } else if (address < 0xC000) {
        slot_bank = bank7;
    } else if (address < 0xE000) {
        slot_bank = prg_mode ? bank6 : second_last;
    } else {
        slot_bank = last;
    }

    return (slot_bank * 0x2000 + (address & 0x1FFF)) % prg_memory.size();
}

size_t Cartridge::map_mmc3_chr(uint16_t address) const {
    const size_t chr_bank_count = std::max<size_t>(1, chr_memory.size() / 0x0400);
    const bool chr_inversion = (mmc3_bank_select & 0x80) != 0;
    const uint8_t r0_even = static_cast<uint8_t>(mmc3_bank_regs[0] & 0xFE);
    const uint8_t r1_even = static_cast<uint8_t>(mmc3_bank_regs[1] & 0xFE);

    uint8_t banks[8] = {0};
    if (!chr_inversion) {
        banks[0] = r0_even;
        banks[1] = static_cast<uint8_t>(r0_even + 1);
        banks[2] = r1_even;
        banks[3] = static_cast<uint8_t>(r1_even + 1);
        banks[4] = mmc3_bank_regs[2];
        banks[5] = mmc3_bank_regs[3];
        banks[6] = mmc3_bank_regs[4];
        banks[7] = mmc3_bank_regs[5];
    } else {
        banks[0] = mmc3_bank_regs[2];
        banks[1] = mmc3_bank_regs[3];
        banks[2] = mmc3_bank_regs[4];
        banks[3] = mmc3_bank_regs[5];
        banks[4] = r0_even;
        banks[5] = static_cast<uint8_t>(r0_even + 1);
        banks[6] = r1_even;
        banks[7] = static_cast<uint8_t>(r1_even + 1);
    }

    const size_t slot = (address >> 10) & 0x07;
    const size_t bank = banks[slot] % chr_bank_count;
    return (bank * 0x0400 + (address & 0x03FF)) % chr_memory.size();
}

void Cartridge::mmc3_clock_irq(uint16_t ppu_address) {
    const bool a12 = (ppu_address & 0x1000) != 0;
    if (a12 && !mmc3_prev_a12) {
        if (mmc3_irq_counter == 0 || mmc3_irq_reload) {
            mmc3_irq_counter = mmc3_irq_latch;
        } else {
            mmc3_irq_counter--;
        }
        if (mmc3_irq_counter == 0 && mmc3_irq_enabled) {
            mmc3_irq_pending = true;
        }
        mmc3_irq_reload = false;
    }
    mmc3_prev_a12 = a12;
}

bool Cartridge::irq_asserted() const {
    return mapper_id == 4 && mmc3_irq_pending;
}

bool Cartridge::cpu_read(uint16_t address, uint8_t& data) {
    if (address >= 0x6000 && address <= 0x7FFF && !prg_ram.empty()) {
        data = prg_ram[(address - 0x6000) % prg_ram.size()];
        return true;
    }

    if (address < 0x8000 || prg_memory.empty()) {
        return false;
    }

    if (mapper_id == 0) {
        size_t mapped_addr = address - 0x8000;
        if (prg_banks == 1) {
            mapped_addr &= 0x3FFF;
        }
        data = prg_memory[mapped_addr % prg_memory.size()];
        return true;
    }

    if (mapper_id == 1) {
        const uint8_t prg_mode = (mmc1_control >> 2) & 0x03;
        const size_t bank16_count = std::max<size_t>(1, prg_memory.size() / 0x4000);
        const size_t bank16_last = bank16_count - 1;
        size_t mapped_addr = 0;

        if (prg_mode == 0 || prg_mode == 1) {
            const size_t bank32 = ((mmc1_prg_bank & 0x0E) >> 1) % std::max<size_t>(1, bank16_count / 2);
            mapped_addr = bank32 * 0x8000 + (address - 0x8000);
        } else if (prg_mode == 2) {
            if (address < 0xC000) {
                mapped_addr = address - 0x8000;
            } else {
                const size_t bank = (mmc1_prg_bank & 0x0F) % bank16_count;
                mapped_addr = bank * 0x4000 + (address - 0xC000);
            }
        } else {
            if (address < 0xC000) {
                const size_t bank = (mmc1_prg_bank & 0x0F) % bank16_count;
                mapped_addr = bank * 0x4000 + (address - 0x8000);
            } else {
                mapped_addr = bank16_last * 0x4000 + (address - 0xC000);
            }
        }

        data = prg_memory[mapped_addr % prg_memory.size()];
        return true;
    }

    if (mapper_id == 2) {
        const size_t bank16_count = std::max<size_t>(1, prg_memory.size() / 0x4000);
        const size_t last_bank = bank16_count - 1;
        size_t mapped_addr = 0;
        if (address < 0xC000) {
            const size_t bank = mapper2_prg_bank % bank16_count;
            mapped_addr = bank * 0x4000 + (address - 0x8000);
        } else {
            mapped_addr = last_bank * 0x4000 + (address - 0xC000);
        }
        data = prg_memory[mapped_addr % prg_memory.size()];
        return true;
    }

    if (mapper_id == 3) {
        size_t mapped_addr = address - 0x8000;
        if (prg_banks == 1) {
            mapped_addr &= 0x3FFF;
        }
        data = prg_memory[mapped_addr % prg_memory.size()];
        return true;
    }

    if (mapper_id == 4) {
        data = prg_memory[map_mmc3_prg(address)];
        return true;
    }

    return false;
}

bool Cartridge::cpu_write(uint16_t address, uint8_t data) {
    if (address >= 0x6000 && address <= 0x7FFF && !prg_ram.empty()) {
        prg_ram[(address - 0x6000) % prg_ram.size()] = data;
        return true;
    }

    if (mapper_id == 0) {
        return false;
    }

    if (mapper_id == 1) {
        if (address < 0x8000) {
            return false;
        }

        if (data & 0x80) {
            mmc1_shift = 0x10;
            mmc1_control |= 0x0C;
            update_mirroring_from_mmc1();
            return true;
        }

        const bool complete = (mmc1_shift & 0x01) != 0;
        mmc1_shift >>= 1;
        mmc1_shift |= static_cast<uint8_t>((data & 0x01) << 4);

        if (complete) {
            const uint8_t target = (address >> 13) & 0x03;
            const uint8_t value = mmc1_shift;
            switch (target) {
            case 0:
                mmc1_control = value;
                update_mirroring_from_mmc1();
                break;
            case 1:
                mmc1_chr_bank0 = value;
                break;
            case 2:
                mmc1_chr_bank1 = value;
                break;
            case 3:
                mmc1_prg_bank = value;
                break;
            }
            mmc1_shift = 0x10;
        }

        return true;
    }

    if (mapper_id == 2) {
        if (address >= 0x8000) {
            mapper2_prg_bank = data & 0x0F;
            return true;
        }
        return false;
    }

    if (mapper_id == 3) {
        if (address >= 0x8000) {
            mapper3_chr_bank = data;
            return true;
        }
        return false;
    }

    if (mapper_id == 4) {
        if (address < 0x8000) {
            return false;
        }

        if (address <= 0x9FFF) {
            if ((address & 0x01) == 0) {
                mmc3_bank_select = data;
            } else {
                mmc3_bank_regs[mmc3_bank_select & 0x07] = data;
            }
            return true;
        }

        if (address <= 0xBFFF) {
            if ((address & 0x01) == 0) {
                if (mirror != FOUR_SCREEN) {
                    mirror = (data & 0x01) ? HORIZONTAL : VERTICAL;
                }
            }
            return true;
        }

        if (address <= 0xDFFF) {
            if ((address & 0x01) == 0) {
                mmc3_irq_latch = data;
            } else {
                mmc3_irq_reload = true;
            }
            return true;
        }

        if ((address & 0x01) == 0) {
            mmc3_irq_enabled = false;
            mmc3_irq_pending = false;
        } else {
            mmc3_irq_enabled = true;
        }
        return true;
    }

    return false;
}

bool Cartridge::ppu_read(uint16_t address, uint8_t& data) {
    if (address > 0x1FFF || chr_memory.empty()) {
        return false;
    }

    if (mapper_id == 0 || mapper_id == 2) {
        data = chr_memory[address % chr_memory.size()];
        return true;
    }

    if (mapper_id == 1) {
        const uint8_t chr_mode = (mmc1_control >> 4) & 0x01;
        size_t mapped_addr = 0;

        if (chr_mode == 0) {
            mapped_addr = static_cast<size_t>(mmc1_chr_bank0 & 0x1E) * 0x1000 + address;
        } else if (address < 0x1000) {
            mapped_addr = static_cast<size_t>(mmc1_chr_bank0) * 0x1000 + address;
        } else {
            mapped_addr = static_cast<size_t>(mmc1_chr_bank1) * 0x1000 + (address - 0x1000);
        }

        data = chr_memory[mapped_addr % chr_memory.size()];
        return true;
    }

    if (mapper_id == 3) {
        const size_t bank8_count = std::max<size_t>(1, chr_memory.size() / 0x2000);
        const size_t bank = mapper3_chr_bank % bank8_count;
        const size_t mapped_addr = bank * 0x2000 + address;
        data = chr_memory[mapped_addr % chr_memory.size()];
        return true;
    }

    if (mapper_id == 4) {
        mmc3_clock_irq(address);
        data = chr_memory[map_mmc3_chr(address)];
        return true;
    }

    return false;
}

bool Cartridge::ppu_write(uint16_t address, uint8_t data) {
    if (address > 0x1FFF || chr_memory.empty()) {
        return false;
    }

    if (mapper_id == 0 || mapper_id == 2) {
        if (chr_banks == 0) {
            chr_memory[address % chr_memory.size()] = data;
            return true;
        }
        return false;
    }

    if (mapper_id == 1) {
        if (chr_banks != 0) {
            return false;
        }

        const uint8_t chr_mode = (mmc1_control >> 4) & 0x01;
        size_t mapped_addr = 0;

        if (chr_mode == 0) {
            mapped_addr = static_cast<size_t>(mmc1_chr_bank0 & 0x1E) * 0x1000 + address;
        } else if (address < 0x1000) {
            mapped_addr = static_cast<size_t>(mmc1_chr_bank0) * 0x1000 + address;
        } else {
            mapped_addr = static_cast<size_t>(mmc1_chr_bank1) * 0x1000 + (address - 0x1000);
        }

        chr_memory[mapped_addr % chr_memory.size()] = data;
        return true;
    }

    if (mapper_id == 3) {
        if (chr_banks == 0) {
            const size_t bank8_count = std::max<size_t>(1, chr_memory.size() / 0x2000);
            const size_t bank = mapper3_chr_bank % bank8_count;
            const size_t mapped_addr = bank * 0x2000 + address;
            chr_memory[mapped_addr % chr_memory.size()] = data;
            return true;
        }
        return false;
    }

    if (mapper_id == 4) {
        mmc3_clock_irq(address);
        if (chr_banks == 0) {
            chr_memory[map_mmc3_chr(address)] = data;
            return true;
        }
        return false;
    }

    return false;
}
