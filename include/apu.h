#ifndef APU_H
#define APU_H
#include <cstdint>

class Bus;

class APU {
public:
    APU();
    ~APU();

    void connect_bus(Bus* b) { bus = b; }
    
    void cpu_write(uint16_t address, uint8_t data);
    uint8_t cpu_read(uint16_t address);
    
    void clock();
    float get_output_sample();

    bool sample_ready = false;
    float output_sample = 0.0f;

private:
    Bus* bus = nullptr;
    uint64_t frame_clock_counter = 0;
    
    double global_time = 0.0;
    const double time_per_clock = 1.0 / 1789773.0;
    const double time_per_sample = 1.0 / 44100.0;

    struct PulseChannel {
        bool enabled = false;
        
        uint16_t timer = 0;
        uint16_t timer_period = 0;
        uint8_t sequence_pos = 0;
        uint8_t duty_mode = 0;

        bool envelope_start = false;
        bool envelope_loop = false;
        bool constant_volume = false;
        uint8_t volume_envelope = 0;
        uint8_t envelope_period = 0;
        uint8_t envelope_counter = 0;
        uint8_t constant_volume_val = 0;
        
        uint8_t length_value = 0;
        bool length_halt = false;

        bool sweep_enable = false;
        bool sweep_negate = false;
        uint8_t sweep_period = 0;
        uint8_t sweep_shift = 0;
        uint8_t sweep_counter = 0;
        bool sweep_reload = false;
        uint16_t sweep_target_period = 0;
    } pulse1, pulse2;

    void clock_pulse(PulseChannel& p);
    void clock_length(PulseChannel& p);
    void clock_envelope(PulseChannel& p);
};

#endif //APU_H
