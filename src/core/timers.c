#include "gb.h"
#include "internal.h"

// todo: IO_DIV is a 16 bit value
void GB_timer_run(struct GB_Data* gb, GB_U16 cycles) {
	IO_DIV += cycles;

	if (IO_TAC & 0x04) {
		static const GB_U16 TAC_FREQ[4] = { 1024, 16, 64, 256 };
		gb->timer.next_cycles += cycles;

		while ((gb->timer.next_cycles) >= TAC_FREQ[IO_TAC & 0x03]) {
			gb->timer.next_cycles -= TAC_FREQ[IO_TAC & 0x03];

			if (IO_TIMA == 0xFF) {
				IO_TIMA = IO_TMA;
				IO_IF |= 4;
			} else {
				++IO_TIMA;
			}
		}
	}
}
