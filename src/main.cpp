
#include "bus.h"
#include "cartridge.h"
#include <iostream>

int main() {
    Bus bus;
    
    Cartridge cart("../../smb.nes");

    bus.insert_cartridge(&cart);

    bus.cpu.reset();
    bus.ppu.cycle = 21;
    unsigned long int cyc =0;
    while (true) {
        bus.cpu.log_status();
        bus.ppu.log_status();
        std::cout<<std::endl;
        int cpu_cycles = bus.cpu.clock();
        
        for (int i = 0; i < cpu_cycles * 3; ++i) {
            bus.ppu.clock();
        }
    }

    return 0;
}
