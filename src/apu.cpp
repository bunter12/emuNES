#include "apu.h"

static const uint8_t length_table[] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static uint8_t pulse_sequence[4] = {
    0b00000001,
    0b00000011,
    0b00001111,
    0b11111100
};

APU::APU() {}
APU::~APU() {}

void APU::cpu_write(uint16_t address, uint8_t data) {
    switch (address) {
    case 0x4000:
        pulse1.duty_mode = (data & 0xC0) >> 6;
        pulse1.length_halt = (data & 0x20);
        pulse1.envelope_loop = (data & 0x20);
        pulse1.constant_volume = (data & 0x10);
        pulse1.constant_volume_val = (data & 0x0F);
        pulse1.envelope_period = (data & 0x0F);
        break;
    case 0x4001:
        pulse1.sweep_enable = (data & 0x80);
        pulse1.sweep_period = (data & 0x70) >> 4;
        pulse1.sweep_negate = (data & 0x08);
        pulse1.sweep_shift = (data & 0x07);
        pulse1.sweep_reload = true;
        break;
    case 0x4002:
        pulse1.timer_period = (pulse1.timer_period & 0xFF00) | data;
        break;
    case 0x4003:
        pulse1.timer_period = (pulse1.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
        if (pulse1.enabled) {
            pulse1.length_value = length_table[(data & 0xF8) >> 3];
        }
        pulse1.sequence_pos = 0;
        pulse1.envelope_start = true;
        break;
    case 0x4004:
        pulse2.duty_mode = (data & 0xC0) >> 6;
        pulse2.length_halt = (data & 0x20);
        pulse2.envelope_loop = (data & 0x20);
        pulse2.constant_volume = (data & 0x10);
        pulse2.constant_volume_val = (data & 0x0F);
        pulse2.envelope_period = (data & 0x0F);
        break;
    case 0x4005:
        pulse2.sweep_enable = (data & 0x80);
        pulse2.sweep_period = (data & 0x70) >> 4;
        pulse2.sweep_negate = (data & 0x08);
        pulse2.sweep_shift = (data & 0x07);
        pulse2.sweep_reload = true;
        break;
    case 0x4006:
        pulse2.timer_period = (pulse2.timer_period & 0xFF00) | data;
        break;
    case 0x4007:
        pulse2.timer_period = (pulse2.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
        if (pulse2.enabled) {
            pulse2.length_value = length_table[(data & 0xF8) >> 3];
        }
        pulse2.sequence_pos = 0;
        pulse2.envelope_start = true;
        break;
    case 0x4015:
        pulse1.enabled = (data & 0x01);
        pulse2.enabled = (data & 0x02);
        if (!pulse1.enabled) pulse1.length_value = 0;
        if (!pulse2.enabled) pulse2.length_value = 0;
        break;
    }
}

uint8_t APU::cpu_read(uint16_t address) {
    return 0x00;
}

void APU::clock_pulse(PulseChannel& p) {
    if (p.timer > 0) {
        p.timer--;
    } else {
        p.timer = p.timer_period;
        p.sequence_pos++;
        p.sequence_pos &= 0x07;
    }
}

void APU::clock_length(PulseChannel& p) {
    if (p.length_value > 0 && !p.length_halt) {
        p.length_value--;
    }
}

void APU::clock_envelope(PulseChannel& p) {
    if (p.envelope_start) {
        p.envelope_start = false;
        p.envelope_counter = p.envelope_period + 1;
        p.volume_envelope = 15;
    } else {
        if (p.envelope_counter > 0) {
            p.envelope_counter--;
        } else {
            p.envelope_counter = p.envelope_period;
            if (p.volume_envelope > 0) {
                p.volume_envelope--;
            } else if (p.envelope_loop) {
                p.volume_envelope = 15;
            }
        }
    }
}

void APU::clock() {
    frame_clock_counter++;

    if (frame_clock_counter % 7457 == 0) {
        clock_envelope(pulse1);
        clock_envelope(pulse2);
    }
    
    if (frame_clock_counter % 14914 == 0) {
        clock_length(pulse1);
        clock_length(pulse2);
    }

    if (frame_clock_counter % 2 == 0) {
        clock_pulse(pulse1);
        clock_pulse(pulse2);
    }
    
    global_time += time_per_clock;
    if (global_time >= time_per_sample) {
        global_time -= time_per_sample;
        sample_ready = true;
        output_sample = get_output_sample();
    }
}

float APU::get_output_sample() {
    auto get_pulse_sample = [&](PulseChannel& p) -> float {
        if (p.enabled && p.length_value > 0 && p.timer_period > 8) {
            if (pulse_sequence[p.duty_mode] & (1 << (7 - p.sequence_pos))) {
                uint8_t vol = p.constant_volume ? p.constant_volume_val : p.volume_envelope;
                return (float)vol / 15.0f;
            }
        }
        return 0.0f;
    };

    float p1 = get_pulse_sample(pulse1);
    float p2 = get_pulse_sample(pulse2);
    
    return (p1 + p2) * 0.1f;
}
