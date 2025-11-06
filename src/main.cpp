
#include "bus.h"
#include "cartridge.h"

int main() {
    // 1. Создаем шину
    Bus bus;
    
    // 2. Создаем картридж из файла
    Cartridge cart("../../nestest.nes");

    // 3. Вставляем картридж в шину
    bus.insert_cartridge(&cart);

    // 4. Сбрасываем CPU (reset прочитает стартовый адрес с картриджа!)
    bus.cpu.reset();

    // 5. Запускаем главный цикл
    while (true) {
        bus.cpu.log_status(); 
        bus.cpu.clock();
        // ... ppu clocks ...
    }

    return 0;
}
