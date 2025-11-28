#include "apu.h"
#include "bus.h"

static const uint8_t length_table[] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const uint16_t dmc_rate_table[] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};

static uint8_t pulse_sequence[4] = { 0b00000001, 0b00000011, 0b00001111, 0b11111100 };

static uint8_t triangle_sequence[32] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static const uint16_t noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

APU::APU() {}
APU::~APU() {}

void APU::clock_dmc() {
    if (dmc.enabled && dmc.sample_buffer_empty && dmc.bytes_remaining > 0) {
        dmc.sample_buffer = bus->cpu_read(dmc.current_address);
        dmc.sample_buffer_empty = false;
        
        if (dmc.current_address == 0xFFFF) dmc.current_address = 0x8000;
        else dmc.current_address++;
        
        dmc.bytes_remaining--;
        if (dmc.bytes_remaining == 0) {
            if (dmc.loop) {
                dmc.current_address = dmc.sample_address;
                dmc.bytes_remaining = dmc.sample_length;
            } else {
                if (dmc.irq_enable) { }
                dmc.enabled = false;
            }
        }
    }


    if (dmc.timer > 0) {
        dmc.timer--;
    } else {
        dmc.timer = dmc.timer_period;
        if (!dmc.silence) {
            if (dmc.shift_register & 0x01) {
                if (dmc.output_level <= 125) dmc.output_level += 2;
            } else {
                if (dmc.output_level >= 2) dmc.output_level -= 2;
            }
            dmc.shift_register >>= 1;
            dmc.bits_remaining--;
        }
        
        if (dmc.bits_remaining == 0) {
            dmc.bits_remaining = 8;
            if (dmc.sample_buffer_empty) {
                dmc.silence = true;
            } else {
                dmc.silence = false;
                dmc.shift_register = dmc.sample_buffer;
                dmc.sample_buffer_empty = true;
            }
        }
    }
}

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
            printf("P1 Length Loaded: %d (Index %d)\n", pulse1.length_value, (data & 0xF8) >> 3);
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
    case 0x4008:
        triangle.length_halt = (data & 0x80);
        triangle.linear_counter_reload = (data & 0x7F);
        break;
    case 0x400A:
        triangle.timer_period = (triangle.timer_period & 0xFF00) | data;
        break;
    case 0x400B:
        triangle.timer_period = (triangle.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
        if (triangle.enabled) triangle.length_value = length_table[(data & 0xF8) >> 3];
        triangle.linear_reload_flag = true;
        break;
    case 0x400C:
        noise.length_halt = (data & 0x20);
        noise.envelope_loop = (data & 0x20);
        noise.constant_volume = (data & 0x10);
        noise.constant_volume_val = (data & 0x0F);
        noise.envelope_period = (data & 0x0F);
        break;
    case 0x400E:
        noise.mode = (data & 0x80);
        noise.timer_period = noise_period_table[data & 0x0F];
        break;
    case 0x400F:
        if (noise.enabled) noise.length_value = length_table[(data & 0xF8) >> 3];
        noise.envelope_start = true;
        break;
    case 0x4010:
        dmc.irq_enable = (data & 0x80);
        dmc.loop = (data & 0x40);
        dmc.timer_period = dmc_rate_table[data & 0x0F];
        break;
    case 0x4011:
        dmc.output_level = (data & 0x7F);
        break;
    case 0x4012:
        dmc.sample_address = 0xC000 + (uint16_t)data * 64;
        break;
    case 0x4013:
        dmc.sample_length = (uint16_t)data * 16 + 1;
        break;
    case 0x4015:
        pulse1.enabled = (data & 0x01);
        pulse2.enabled = (data & 0x02);
        triangle.enabled = (data & 0x04);
        noise.enabled = (data & 0x08);
        dmc.enabled = (data & 0x10);
        if (!pulse1.enabled) pulse1.length_value = 0;
        if (!pulse2.enabled) pulse2.length_value = 0;
        if (!triangle.enabled) triangle.length_value = 0;
        if (!noise.enabled) noise.length_value = 0;
        if (!dmc.enabled) dmc.bytes_remaining = 0;
        else if (dmc.bytes_remaining == 0) {
            dmc.current_address = dmc.sample_address;
            dmc.bytes_remaining = dmc.sample_length;
        }
        break;
    case 0x4017:
        frame_counter_mode = (data & 0x80);
        irq_inhibit = (data & 0x40);
        
        if (irq_inhibit) frame_interrupt = false;
        frame_clock_counter = -1;

        if (frame_counter_mode) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
            
            clock_length(pulse1);
            clock_length(pulse2);
            clock_triangle_length();
            clock_noise_length();
        }
        break;
    }
}

uint8_t APU::cpu_read(uint16_t address) {
    uint8_t data = 0x00;
    if (address == 0x4015) {
        if (pulse1.length_value > 0) data |= 0x01;
        if (pulse2.length_value > 0) data |= 0x02;
        if (triangle.length_value > 0) data |= 0x04;
        if (noise.length_value > 0) data |= 0x08;
    }
    else {
        printf("Read $4015: P1 is inactive\n");
    }
    return data;
}

void APU::clock_pulse(PulseChannel& p) {
    if (p.timer > 0) { p.timer--; }
    else {
        p.timer = p.timer_period;
        p.sequence_pos++;
        p.sequence_pos &= 0x07;
    }
}

void APU::clock_triangle() {
    if (triangle.timer > 0) { triangle.timer--; }
    else {
        triangle.timer = triangle.timer_period;
        if (triangle.length_value > 0 && triangle.linear_counter > 0) {
            triangle.sequence_pos++;
            triangle.sequence_pos &= 0x1F;
        }
    }
}

void APU::clock_noise() {
    if (noise.timer > 0) { noise.timer--; }
    else {
        noise.timer = noise.timer_period;
        uint8_t feedback = (noise.shift_register & 0x01) ^
                           ((noise.shift_register >> (noise.mode ? 6 : 1)) & 0x01);
        noise.shift_register >>= 1;
        noise.shift_register |= (feedback << 14);
    }
}

void APU::clock_length(PulseChannel& p) {
    if (&p == &pulse1) { // Логируем только первый канал
         printf("Clock Length P1: Val=%d, Halt=%d\n", p.length_value, p.length_halt);
    }
    if (p.length_value > 0 && !p.length_halt) {
        p.length_value--;
    }
}

void APU::clock_triangle_length() {
    if (triangle.length_value > 0 && !triangle.length_halt) {
        triangle.length_value--;
    }
}

void APU::clock_noise_length() {
    if (noise.length_value > 0 && !noise.length_halt) {
        noise.length_value--;
    }
}

void APU::clock_linear_counter() {
    if (triangle.linear_reload_flag) {
        triangle.linear_counter = triangle.linear_counter_reload;
    } else if (triangle.linear_counter > 0) {
        triangle.linear_counter--;
    }
    if (!triangle.length_halt) {
        triangle.linear_reload_flag = false;
    }
}

void APU::clock_envelope(PulseChannel& p) {
    if (p.envelope_start) {
        p.envelope_start = false;
        p.envelope_counter = p.envelope_period + 1;
        p.volume_envelope = 15;
    } else {
        if (p.envelope_counter > 0) { p.envelope_counter--; }
        else {
            p.envelope_counter = p.envelope_period + 1;
            if (p.volume_envelope > 0) { p.volume_envelope--; }
            else if (p.envelope_loop) { p.volume_envelope = 15; }
        }
    }
}

void APU::clock_envelope_noise() {
    if (noise.envelope_start) {
        noise.envelope_start = false;
        noise.envelope_counter = noise.envelope_period + 1;
        noise.volume_envelope = 15;
    } else {
        if (noise.envelope_counter > 0) { noise.envelope_counter--; }
        else {
            noise.envelope_counter = noise.envelope_period + 1;
            if (noise.volume_envelope > 0) { noise.volume_envelope--; }
            else if (noise.envelope_loop) { noise.volume_envelope = 15; }
        }
    }
}

void APU::clock() {
    frame_clock_counter++;

    if (!frame_counter_mode) {
        if (frame_clock_counter == 7457) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
        }
        else if (frame_clock_counter == 14913) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
            
            clock_length(pulse1);
            clock_length(pulse2);
            clock_triangle_length();
            clock_noise_length();
        }
        else if (frame_clock_counter == 22371) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
        }
        else if (frame_clock_counter == 29829) {
            if (!irq_inhibit) frame_interrupt = true;
        }
        else if (frame_clock_counter == 29830) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
            
            clock_length(pulse1);
            clock_length(pulse2);
            clock_triangle_length();
            clock_noise_length();
            
            if (!irq_inhibit) frame_interrupt = true;
            frame_clock_counter = 0;
        }
    }
    else {
        if (frame_clock_counter == 7457) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
        }
        else if (frame_clock_counter == 14913) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
            
            clock_length(pulse1);
            clock_length(pulse2);
            clock_triangle_length();
            clock_noise_length();
        }
        else if (frame_clock_counter == 22371) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
        }
        else if (frame_clock_counter == 37281) {
            clock_envelope(pulse1);
            clock_envelope(pulse2);
            clock_envelope_noise();
            clock_linear_counter();
            
            clock_length(pulse1);
            clock_length(pulse2);
            clock_triangle_length();
            clock_noise_length();
            
            frame_clock_counter = 0;
        }
    }

    if (frame_clock_counter % 2 == 0) {
        clock_pulse(pulse1);
        clock_pulse(pulse2);
        clock_noise();
        clock_dmc();
    }
    
    clock_triangle();
    
    global_time += time_per_clock;
    if (global_time >= time_per_sample) {
        global_time -= time_per_sample;
        sample_ready = true;
        output_sample = get_output_sample();
    }
}

float APU::get_output_sample() {
    float p_out = 0.0f;
    auto get_pulse = [&](PulseChannel& p) {
        if (p.enabled && p.length_value > 0 && p.timer_period > 8) {
            if (pulse_sequence[p.duty_mode] & (1 << (7 - p.sequence_pos))) {
                return (float)(p.constant_volume ? p.constant_volume_val : p.volume_envelope) * 0.5f;
            }
        }
        return 0.0f;
    };
    p_out = 0.00752 * (get_pulse(pulse1) + get_pulse(pulse2));

    float t_out = 0.0f;
    if (triangle.enabled && triangle.length_value > 0 && triangle.linear_counter > 0) {
        t_out = 0.00851 * triangle_sequence[triangle.sequence_pos];
    }

    float n_out = 0.0f;
    if (noise.enabled && noise.length_value > 0 && !(noise.shift_register & 0x01)) {
        float vol = (float)(noise.constant_volume ? noise.constant_volume_val : noise.volume_envelope);
        n_out = 0.00494 * vol;
    }
    
    float d_out = 0.00335 * dmc.output_level;
    
    return (p_out + t_out + n_out + d_out) * 0.5f;
}
