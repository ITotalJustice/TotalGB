#include "gb.h"
#include "internal.h"


static const uint16_t TAC_FREQ[4] = { 1024, 16, 64, 256 };

// todo: proper timer impl
// use: https://hacktixme.ga/GBEDG/timers/
#if 0
static uint16_t get_div_tac_bit(const struct GB_Core* gb)
{
    const uint8_t POS[4] = { 9, 3, 5, 7 };

    const uint16_t div = (IO_DIV_UPPER << 8) | IO_DIV_LOWER;

    return (div & (1 << POS[IO_TAC & 0x3])) > 0;
}
#endif

static inline bool is_timer_enabled(const struct GB_Core* gb)
{
    return (IO_TAC & 0x04) > 0;
}

void GB_div_write(struct GB_Core* gb, uint8_t value)
{
    // writes to DIV resets it to zero
    UNUSED(value);

    // todo: edge case...
    if (IO_DIV_UPPER == 1)
    {

    }

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
    if (IO_DIV_LOWER + cycles > 0xFF)
    {
        // increase the upper byte
        ++IO_DIV_UPPER;
    }

    // always add the cycles to lower byte
    // this will wrap round if overflow
    IO_DIV_LOWER += cycles;

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
