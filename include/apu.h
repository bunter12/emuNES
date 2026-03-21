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
    void reset();

    bool sample_ready = false;
    float output_sample = 0.0f;

private:
    Bus* bus = nullptr;
    uint64_t frame_clock_counter = 0;
    bool frame_counter_mode = false;
    bool irq_inhibit = false;
    bool frame_interrupt = false;
    bool even_cycle = false;

    double global_time = 0.0;
    const double time_per_clock = 1.0 / 1789773.0;
    const double time_per_sample = 1.0 / 44100.0;
    double hp_90_state = 0.0;
    double hp_440_state = 0.0;
    double lp_14000_state = 0.0;
    double hp_90_prev_input = 0.0;
    double hp_440_prev_input = 0.0;

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
        bool sweep_mute = false;
    } pulse1, pulse2;

    struct TriangleChannel {
        bool enabled = false;
        uint16_t timer = 0;
        uint16_t timer_period = 0;
        uint8_t sequence_pos = 0;
        uint8_t length_value = 0;
        bool length_halt = false;
        uint8_t linear_counter_reload = 0;
        uint8_t linear_counter = 0;
        bool linear_reload_flag = false;
    } triangle;

    struct NoiseChannel {
        bool enabled = false;
        uint16_t timer = 0;
        uint16_t timer_period = 0;
        uint16_t shift_register = 1;
        bool mode = false;
        bool envelope_start = false;
        bool envelope_loop = false;
        bool constant_volume = false;
        uint8_t volume_envelope = 0;
        uint8_t envelope_period = 0;
        uint8_t envelope_counter = 0;
        uint8_t constant_volume_val = 0;
        uint8_t length_value = 0;
        bool length_halt = false;
    } noise;

    struct DMCChannel {
        bool enabled = false;
        bool irq_enabled = false;
        bool irq_flag = false;
        bool loop = false;
        uint8_t rate_index = 0;
        uint16_t timer = 0;
        uint16_t timer_period = 428;
        uint8_t output_level = 0;
        uint8_t sample_buffer = 0;
        bool sample_buffer_empty = true;
        uint8_t shift_register = 0;
        uint8_t bits_remaining = 8;
        bool silence = true;
        uint16_t sample_address = 0xC000;
        uint16_t current_address = 0xC000;
        uint16_t sample_length = 1;
        uint16_t bytes_remaining = 0;
    } dmc;

    void clock_pulse(PulseChannel& p);
    void clock_sweep(PulseChannel& p, bool ones_complement);
    void clock_length(PulseChannel& p);
    void clock_envelope(PulseChannel& p);
    void clock_triangle();
    void clock_linear_counter();
    void clock_noise();
    void clock_envelope_noise();
    void clock_dmc();
    bool pulse_muted(const PulseChannel& p, bool ones_complement) const;
    void update_sweep_target(PulseChannel& p, bool ones_complement);
    void clock_quarter_frame();
    void clock_half_frame();
    void restart_dmc_sample();
    void fill_dmc_sample_buffer();
    float get_pulse_output(const PulseChannel& p, bool ones_complement) const;
    float get_triangle_output() const;
    float get_noise_output() const;
    float get_dmc_output() const;
    float apply_filter_chain(float sample);
    float mix_sample();
    
    void clock_triangle_length();
    void clock_noise_length();
};

#endif
