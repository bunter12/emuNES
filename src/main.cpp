#include <iostream>
#include <SDL2/SDL.h>
#include "bus.h"
#include "cartridge.h"

int main() {
    Bus bus;
    
    Cartridge cart("/Users/kiramsabirzanov/projects/emuNES/smb.nes");
    
    bus.insert_cartridge(&cart);

    bus.cpu.reset();
    bus.ppu.reset();
    bus.ppu.cycle = 21;
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL не смог инициализироваться! Ошибка: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("emuNES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256 * 2, 240 * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Окно не может быть создано! Ошибка: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Рендерер не может быть создан! Ошибка: " << SDL_GetError() << std::endl;
        
        return -1;
    }
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    if (!texture) {
        std::cerr << "Текстура не может быть создана! Ошибка: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    bool quit = false;
    SDL_Event e;
    
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
        bus.cpu.clock();
        bus.ppu.clock();
        bus.ppu.clock();
        bus.ppu.clock();

        // Проверяем, не завершился ли кадр, чтобы его отрисовать
        if (bus.ppu.frame_complete) {
            SDL_UpdateTexture(texture, nullptr, bus.ppu.get_screen(), 256 * sizeof(uint32_t));
            
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
            bus.ppu.frame_complete = false;
        }
        

        bus.ppu.frame_complete = false;
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
