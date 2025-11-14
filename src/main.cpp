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
    
    const int FPS = 60;
    const float FRAME_DURATION_MS = 1000.0f / FPS;
    uint32_t frame_start_ticks;
    int frame_time;
    
    bool quit = false;
    SDL_Event e;
    
    while (!quit) {
        frame_start_ticks = SDL_GetTicks();
        
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                bool pressed = (e.type == SDL_KEYDOWN);

                switch (e.key.keysym.sym) {
                    case SDLK_z:      bus.controller[0].set_button_state(Controller::A, pressed); break;
                    case SDLK_x:      bus.controller[0].set_button_state(Controller::B, pressed); break;
                    case SDLK_a:      bus.controller[0].set_button_state(Controller::SELECT, pressed); break;
                    case SDLK_s:      bus.controller[0].set_button_state(Controller::START, pressed); break;
                    case SDLK_UP:     bus.controller[0].set_button_state(Controller::UP, pressed); break;
                    case SDLK_DOWN:   bus.controller[0].set_button_state(Controller::DOWN, pressed); break;
                    case SDLK_LEFT:   bus.controller[0].set_button_state(Controller::LEFT, pressed); break;
                    case SDLK_RIGHT:  bus.controller[0].set_button_state(Controller::RIGHT, pressed); break;
                }
            }
        }


        
        bus.ppu.frame_complete = false;
        while (!bus.ppu.frame_complete) {
            bus.cpu.clock();
            bus.ppu.clock();
            bus.ppu.clock();
            bus.ppu.clock();
            if(bus.cpu.is_instruction_complete()){
//                bus.cpu.log_status();s
//                bus.ppu.log_status();
//                std::cout<<std::endl;
            }
        }

        SDL_UpdateTexture(texture, nullptr, bus.ppu.get_screen(), 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        int frame_time_ms = SDL_GetTicks() - frame_start_ticks;
        if (frame_time_ms < FRAME_DURATION_MS) {
            SDL_Delay(FRAME_DURATION_MS - frame_time_ms);
        }

    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
