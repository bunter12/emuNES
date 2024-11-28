#include "../include/apu.h"

// Lookup tables
const uint8_t APU::length_table[32] = {
    10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

const uint8_t APU::duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // 75% (inverted 25%)
};

const uint16_t APU::noise_period_table[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

const uint16_t APU::dmc_rate_table[16] = {
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};


void APU::reset() {
    // Reset square channels
    square1 = Square();
    square2 = Square();
    
    // Reset triangle channel
    triangle = Triangle();
    
    // Reset noise channel
    noise = Noise();
    noise.shift_register = 1;
    
    // Reset DMC
    dmc = DMC();
    
    // Reset frame counter
    frame_counter = 0;
    frame_irq_enabled = false;
    frame_irq_flag = false;
}

void APU::write_register(uint16_t address, uint8_t data) {
    switch(address) {
        // Square 1
        case 0x4000:
            square1.duty = (data >> 6) & 0x03;
            square1.volume = data & 0x0F;
            square1.constant_volume = (data & 0x10) != 0;
            square1.envelope = data & 0x0F;
            break;
        case 0x4001:
            square1.sweep_enabled = (data & 0x80) != 0;
            square1.sweep_period = (data >> 4) & 7;
            square1.sweep_negate = (data & 0x08) != 0;
            square1.sweep_shift = data & 7;
            break;
        case 0x4002:
            square1.timer = (square1.timer & 0xFF00) | data;
            break;
        case 0x4003:
            square1.timer = (square1.timer & 0x00FF) | ((data & 7) << 8);
            square1.length_counter = length_table[data >> 3];
            square1.sequence_pos = 0;
            break;
            
        // Square 2 (аналогично Square 1)
        case 0x4004:
            square2.duty = (data >> 6) & 0x03;
            square2.volume = data & 0x0F;
            square2.constant_volume = (data & 0x10) != 0;
            square2.envelope = data & 0x0F;
            break;
            
        // Triangle
        case 0x4008:
            triangle.linear_counter = data & 0x7F;
            break;
        case 0x400A:
            triangle.timer = (triangle.timer & 0xFF00) | data;
            break;
        case 0x400B:
            triangle.timer = (triangle.timer & 0x00FF) | ((data & 7) << 8);
            triangle.length_counter = length_table[data >> 3];
            break;
            
        // Noise
        case 0x400C:
            noise.volume = data & 0x0F;
            noise.constant_volume = (data & 0x10) != 0;
            noise.envelope = data & 0x0F;
            break;
        case 0x400E:
            noise.mode = (data & 0x80) != 0;
            noise.timer = noise_period_table[data & 0x0F];
            break;
        case 0x400F:
            noise.length_counter = length_table[data >> 3];
            break;
            
        // Status
        case 0x4015:
            square1.enabled = (data & 0x01) != 0;
            square2.enabled = (data & 0x02) != 0;
            triangle.enabled = (data & 0x04) != 0;
            noise.enabled = (data & 0x08) != 0;
            dmc.enabled = (data & 0x10) != 0;
            break;
    }
}

uint8_t APU::read_register(uint16_t address) {
    if (address == 0x4015) {
        uint8_t status = 0;
        status |= (square1.length_counter > 0) ? 0x01 : 0;
        status |= (square2.length_counter > 0) ? 0x02 : 0;
        status |= (triangle.length_counter > 0) ? 0x04 : 0;
        status |= (noise.length_counter > 0) ? 0x08 : 0;
        status |= (dmc.bits_remaining > 0) ? 0x10 : 0;
        status |= frame_irq_flag ? 0x40 : 0;
        status |= dmc.irq_enabled ? 0x80 : 0;
        
        frame_irq_flag = false;
        return status;
    }
    return 0;
}

void APU::step() {
    // Update frame counter
    frame_counter++;
    if (frame_counter >= 14915) {
        frame_counter = 0;
        clock_envelope();
        clock_sweep();
        clock_length();
        clock_linear();
    }
    
    // Update channels
    update_square(square1, 0);
    update_square(square2, 1);
    update_triangle();
    update_noise();
    update_dmc();
}

float APU::get_audio_sample() {
    float square_out = 0.0f;
    if (square1.enabled) {
        square_out += duty_table[square1.duty][square1.sequence_pos] * 
                     (square1.constant_volume ? square1.volume : square1.envelope) / 15.0f;
    }
    if (square2.enabled) {
        square_out += duty_table[square2.duty][square2.sequence_pos] * 
                     (square2.constant_volume ? square2.volume : square2.envelope) / 15.0f;
    }
    square_out *= 0.5f;
    
    float triangle_out = 0.0f;
    if (triangle.enabled && triangle.length_counter > 0) {
        triangle_out = (triangle.sequence_pos < 16 ? triangle.sequence_pos : 31 - triangle.sequence_pos) / 15.0f;
    }
    
    float noise_out = 0.0f;
    if (noise.enabled && noise.length_counter > 0) {
        noise_out = ((noise.shift_register & 1) * 
                    (noise.constant_volume ? noise.volume : noise.envelope)) / 15.0f;
    }
    
    float dmc_out = dmc.output_level / 127.0f;
    
    // Mix all channels
    return 0.25f * (square_out + triangle_out + noise_out + dmc_out);
}

void APU::update_square(Square& square, uint8_t channel) {
    if (!square.enabled) return;
    
    if (square.timer > 0) {
        square.timer--;
    } else {
        square.timer = ((square.timer & 0x700) >> 8) | ((square.timer & 0xFF) << 1);
        square.sequence_pos = (square.sequence_pos + 1) & 7;
    }
}

void APU::update_triangle() {
    if (!triangle.enabled) return;
    
    if (triangle.timer > 0) {
        triangle.timer--;
    } else {
        triangle.timer = ((triangle.timer & 0x700) >> 8) | ((triangle.timer & 0xFF) << 1);
        if (triangle.length_counter > 0 && triangle.linear_counter > 0) {
            triangle.sequence_pos = (triangle.sequence_pos + 1) & 31;
        }
    }
}

void APU::update_noise() {
    if (!noise.enabled) return;
    
    if (noise.timer > 0) {
        noise.timer--;
    } else {
        noise.timer = noise_period_table[noise.mode ? 6 : 0];
        uint16_t feedback = ((noise.shift_register & 1) ^ 
                           ((noise.shift_register >> (noise.mode ? 6 : 1)) & 1)) << 14;
        noise.shift_register = (noise.shift_register >> 1) | feedback;
    }
}

void APU::update_dmc() {
    if (!dmc.enabled) return;
    
    if (dmc.timer > 0) {
        dmc.timer--;
        return;
    }
    
    dmc.timer = dmc_rate_table[0];
    
    if (dmc.bits_remaining == 0) {
        if (dmc.sample_length > 0) {
            dmc.bits_remaining = 8;
            dmc.sample_buffer = 0;
            dmc.sample_length--;
        } else if (dmc.loop_flag) {
            dmc.sample_length = (dmc.sample_address << 4) | 1;
            dmc.bits_remaining = 8;
            dmc.sample_buffer = 0;
        }
    }
    
    if (dmc.bits_remaining > 0) {
        if (dmc.sample_buffer & 1) {
            if (dmc.output_level <= 125) dmc.output_level += 2;
        } else {
            if (dmc.output_level >= 2) dmc.output_level -= 2;
        }
        dmc.sample_buffer >>= 1;
        dmc.bits_remaining--;
    }
}

void APU::clock_envelope() {
    if (!square1.constant_volume && square1.envelope > 0) {
        square1.envelope--;
    }
    if (!square2.constant_volume && square2.envelope > 0) {
        square2.envelope--;
    }
    if (!noise.constant_volume && noise.envelope > 0) {
        noise.envelope--;
    }
}

void APU::clock_sweep() {
    if (square1.sweep_enabled) {
        uint16_t delta = square1.timer >> square1.sweep_shift;
        if (square1.sweep_negate) {
            square1.sweep_target = square1.timer - delta;
            if (square1.sweep_target > 0x7FF) square1.enabled = false;
        } else {
            square1.sweep_target = square1.timer + delta;
        }
    }
    
    if (square2.sweep_enabled) {
        uint16_t delta = square2.timer >> square2.sweep_shift;
        if (square2.sweep_negate) {
            square2.sweep_target = square2.timer - delta;
            if (square2.sweep_target > 0x7FF) square2.enabled = false;
        } else {
            square2.sweep_target = square2.timer + delta;
        }
    }
}

void APU::clock_length() {
    if (square1.length_counter > 0) square1.length_counter--;
    if (square2.length_counter > 0) square2.length_counter--;
    if (triangle.length_counter > 0) triangle.length_counter--;
    if (noise.length_counter > 0) noise.length_counter--;
}

void APU::clock_linear() {
    if (triangle.linear_counter > 0) {
        triangle.linear_counter--;
    }
}