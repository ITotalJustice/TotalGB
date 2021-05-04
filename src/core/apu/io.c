#include "core/gb.h"
#include "core/internal.h"
#include "core/apu/apu.h"


// IO read / writes
uint8_t GB_apu_ioread(const struct GB_Core* gb, const uint16_t addr) {
    switch (addr & 0x7F) {
        case 0x10:
            return 0x80 | (IO_NR10.sweep_period << 4) | (IO_NR10.negate << 3) | IO_NR10.shift;

        case 0x11:
            return 0x3F | (IO_NR11.duty << 6);

        case 0x12:
            return (IO_NR12.starting_vol << 4) | (IO_NR12.env_add_mode << 3) | IO_NR12.period;

        case 0x14:
            return 0xBF | (IO_NR14.length_enable << 6);

        case 0x16:
            return 0x3F | (IO_NR21.duty << 6);

        case 0x17:
            return (IO_NR22.starting_vol << 4) | (IO_NR22.env_add_mode << 3) | IO_NR22.period;

        case 0x19:
            return 0xBF | (IO_NR24.length_enable << 6);

        case 0x1A:
            return 0x7F |  (IO_NR30.DAC_power << 7);

        case 0x1C:
            return 0x9F | (IO_NR32.vol_code << 5);

        case 0x1E:
            return 0xBF | (IO_NR34.length_enable << 6);

        case 0x21:
            return (IO_NR42.starting_vol << 4) | (IO_NR42.env_add_mode << 3) | IO_NR42.period;

        case 0x22:
            return (IO_NR43.clock_shift << 4) | (IO_NR43.width_mode << 3) | IO_NR43.divisor_code;

        case 0x23:
            return 0xBF | (IO_NR44.length_enable << 6);

        case 0x24:
            return (IO_NR50.vin_l << 7) | (IO_NR50.left_vol << 4) | (IO_NR50.vin_r << 3) | IO_NR50.right_vol;

        case 0x25:
            return (IO_NR51.noise_left << 7) | (IO_NR51.wave_left << 6) | (IO_NR51.square1_left << 5) | (IO_NR51.square2_left << 4) | (IO_NR51.noise_right << 3) | (IO_NR51.wave_right << 2) | (IO_NR51.square1_right << 1) | IO_NR51.square2_right;

        case 0x26:
            return 0x70 | (IO_NR52.power << 7) | (IO_NR52.noise << 3) | (IO_NR52.wave << 2) | (IO_NR52.square2 << 1) | (IO_NR52.square1);

        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            if (!is_wave_enabled(gb)) {
                return IO_WAVE_TABLE[addr & 0xF];
            }
            return 0xFF;

        default:
            return 0xFF;
    }
}

void GB_apu_iowrite(struct GB_Core* gb, const uint16_t addr, const uint8_t value) {
    switch (addr & 0x7F) {
        case 0x10:
            IO_NR10.sweep_period = (value >> 4) & 0x7;
            IO_NR10.negate = (value >> 3) & 0x1;
            IO_NR10.shift = value & 0x7;
            break;

        case 0x11:
            IO_NR11.duty = value >> 6;
            IO_NR11.length_load = value & 0x3F;
            SQUARE1_CHANNEL.length_counter = 64 - IO_NR11.length_load;
            break;

        case 0x12:
            IO_NR12.starting_vol = value >> 4;
            IO_NR12.env_add_mode = (value >> 3) & 0x1;
            IO_NR12.period = value & 0x7;
            if (is_square1_dac_enabled(gb) == false) {
                square1_disable(gb);
            }
            break;

        case 0x13:
            IO_NR13.freq_lsb = value;
            break;

        case 0x14:
            IO_NR14.trigger = (value >> 7) & 0x1;
            IO_NR14.length_enable = (value >> 6) & 0x1;
            IO_NR14.freq_msb = value & 0x7;

            if (IO_NR14.trigger) {
                on_square1_trigger(gb);
            }
            break;

        case 0x16:
            IO_NR21.duty = value >> 6;
            IO_NR21.length_load = value & 0x3F;
            SQUARE2_CHANNEL.length_counter = 64 - IO_NR21.length_load;
            break;

        case 0x17:
            IO_NR22.starting_vol = value >> 4;
            IO_NR22.env_add_mode = (value >> 3) & 0x1;
            IO_NR22.period = value & 0x7;
            if (is_square2_dac_enabled(gb) == false) {
                square2_disable(gb);
            }
            break;

        case 0x18:
            IO_NR23.freq_lsb = value;
            break;

        case 0x19:
            IO_NR24.trigger = (value >> 7) & 0x1;
            IO_NR24.length_enable = (value >> 6) & 0x1;
            IO_NR24.freq_msb = value & 0x7;

            if (IO_NR24.trigger) {
                on_square2_trigger(gb);
            }
            break;

        case 0x1A:
            IO_NR30.DAC_power = value >> 7;
            if (is_wave_dac_enabled(gb) == false) {
                wave_disable(gb);
            }
            break;

        case 0x1B:
            IO_NR31.length_load = value;
            WAVE_CHANNEL.length_counter = 256 - IO_NR31.length_load;
            break;

        case 0x1C:
            IO_NR32.vol_code = (value >> 5) & 0x3;
            break;

        case 0x1D:
            IO_NR33.freq_lsb = value;
            break;

        case 0x1E:
            IO_NR34.trigger = (value >> 7) & 0x1;
            IO_NR34.length_enable = (value >> 6) & 0x1;
            IO_NR34.freq_msb = value & 0x7;

            if (IO_NR34.trigger) {
                on_wave_trigger(gb);
            }
            break;

        case 0x20:
            IO_NR41.length_load = value & 0x3F;
            NOISE_CHANNEL.length_counter = 64 - IO_NR41.length_load;
            break;

        case 0x21:
            IO_NR42.starting_vol = value >> 4;
            IO_NR42.env_add_mode = (value >> 3) & 0x1;
            IO_NR42.period = value & 0x7;
            if (is_noise_dac_enabled(gb) == false) {
                noise_disable(gb);
            }
            break;

        case 0x22:
            IO_NR43.clock_shift = value >> 4;
            IO_NR43.width_mode = (value >> 3) & 0x1;
            IO_NR43.divisor_code = value & 0x7;
            break;

        case 0x23:
            IO_NR44.trigger = (value >> 7) & 0x1;
            IO_NR44.length_enable = (value >> 6) & 0x1;

            if (IO_NR44.trigger) {
                on_noise_trigger(gb);
            }
            break;

        case 0x24:
            IO_NR50.vin_l = value >> 7;
            IO_NR50.left_vol = (value >> 4) & 0x7;
            IO_NR50.vin_r = (value >> 3) & 0x1;
            IO_NR50.right_vol = value & 0x7;
            break;

        case 0x25:
            IO_NR51.noise_left = value >> 7;
            IO_NR51.wave_left = (value >> 6) & 0x1;
            IO_NR51.square2_left = (value >> 5) & 0x1;
            IO_NR51.square1_left = (value >> 4) & 0x1;
            IO_NR51.noise_right = (value >> 3) & 0x1;
            IO_NR51.wave_right = (value >> 2) & 0x1;
            IO_NR51.square2_right = (value >> 1) & 0x1;
            IO_NR51.square1_right = value & 0x1;
            break;

        case 0x26:
            IO_NR52.power = value >> 7;
            break;

        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            // wave ram should only be accessed whilst the channel
            // is disabled.
            // otherwise, for the first few clocks after the last
            // time the wave channel accessed this wave ram, it will read
            // and write to the current index.
            // otherwise, writes are ignored and reads return 0xFF.

            // for now, all reads / writes that happen whilst the channel is
            // disabled only!

            if (!is_wave_enabled(gb)) {
                IO_WAVE_TABLE[addr & 0xF] = value;
            }
            break;
    }
}
