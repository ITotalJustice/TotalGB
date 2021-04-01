#include "core/gb.h"
#include "core/internal.h"


static const uint16_t TAC_FREQ[4] = { 1024, 16, 64, 256 };


void GB_timer_run(struct GB_Core* gb, uint16_t cycles) {
	// DIV is a 16-bit register
	// reads return the upper byte, but the lower byte
	// gets ticked every 4-T cycles (1M)

	// check if we overflow
	if (IO_DIV_LOWER + cycles > 0xFF) {
		// increase the upper byte
		++IO_DIV_UPPER;
	}

	// always add the cycles to lower byte
	// this will wrap round if overflow
	IO_DIV_LOWER += cycles;

	if (IO_TAC & 0x04) {
		gb->timer.next_cycles += cycles;

		while ((gb->timer.next_cycles) >= TAC_FREQ[IO_TAC & 0x03]) {
			gb->timer.next_cycles -= TAC_FREQ[IO_TAC & 0x03];

			if (IO_TIMA == 0xFF) {
				IO_TIMA = IO_TMA;
				GB_enable_interrupt(gb, GB_INTERRUPT_TIMER);
			} else {
				++IO_TIMA;
			}
		}
	}
}
