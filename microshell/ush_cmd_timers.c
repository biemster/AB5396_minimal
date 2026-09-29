/*
MIT License

Copyright (c) 2021 Marcin Borowicz <marcinbor85@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "inc/ush.h"
#include "inc/ush_internal.h"
#include "sfr.h"

extern void timer_init(int timer_idx, uint32_t period_ticks);
extern void timer_stop(int timer_idx);
extern uint32_t timer_get_count(int timer_idx);
extern uint32_t systick;

/*
 * Command callback.
 */
void timers_callback(struct ush_object *self, struct ush_file_descriptor const *file, int argc, char *argv[]) {
	(void)argv;

	if (argc == 1) {
		// print
		ush_printf(self, "timer counts 0:%lu 1:%lu systick:%lu\r\n", timer_get_count(0), timer_get_count(1), systick);
	}
	else if (argc == 3 && strcmp(argv[1], "--init") == 0) {
		timer_init(atoi(argv[2]), 1000); // 1ms tick?

		// Enable global interrupts (RISC-V specific)
		// PICCON bit 0 enables interrupts
		PICCON |= BIT(0);
		PICCON |= 0x10000; // Enable CLIC mode (what's this?)

		ush_print(self, "--- done ---\r\n");
	}
	else if (argc == 3 && strcmp(argv[1], "--reset") == 0) {
		timer_stop(atoi(argv[2]));
		ush_printf(self, "--- timer%d stopped ---\r\n", atoi(argv[2]));
	}
	else {
		ush_print_status(
			self,
			USH_STATUS_ERROR_COMMAND_WRONG_ARGUMENTS
		);
		return;
	}
}
