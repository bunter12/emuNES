#ifndef APU_H
#define APU_H
#include <cstdint>
#include <array>

class APU {
public:
    
    // Core functions
    void step();
    void reset();
    void write_register(uint16_t address, uint8_t data);
    uint8_t read_register(uint16_t address);
    
    // Audio output
    float get_audio_sample();
    
private:
    // Square channels (Pulse waves)
    struct Square {
        bool enabled;
        uint8_t duty;          // Duty cycle
        uint8_t volume;        // Volume/Envelope
        bool constant_volume;  // Constant volume flag
        uint8_t envelope;      // Envelope divider
        uint16_t timer;        // Timer value
        uint8_t length_counter;// Length counter
        uint8_t sequence_pos;  // Current position in sequence
        
        // Sweep unit
        bool sweep_enabled;
        uint8_t sweep_period;
        bool sweep_negate;
        uint8_t sweep_shift;
        uint16_t sweep_target;
    };
    Square square1, square2;
    
    // Triangle channel
    struct Triangle {
        bool enabled;
        uint8_t linear_counter;
        uint8_t length_counter;
        uint16_t timer;
        uint8_t sequence_pos;
    } triangle;
    
    // Noise channel
    struct Noise {
        bool enabled;
        uint8_t volume;
        bool constant_volume;
        uint8_t envelope;
        uint16_t timer;
        uint8_t length_counter;
        uint16_t shift_register;
        bool mode;
    } noise;
    
    // DMC (Delta Modulation Channel)
    struct DMC {
        bool enabled;
        uint16_t sample_address;
        uint16_t sample_length;
        uint8_t current_sample;
        uint8_t sample_buffer;
        bool sample_buffer_empty;
        uint8_t shift_register;
        uint8_t bits_remaining;
        uint8_t output_level;
        uint16_t timer;
        bool irq_enabled;
        bool loop_flag;
    } dmc;
    
    // Frame counter
    uint8_t frame_counter;
    bool frame_irq_enabled;
    bool frame_irq_flag;
    
    // Internal helper functions
    void clock_envelope();
    void clock_sweep();
    void clock_length();
    void clock_linear();
    
    void update_square(Square& square, uint8_t channel);
    void update_triangle();
    void update_noise();
    void update_dmc();
    
    // Lookup tables
    static const uint8_t length_table[32];
    static const uint8_t duty_table[4][8];
    static const uint16_t noise_period_table[16];
    static const uint16_t dmc_rate_table[16];
};

#endif //APU_H
