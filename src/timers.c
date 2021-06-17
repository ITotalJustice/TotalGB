#include "gb.h"
#include "internal.h"


static const uint16_t TAC_FREQ[4] = { 1024, 16, 64, 256 };


static inline bool is_timer_enabled(const uint8_t tac)
{
    return (tac & 0x04) > 0;
}

static inline uint16_t get_timer_freq(const struct GB_Core* gb)
{
    return TAC_FREQ[IO_TAC & 0x03];
}

void GB_div_write(struct GB_Core* gb, uint8_t value)
{
    // writes to DIV resets it to zero
    UNUSED(value);

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
    // check if we are go
    if (is_timer_enabled(IO_TAC) && !is_timer_enabled(value))
    {
        GB_pop_event(gb, GB_EventType_TIMER);
    }
    else if (!is_timer_enabled(IO_TAC) && is_timer_enabled(value))
    {
        GB_add_event(gb, GB_EventType_TIMER, get_timer_freq(gb));
    }

    IO_TAC = value;
}

void GB_timer_on_event(struct GB_Core* gb)
{
    if (UNLIKELY(IO_TIMA == 0xFF))
    {
        IO_TIMA = IO_TMA;
        GB_enable_interrupt(gb, GB_INTERRUPT_TIMER);
    }
    else
    {
        ++IO_TIMA;
    }

    GB_add_event(gb, GB_EventType_TIMER, get_timer_freq(gb));
}

void GB_timer_run(struct GB_Core* gb, uint16_t cycles)
{    
    // DIV is a 16-bit register
    // reads return the upper byte, but the lower byte
    // gets ticked every 4-T cycles (1M)

    // always add the cycles to lower byte
    // this will wrap round if overflow
    IO_DIV_UPPER += (IO_DIV_LOWER + cycles) > 0xFF;
    IO_DIV_LOWER += cycles;
}

void GB_timer_init(struct GB_Core* gb)
{
    GB_add_event(gb, GB_EventType_TIMER, get_timer_freq(gb));
}
