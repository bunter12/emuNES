#include <iostream>
#include <string>
#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif
#include "bus.h"
#include "cartridge.h"

Bus bus;

int main(int argc, char* argv[]) {
    bool test_mode = false;
    std::string rom_path = "/Users/kiramsabirzanov/projects/emuNES/zelda.nes";
    if (argc > 1 && std::string(argv[1]) == "--test") {
        test_mode = true;
        if (argc > 2) {
            rom_path = argv[2];
        }
    } else if (argc > 1) {
        rom_path = argv[1];
    }

    Cartridge cart(rom_path);

    bus.insert_cartridge(&cart);
    bus.cpu.reset();
    bus.ppu.reset();
    bus.apu.reset();

    if (test_mode) {
        auto step_frame = [&]() {
            while (!bus.ppu.frame_complete) {
                bus.clock();
            }
            bus.ppu.frame_complete = false;
        };

        bool did_reset = false;
        const int max_frames = 2000;
        uint8_t status = 0x80;
        bool signature_seen = false;
        int signature_frame = -1;
        for (int frame = 0; frame < max_frames; ++frame) {
            step_frame();

            uint8_t sig1 = bus.cpu_read(0x6001);
            uint8_t sig2 = bus.cpu_read(0x6002);
            uint8_t sig3 = bus.cpu_read(0x6003);
            if (sig1 == 0xDE && sig2 == 0xB0 && sig3 == 0x61) {
                signature_seen = true;
                if (signature_frame < 0) {
                    signature_frame = frame;
                }
            }

            if (!signature_seen) {
                continue;
            }

            status = bus.cpu_read(0x6000);
            if (status == 0x81 && !did_reset && frame > 10) {
                bus.cpu.reset();
                did_reset = true;
            }
            if (status < 0x80) {
                break;
            }
        }

        uint8_t sig1 = bus.cpu_read(0x6001);
        uint8_t sig2 = bus.cpu_read(0x6002);
        uint8_t sig3 = bus.cpu_read(0x6003);
        std::cout << "[TEST] status=$" << std::hex << +status
                  << " signature=$" << +sig1 << " $" << +sig2 << " $" << +sig3
                  << std::dec << std::endl;
        std::cout << "[TEST] signature_seen=" << (signature_seen ? "yes" : "no")
                  << " first_frame=" << signature_frame << std::endl;

        std::string text;
        for (uint16_t i = 0; i < 1024; ++i) {
            uint8_t c = bus.cpu_read(static_cast<uint16_t>(0x6004 + i));
            if (c == 0x00) {
                break;
            }
            text.push_back(static_cast<char>(c));
        }
        if (!text.empty()) {
            std::cout << "[TEST] text:\n" << text << std::endl;
        }
        if (!signature_seen) {
            return 2;
        }
        return status == 0 ? 0 : 1;
    }

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

    SDL_Window* window = SDL_CreateWindow("emuNES", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 256 * 4, 240 * 4, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);

    const double TARGET_FPS = 60.0988138974405;
    const double FRAME_DURATION_SEC = 1.0 / TARGET_FPS;
    const double perf_freq = static_cast<double>(SDL_GetPerformanceFrequency());
    auto now_sec = [&]() -> double {
        return static_cast<double>(SDL_GetPerformanceCounter()) / perf_freq;
    };
    double next_frame_time = now_sec();

    bool quit = false;
    SDL_Event e;

    while (!quit) {
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

        double now = now_sec();
        if (now + 0.0005 < next_frame_time) {
            const double wait_ms = (next_frame_time - now - 0.0005) * 1000.0;
            if (wait_ms > 0.0) {
                SDL_Delay(static_cast<uint32_t>(wait_ms));
            }
            continue;
        }

        if (SDL_GetQueuedAudioSize(audio_device) > 4096 * sizeof(float)) {
            SDL_Delay(1);
            continue;
        }
        while (!bus.ppu.frame_complete) {
            bus.clock();

            if (bus.apu.sample_ready) {
                bus.apu.sample_ready = false;
                float sample = bus.apu.output_sample;
                SDL_QueueAudio(audio_device, &sample, sizeof(float));
            }
        }

        bus.ppu.frame_complete = false;

        SDL_UpdateTexture(texture, nullptr, bus.ppu.get_screen(), 256 * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        
        next_frame_time += FRAME_DURATION_SEC;
        now = now_sec();
        if (next_frame_time < now - (FRAME_DURATION_SEC * 2.0)) {
            next_frame_time = now;
        }
    }

    SDL_CloseAudioDevice(audio_device);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
