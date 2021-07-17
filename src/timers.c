#include "gb.h"
#include "internal.h"
#include "apu/apu.h"


static const uint16_t TAC_FREQ[4] = { 1024, 16, 64, 256 };

// todo: proper timer impl
// use: https://hacktixme.ga/GBEDG/timers/

static inline bool is_timer_enabled(const struct GB_Core* gb)
{
    return (IO_TAC & 0x04) > 0;
}

static inline bool check_div_clocks_fs(const struct GB_Core* gb, uint8_t old_div, uint8_t new_div)
{
    // either bit 4 or 5 falling edge is checked
    const uint8_t bit = 1 << (4 + gb->cpu.double_speed);

    return (old_div & bit) && !(new_div & bit);
}

static inline void div_clock_fs(struct GB_Core* gb, uint8_t old_div, uint8_t new_div)
{
    if (UNLIKELY(check_div_clocks_fs(gb, old_div, new_div)))
    {
        step_frame_sequencer(gb);
    }
}

void GB_div_write(struct GB_Core* gb, uint8_t value)
{
    // writes to DIV resets it to zero
    UNUSED(value);

    if (check_div_clocks_fs(gb, IO_DIV_UPPER, 0))
    {
        GB_log("div write caused early fs tick!\n");
    }

    GB_log("div write!\n");

    div_clock_fs(gb, IO_DIV_UPPER, 0);

    IO_DIV_UPPER = IO_DIV_LOWER = 0;
}

void GB_tima_write(struct GB_Core* gb, uint8_t value)
{
    IO_TIMA = value;
}

void GB_tma_write(struct GB_Core* gb, uint8_t value)
{
    IO_TMA = value;
}

void GB_tac_write(struct GB_Core* gb, uint8_t value)
{
    IO_TAC = value;
}

void GB_timer_run(struct GB_Core* gb, uint16_t cycles)
{    
    // DIV is a 16-bit register
    // reads return the upper byte, but the lower byte
    // gets ticked every 4-T cycles (1M)

    // check if we overflow
    if (UNLIKELY(IO_DIV_LOWER + cycles > 0xFF))
    {
        div_clock_fs(gb, IO_DIV_UPPER, IO_DIV_UPPER + 1);

        // increase the upper byte
        ++IO_DIV_UPPER;
    }

    // always add the cycles to lower byte
    // this will wrap round if overflow
    IO_DIV_LOWER = (IO_DIV_LOWER + cycles) & 0xFF;

    if (is_timer_enabled(gb))
    {
        gb->timer.next_cycles += cycles;

        while (UNLIKELY((gb->timer.next_cycles) >= TAC_FREQ[IO_TAC & 0x03]))
        {
            gb->timer.next_cycles -= TAC_FREQ[IO_TAC & 0x03];

            if (UNLIKELY(IO_TIMA == 0xFF))
            {
                IO_TIMA = IO_TMA;
                GB_enable_interrupt(gb, GB_INTERRUPT_TIMER);
            }
            else
            {
                ++IO_TIMA;
            }
        }
    }
}
