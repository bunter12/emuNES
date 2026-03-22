#include "apu.h"

#include "bus.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

static const uint8_t length_table[] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

static const uint8_t pulse_duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1},
};

static const uint8_t triangle_sequence[32] = {
    15, 14, 13, 12, 11, 10, 9, 8,
    7, 6, 5, 4, 3, 2, 1, 0,
    0, 1, 2, 3, 4, 5, 6, 7,
    8, 9, 10, 11, 12, 13, 14, 15
};

static const uint16_t noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};

static const uint16_t dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106, 84, 72, 54
};

static constexpr double kPi = 3.14159265358979323846;

APU::APU() {
    reset();
}

APU::~APU() {}

void APU::reset() {
    frame_clock_counter = 0;
    frame_counter_mode = false;
    irq_inhibit = false;
    frame_interrupt = false;
    even_cycle = false;

    global_time = 0.0;
    sample_ready = false;
    output_sample = 0.0f;

    hp_90_state = 0.0;
    hp_440_state = 0.0;
    lp_14000_state = 0.0;
    hp_90_prev_input = 0.0;
    hp_440_prev_input = 0.0;

    pulse1 = PulseChannel{};
    pulse2 = PulseChannel{};
    triangle = TriangleChannel{};
    noise = NoiseChannel{};
    dmc = DMCChannel{};

    noise.shift_register = 1;
    dmc.timer_period = dmc_rate_table[0];
    dmc.sample_address = 0xC000;
    dmc.current_address = 0xC000;
    dmc.sample_length = 1;

    if (bus) {
        bus->set_irq_line(false);
    }
}

void APU::cpu_write(uint16_t address, uint8_t data) {
    switch (address) {
    case 0x4000:
        pulse1.duty_mode = (data >> 6) & 0x03;
        pulse1.length_halt = (data & 0x20) != 0;
        pulse1.envelope_loop = (data & 0x20) != 0;
        pulse1.constant_volume = (data & 0x10) != 0;
        pulse1.constant_volume_val = data & 0x0F;
        pulse1.envelope_period = data & 0x0F;
        break;
    case 0x4001:
        pulse1.sweep_enable = (data & 0x80) != 0;
        pulse1.sweep_period = (data >> 4) & 0x07;
        pulse1.sweep_negate = (data & 0x08) != 0;
        pulse1.sweep_shift = data & 0x07;
        pulse1.sweep_reload = true;
        update_sweep_target(pulse1, true);
        break;
    case 0x4002:
        pulse1.timer_period = (pulse1.timer_period & 0x0700) | data;
        update_sweep_target(pulse1, true);
        break;
    case 0x4003:
        pulse1.timer_period = (pulse1.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
        pulse1.length_value = pulse1.enabled ? length_table[(data >> 3) & 0x1F] : 0;
        pulse1.sequence_pos = 0;
        pulse1.envelope_start = true;
        pulse1.timer = pulse1.timer_period;
        update_sweep_target(pulse1, true);
        break;
    case 0x4004:
        pulse2.duty_mode = (data >> 6) & 0x03;
        pulse2.length_halt = (data & 0x20) != 0;
        pulse2.envelope_loop = (data & 0x20) != 0;
        pulse2.constant_volume = (data & 0x10) != 0;
        pulse2.constant_volume_val = data & 0x0F;
        pulse2.envelope_period = data & 0x0F;
        break;
    case 0x4005:
        pulse2.sweep_enable = (data & 0x80) != 0;
        pulse2.sweep_period = (data >> 4) & 0x07;
        pulse2.sweep_negate = (data & 0x08) != 0;
        pulse2.sweep_shift = data & 0x07;
        pulse2.sweep_reload = true;
        update_sweep_target(pulse2, false);
        break;
    case 0x4006:
        pulse2.timer_period = (pulse2.timer_period & 0x0700) | data;
        update_sweep_target(pulse2, false);
        break;
    case 0x4007:
        pulse2.timer_period = (pulse2.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
        pulse2.length_value = pulse2.enabled ? length_table[(data >> 3) & 0x1F] : 0;
        pulse2.sequence_pos = 0;
        pulse2.envelope_start = true;
        pulse2.timer = pulse2.timer_period;
        update_sweep_target(pulse2, false);
        break;
    case 0x4008:
        triangle.length_halt = (data & 0x80) != 0;
        triangle.linear_counter_reload = data & 0x7F;
        break;
    case 0x400A:
        triangle.timer_period = (triangle.timer_period & 0x0700) | data;
        break;
    case 0x400B:
        triangle.timer_period = (triangle.timer_period & 0x00FF) | ((uint16_t)(data & 0x07) << 8);
        triangle.length_value = triangle.enabled ? length_table[(data >> 3) & 0x1F] : 0;
        triangle.linear_reload_flag = true;
        triangle.timer = triangle.timer_period;
        break;
    case 0x400C:
        noise.length_halt = (data & 0x20) != 0;
        noise.envelope_loop = (data & 0x20) != 0;
        noise.constant_volume = (data & 0x10) != 0;
        noise.constant_volume_val = data & 0x0F;
        noise.envelope_period = data & 0x0F;
        break;
    case 0x400E:
        noise.mode = (data & 0x80) != 0;
        noise.timer_period = noise_period_table[data & 0x0F];
        break;
    case 0x400F:
        noise.length_value = noise.enabled ? length_table[(data >> 3) & 0x1F] : 0;
        noise.envelope_start = true;
        break;
    case 0x4010:
        dmc.irq_enabled = (data & 0x80) != 0;
        dmc.loop = (data & 0x40) != 0;
        dmc.rate_index = data & 0x0F;
        dmc.timer_period = dmc_rate_table[dmc.rate_index];
        if (!dmc.irq_enabled) {
            dmc.irq_flag = false;
            if (bus) {
                bus->set_irq_line(frame_interrupt || dmc.irq_flag);
            }
        }
        break;
    case 0x4011:
        dmc.output_level = data & 0x7F;
        break;
    case 0x4012:
        dmc.sample_address = 0xC000 | ((uint16_t)data << 6);
        break;
    case 0x4013:
        dmc.sample_length = ((uint16_t)data << 4) | 0x0001;
        break;
    case 0x4015: {
        pulse1.enabled = (data & 0x01) != 0;
        pulse2.enabled = (data & 0x02) != 0;
        triangle.enabled = (data & 0x04) != 0;
        noise.enabled = (data & 0x08) != 0;
        bool dmc_enable = (data & 0x10) != 0;

        if (!pulse1.enabled) pulse1.length_value = 0;
        if (!pulse2.enabled) pulse2.length_value = 0;
        if (!triangle.enabled) triangle.length_value = 0;
        if (!noise.enabled) noise.length_value = 0;

        dmc.irq_flag = false;
        dmc.enabled = dmc_enable;
        if (bus) {
            bus->set_irq_line(frame_interrupt || dmc.irq_flag);
        }
        if (!dmc.enabled) {
            dmc.bytes_remaining = 0;
        } else if (dmc.bytes_remaining == 0) {
            restart_dmc_sample();
        }
        break;
    }
    case 0x4017:
        frame_counter_mode = (data & 0x80) != 0;
        irq_inhibit = (data & 0x40) != 0;
        if (irq_inhibit) {
            frame_interrupt = false;
            if (bus) {
                bus->set_irq_line(frame_interrupt || dmc.irq_flag);
            }
        }
        frame_clock_counter = 0;
        if (frame_counter_mode) {
            clock_quarter_frame();
            clock_half_frame();
        }
        break;
    default:
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
        if (dmc.bytes_remaining > 0) data |= 0x10;
        if (frame_interrupt) data |= 0x40;
        if (dmc.irq_flag) data |= 0x80;
        frame_interrupt = false;
        if (bus) {
            bus->set_irq_line(frame_interrupt || dmc.irq_flag);
        }
    }
    return data;
}

void APU::update_sweep_target(PulseChannel& p, bool ones_complement) {
    uint16_t change = p.timer_period >> p.sweep_shift;
    p.sweep_target_period = p.sweep_negate
        ? static_cast<uint16_t>(p.timer_period - change - (ones_complement ? 1 : 0))
        : static_cast<uint16_t>(p.timer_period + change);
    p.sweep_mute = (p.timer_period < 8) || (p.sweep_target_period > 0x07FF);
}

bool APU::pulse_muted(const PulseChannel& p, bool ones_complement) const {
    if (!p.enabled || p.length_value == 0 || p.timer_period < 8) {
        return true;
    }

    if (p.sweep_enable && p.sweep_shift > 0) {
        uint16_t change = p.timer_period >> p.sweep_shift;
        uint16_t target = p.sweep_negate
            ? static_cast<uint16_t>(p.timer_period - change - (ones_complement ? 1 : 0))
            : static_cast<uint16_t>(p.timer_period + change);
        return target > 0x07FF;
    }

    return false;
}

void APU::clock_pulse(PulseChannel& p) {
    if (p.timer == 0) {
        p.timer = p.timer_period;
        p.sequence_pos = (p.sequence_pos + 1) & 0x07;
    } else {
        p.timer--;
    }
}

void APU::clock_sweep(PulseChannel& p, bool ones_complement) {
    update_sweep_target(p, ones_complement);

    if (p.sweep_counter == 0) {
        if (p.sweep_enable && p.sweep_shift > 0 && !p.sweep_mute) {
            p.timer_period = p.sweep_target_period;
            update_sweep_target(p, ones_complement);
        }
        p.sweep_counter = p.sweep_period;
    } else {
        p.sweep_counter--;
    }

    if (p.sweep_reload) {
        p.sweep_reload = false;
        p.sweep_counter = p.sweep_period;
    }
}

void APU::clock_length(PulseChannel& p) {
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
        p.envelope_counter = p.envelope_period;
        p.volume_envelope = 15;
        return;
    }

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

void APU::clock_noise() {
    if (noise.timer == 0) {
        noise.timer = noise.timer_period;
        uint8_t bit0 = noise.shift_register & 0x01;
        uint8_t tap = noise.mode ? ((noise.shift_register >> 6) & 0x01)
                                 : ((noise.shift_register >> 1) & 0x01);
        uint8_t feedback = bit0 ^ tap;
        noise.shift_register >>= 1;
        noise.shift_register |= ((uint16_t)feedback << 14);
    } else {
        noise.timer--;
    }
}

void APU::clock_envelope_noise() {
    if (noise.envelope_start) {
        noise.envelope_start = false;
        noise.envelope_counter = noise.envelope_period;
        noise.volume_envelope = 15;
        return;
    }

    if (noise.envelope_counter > 0) {
        noise.envelope_counter--;
    } else {
        noise.envelope_counter = noise.envelope_period;
        if (noise.volume_envelope > 0) {
            noise.volume_envelope--;
        } else if (noise.envelope_loop) {
            noise.volume_envelope = 15;
        }
    }
}

void APU::restart_dmc_sample() {
    dmc.current_address = dmc.sample_address;
    dmc.bytes_remaining = dmc.sample_length;
}

void APU::fill_dmc_sample_buffer() {
    if (!dmc.enabled || !dmc.sample_buffer_empty || dmc.bytes_remaining == 0 || bus == nullptr) {
        return;
    }

    dmc.sample_buffer = bus->cpu_read(dmc.current_address);
    dmc.sample_buffer_empty = false;

    dmc.current_address++;
    if (dmc.current_address == 0x0000) {
        dmc.current_address = 0x8000;
    }

    dmc.bytes_remaining--;
    if (dmc.bytes_remaining == 0) {
        if (dmc.loop) {
            restart_dmc_sample();
        } else if (dmc.irq_enabled) {
            dmc.irq_flag = true;
            if (bus) {
                bus->set_irq_line(frame_interrupt || dmc.irq_flag);
            }
        }
    }
}

void APU::clock_dmc() {
    fill_dmc_sample_buffer();

    if (dmc.timer == 0) {
        dmc.timer = dmc.timer_period;

        if (!dmc.silence) {
            if (dmc.shift_register & 0x01) {
                if (dmc.output_level <= 125) {
                    dmc.output_level += 2;
                }
            } else if (dmc.output_level >= 2) {
                dmc.output_level -= 2;
            }
        }

        dmc.shift_register >>= 1;
        if (dmc.bits_remaining > 0) {
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
    } else {
        dmc.timer--;
    }
}

void APU::clock_triangle() {
    if (triangle.timer == 0) {
        triangle.timer = triangle.timer_period;
        if (triangle.length_value > 0 && triangle.linear_counter > 0 && triangle.timer_period > 1) {
            triangle.sequence_pos = (triangle.sequence_pos + 1) & 0x1F;
        }
    } else {
        triangle.timer--;
    }
}

void APU::clock_quarter_frame() {
    clock_envelope(pulse1);
    clock_envelope(pulse2);
    clock_envelope_noise();
    clock_linear_counter();
}

void APU::clock_half_frame() {
    clock_length(pulse1);
    clock_length(pulse2);
    clock_triangle_length();
    clock_noise_length();
    clock_sweep(pulse1, true);
    clock_sweep(pulse2, false);
}

void APU::clock() {
    frame_clock_counter++;

    if (!frame_counter_mode) {
        if (frame_clock_counter == 7457 || frame_clock_counter == 22371) {
            clock_quarter_frame();
        } else if (frame_clock_counter == 14913) {
            clock_quarter_frame();
            clock_half_frame();
        } else if (frame_clock_counter == 29829 || frame_clock_counter == 29830) {
            if (!irq_inhibit) {
                frame_interrupt = true;
                if (bus) {
                    bus->set_irq_line(frame_interrupt || dmc.irq_flag);
                }
            }
        } else if (frame_clock_counter == 29831) {
            clock_quarter_frame();
            clock_half_frame();
            frame_clock_counter = 0;
        }
    } else {
        if (frame_clock_counter == 7457 || frame_clock_counter == 22371) {
            clock_quarter_frame();
        } else if (frame_clock_counter == 14913 || frame_clock_counter == 37281) {
            clock_quarter_frame();
            clock_half_frame();
        } else if (frame_clock_counter == 37282) {
            frame_clock_counter = 0;
        }
    }

    even_cycle = !even_cycle;
    if (even_cycle) {
        clock_pulse(pulse1);
        clock_pulse(pulse2);
        clock_noise();
    }

    clock_triangle();
    clock_dmc();

    global_time += time_per_clock;
    while (global_time >= time_per_sample) {
        global_time -= time_per_sample;
        sample_ready = true;
        output_sample = mix_sample();
    }
}

float APU::get_pulse_output(const PulseChannel& p, bool ones_complement) const {
    if (pulse_muted(p, ones_complement) || pulse_duty_table[p.duty_mode][p.sequence_pos] == 0) {
        return 0.0f;
    }

    return static_cast<float>(p.constant_volume ? p.constant_volume_val : p.volume_envelope);
}

float APU::get_triangle_output() const {
    if (!triangle.enabled || triangle.length_value == 0 || triangle.linear_counter == 0 || triangle.timer_period < 2) {
        return 0.0f;
    }

    return static_cast<float>(triangle_sequence[triangle.sequence_pos]);
}

float APU::get_noise_output() const {
    if (!noise.enabled || noise.length_value == 0 || (noise.shift_register & 0x01)) {
        return 0.0f;
    }

    return static_cast<float>(noise.constant_volume ? noise.constant_volume_val : noise.volume_envelope);
}

float APU::get_dmc_output() const {
    return static_cast<float>(dmc.output_level);
}

float APU::apply_filter_chain(float sample) {
    const double dt = time_per_sample;

    const double hp_90_rc = 1.0 / (2.0 * kPi * 90.0);
    const double hp_90_coeff = hp_90_rc / (hp_90_rc + dt);
    hp_90_state = hp_90_coeff * (hp_90_state + sample - hp_90_prev_input);
    hp_90_prev_input = sample;

    const double hp_440_rc = 1.0 / (2.0 * kPi * 440.0);
    const double hp_440_coeff = hp_440_rc / (hp_440_rc + dt);
    hp_440_state = hp_440_coeff * (hp_440_state + hp_90_state - hp_440_prev_input);
    hp_440_prev_input = hp_90_state;

    const double lp_14000_coeff = dt / ((1.0 / (2.0 * kPi * 14000.0)) + dt);
    lp_14000_state += lp_14000_coeff * (hp_440_state - lp_14000_state);

    return static_cast<float>(lp_14000_state);
}

float APU::mix_sample() {
    float pulse1_out = get_pulse_output(pulse1, true);
    float pulse2_out = get_pulse_output(pulse2, false);
    float triangle_out = get_triangle_output();
    float noise_out = get_noise_output();
    float dmc_out = get_dmc_output();

    float pulse_sum = pulse1_out + pulse2_out;
    float pulse_mix = 0.0f;
    if (pulse_sum > 0.0f) {
        pulse_mix = 95.88f / ((8128.0f / pulse_sum) + 100.0f);
    }

    float tnd_sum = triangle_out / 8227.0f + noise_out / 12241.0f + dmc_out / 22638.0f;
    float tnd_mix = 0.0f;
    if (tnd_sum > 0.0f) {
        tnd_mix = 159.79f / ((1.0f / tnd_sum) + 100.0f);
    }

    return apply_filter_chain(pulse_mix + tnd_mix);
}

float APU::get_output_sample() {
    return output_sample;
}
