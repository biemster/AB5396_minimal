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

#include "inc/ush.h"
#include "inc/ush_internal.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Mapped SFRs for the internal RF SPI bus. */
#define RF_SPI_CTRL (*(volatile uint32_t *)0x8070U)
#define RF_SPI_DAT  (*(volatile uint32_t *)0x81B4U)
#define RF_SPI_CMD  (*(volatile uint32_t *)0x81B8U)

/* Trigger the SPI transfer and wait for completion. */
static int rf_spi_tf(void) {
	uint32_t timeout = 1000000U;

	RF_SPI_CTRL |= 0x01U;

	while ((RF_SPI_CTRL & 0x02U) == 0U) {
		if (timeout-- == 0U) {
			return 0;
		}
	}

	return 1;
}

/* Read an RF macrocell register. */
static int rf_reg_rd(uint8_t reg_addr, uint32_t *value) {
	RF_SPI_CMD = (uint32_t)reg_addr | 0x100U;

	if (rf_spi_tf() == 0) {
		return 0;
	}

	*value = RF_SPI_DAT;
	return 1;
}

/* Write an RF macrocell register. */
static void rf_reg_wr(uint8_t reg_addr, uint32_t value) {
	RF_SPI_DAT = value;
	RF_SPI_CMD = (uint32_t)reg_addr | 0x300U;
	rf_spi_tf();
}

/*
 * Start the register dump.
 */
static void dump_rf_registers_start(struct ush_object *self, struct ush_file_descriptor const *file) {
	self->process_index = 0U;
	ush_process_start(self, file);
}

/*
 * Command callback.
 * Do not dump all registers here. Start the asynchronous MicroShell
 * processing state machine instead.
 */
void radio_ctrl_callback(struct ush_object *self, struct ush_file_descriptor const *file, int argc, char *argv[]) {
	if (argc != 2 || strcmp(argv[1], "--dumpregs") != 0) {
 		ush_print_status(self, USH_STATUS_ERROR_COMMAND_WRONG_ARGUMENTS);
 		return;
 	}

	dump_rf_registers_start(self, file);
}

/*
 * Process one output item at a time.

 * ush_write_pointer() changes the shell to USH_STATE_WRITE_CHAR and
 * returns only after scheduling the write. When the write finishes,
 * MicroShell returns here with self->state set to the state supplied
 * as the second argument.
 */
void dump_rf_registers_service(struct ush_object *self, struct ush_file_descriptor const *file) {
	char *output = self->desc->output_buffer;

	(void)file;

	switch (self->state) {
	case USH_STATE_PROCESS_START:
		ush_write_pointer(
			self,
			"--- DUMPING BLUETRUM RF REGISTERS ---\r\n",
			USH_STATE_PROCESS_SERVICE
		);
		break;

	case USH_STATE_PROCESS_SERVICE:
		if (self->process_index <= 0xFFU) {
			uint32_t value;

			if (rf_reg_rd((uint8_t)self->process_index, &value) == 0) {
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
				(unsigned long)self->process_index,
				(unsigned long)value
			);

			self->process_index++;

			ush_write_pointer(self, output, USH_STATE_PROCESS_SERVICE);
		} else {
			ush_write_pointer(self, "--- DUMP COMPLETE ---\r\n", USH_STATE_RESET_PROMPT);
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
