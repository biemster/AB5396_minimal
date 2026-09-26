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

// stuff from iSLER.h that is already included in main.c
extern void rf_init(void);
extern int rf_reg_rd(uint8_t reg_addr, uint32_t *value);
extern void bb_clk_init(void);
extern void ble_baseband_init(void);
extern void ble_send_adv_dtm(uint8_t phys_channel);
extern void hunt_radio_interrupt(struct ush_object *self);
#define BB_CLK 0xf020


typedef enum {
	RADIO_DUMPREGS,
	RADIO_BB_CLOCK,
	RADIO_ADVERTISE
} ush_radio_ctrl_cmds_t;

/*
 * Command callback.
 * Do not dump all registers here. Start the asynchronous MicroShell
 * processing state machine instead.
 */
void radio_ctrl_callback(struct ush_object *self, struct ush_file_descriptor const *file, int argc, char *argv[]) {
	(void)file;

	if (argc != 2 && argc != 3) {
		ush_print_status(self, USH_STATUS_ERROR_COMMAND_WRONG_ARGUMENTS);
		return;
	}

	if (strcmp(argv[1], "--dumpregs") == 0) {
		// we are abusing the process_index fields for this
		self->process_index = RADIO_DUMPREGS;
		self->process_index_item = 0;
		ush_process_start(self, file);
	}
	else if (strcmp(argv[1], "--isrfind") == 0) {
		hunt_radio_interrupt(self);
		ush_print(self, "done.");
	}
	else if (strcmp(argv[1], "--init") == 0) {
		rf_init();
		ble_baseband_init();
		ush_print(self, "RF+BLE init complete.");
	}
	else if (strcmp(argv[1], "--clk") == 0) {
		self->process_index = RADIO_BB_CLOCK;
		self->process_index_item = 0;
		ush_process_start(self, file);
	}
	else if (strcmp(argv[1], "--adv") == 0) {
		self->process_index = RADIO_ADVERTISE;
		self->process_index_item = (argc == 3) ? atoi(argv[2]) : 1;
		ush_process_start(self, file);
	}
	else {
		ush_print_status(self, USH_STATUS_ERROR_COMMAND_WRONG_ARGUMENTS);
	}
}

/*
 * Process one output item at a time.

 * ush_write_pointer() changes the shell to USH_STATE_WRITE_CHAR and
 * returns only after scheduling the write. When the write finishes,
 * MicroShell returns here with self->state set to the state supplied
 * as the second argument.
 */
void radio_ctrl_service(struct ush_object *self, struct ush_file_descriptor const *file) {
	char *output = self->desc->output_buffer;

	(void)file;

	switch (self->state) {
	case USH_STATE_PROCESS_START:
		if(self->process_index == RADIO_DUMPREGS) {
			ush_write_pointer(self, "--- DUMPING BLUETRUM RF REGISTERS ---\r\n", USH_STATE_PROCESS_SERVICE);
		}
		else if(self->process_index == RADIO_BB_CLOCK) {
			ush_write_pointer(self, "BB clock diffs after 100 ticks:\r\n", USH_STATE_PROCESS_SERVICE);
		}
		else if(self->process_index == RADIO_ADVERTISE) {
			ush_write_pointer(self, "Advertising: ", USH_STATE_PROCESS_SERVICE);
		}
		break;

	case USH_STATE_PROCESS_SERVICE:
		if(self->process_index == RADIO_DUMPREGS) {
			if (self->process_index_item <= 0xFF) {
				uint32_t value;

				if (rf_reg_rd((uint8_t)self->process_index_item, &value) == 0) {
					ush_write_pointer(
						self,
						"radio_ctrl: SPI transfer timeout\r\n",
						USH_STATE_RESET_PROMPT
					);
					break;
				}

				(void)snprintf(
					output,
					self->desc->output_buffer_size,
					"RF_REG[0x%02lX] = 0x%08lX\r\n",
					(unsigned long)self->process_index_item,
					(unsigned long)value
				);

				self->process_index_item++;

				ush_write_pointer(self, output, USH_STATE_PROCESS_SERVICE);
			}
			else {
				ush_write_pointer(self, "--- DUMP COMPLETE ---\r\n", USH_STATE_RESET_PROMPT);
			}
		}
		else if(self->process_index == RADIO_BB_CLOCK) {
			if (self->process_index_item <= 0xFF) {
				uint32_t clk1 = BB_CLK & 0x00FFFFFF;
				for(volatile int d = 0; d < 100; d++);
				uint32_t clk2 = BB_CLK & 0x00FFFFFF;

				(void)snprintf(
					output,
					self->desc->output_buffer_size,
					"%lu - %lu d(%lu)\r\n", clk1, clk2, (clk1 - clk2)
				);

				self->process_index_item++;

				ush_write_pointer(self, output, USH_STATE_PROCESS_SERVICE);
			}
			else {
				ush_write_pointer(self, "---\r\n", USH_STATE_RESET_PROMPT);
			}
		}
		else if(self->process_index == RADIO_ADVERTISE) {
			if(self->process_index_item > 0) {
				ble_send_adv_dtm(37);
				ble_send_adv_dtm(38);
				ble_send_adv_dtm(39);

				self->process_index_item--;
				ush_write_pointer(self, ".", USH_STATE_PROCESS_SERVICE);
			}
			else {
				ush_write_pointer(self, "\r\n--- done ---\r\n", USH_STATE_RESET_PROMPT);
			}
		}
		break;

	default:
		/*
		 * This should not normally occur because MicroShell only
		 * invokes the process callback in processing states.
		 */
		self->state = USH_STATE_RESET_PROMPT;
		break;
	}
}
