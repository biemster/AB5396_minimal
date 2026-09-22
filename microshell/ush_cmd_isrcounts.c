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

#include "inc/ush.h"
#include "inc/ush_internal.h"

/* Implemented in main.c. */
extern uint32_t isr_vector_counter_get(size_t index);

#define ISR_VECTOR_COUNTER_COUNT 16

/*
 * Start the asynchronous ISR counter dump.
 */
static void isr_counts_start(
	struct ush_object *self,
	struct ush_file_descriptor const *file
)
{
	self->process_index = 0;
	ush_process_start(self, file);
}

/*
 * Command callback.
 */
void isr_counts_callback(
	struct ush_object *self,
	struct ush_file_descriptor const *file,
	int argc,
	char *argv[]
)
{
	(void)argv;

	if (argc != 1) {
		ush_print_status(
			self,
			USH_STATUS_ERROR_COMMAND_WRONG_ARGUMENTS
		);
		return;
	}

	isr_counts_start(self, file);
}

/*
 * Asynchronous output service.
 *
 * One counter line is sent for each service iteration. This keeps the
 * command non-blocking and allows MicroShell to wait for each character
 * transmission to complete.
 */
void isr_counts_service(
	struct ush_object *self,
	struct ush_file_descriptor const *file
)
{
	char *output = self->desc->output_buffer;

	(void)file;

	switch (self->state) {
	case USH_STATE_PROCESS_START:
		ush_write_pointer(
			self,
			"--- ISR VECTOR COUNTS ---\r\n",
			USH_STATE_PROCESS_SERVICE
		);
		break;

	case USH_STATE_PROCESS_SERVICE:
		if (self->process_index < ISR_VECTOR_COUNTER_COUNT) {
			uint32_t count = isr_vector_counter_get(
				(size_t)self->process_index
			);

			(void)snprintf(
				output,
				self->desc->output_buffer_size,
				"ISR_COUNT[%02lu] = %lu\r\n",
				(unsigned long)self->process_index,
				(unsigned long)count
			);

			self->process_index++;

			ush_write_pointer(
				self,
				output,
				USH_STATE_PROCESS_SERVICE
			);
		} else {
			ush_write_pointer(
				self,
				"--- ISR COUNT DUMP COMPLETE ---\r\n",
				USH_STATE_RESET_PROMPT
			);
		}
		break;

	default:
		self->state = USH_STATE_RESET_PROMPT;
		break;
	}
}
