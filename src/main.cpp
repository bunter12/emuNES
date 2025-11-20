#include <iostream>
#include <SDL2/SDL.h>
#include "bus.h"
#include "cartridge.h"

Bus bus;

int main(int argc, char* argv[]) {

    Cartridge cart("/Users/kiramsabirzanov/projects/emuNES/dk.nes");

    bus.insert_cartridge(&cart);
    bus.cpu.reset();
    bus.ppu.reset();
    bus.ppu.cycle = 21;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_AudioSpec want;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 1024;
    want.callback = NULL;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
    if (audio_device == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return -1;
    }
    SDL_PauseAudioDevice(audio_device, 0);

    SDL_Window* window = SDL_CreateWindow("emuNES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256 * 2, 240 * 2, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    const float FRAME_DURATION_MS = 1000.0f / 60.0f;

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        uint32_t frame_start = SDL_GetTicks();

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

        if (SDL_GetQueuedAudioSize(audio_device) > 4096 * sizeof(float)) {
            SDL_Delay(1);
            continue;
        }
        while (!bus.ppu.frame_complete) {
            bus.cpu.clock();
            
            bus.apu.clock();
            
            bus.ppu.clock();
            bus.ppu.clock();
            bus.ppu.clock();

            if (bus.apu.sample_ready) {
                bus.apu.sample_ready = false;
                float sample = bus.apu.get_output_sample();
                static int log_counter = 0;
                if (sample != 0.0f && log_counter < 10) {
                    printf("Audio Sample: %f\n", sample);
                    log_counter++;
                }
                SDL_QueueAudio(audio_device, &sample, sizeof(float));
            }
        }

        bus.ppu.frame_complete = false;

        SDL_UpdateTexture(texture, nullptr, bus.ppu.get_screen(), 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DURATION_MS) {
            SDL_Delay((uint32_t)(FRAME_DURATION_MS - frame_time));
        }
    }

    SDL_CloseAudioDevice(audio_device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
