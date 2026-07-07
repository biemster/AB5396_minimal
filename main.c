#include <stdint.h>
#include "sfr.h"

typedef void (*rom_uart0_init)(void);
#define ROM_UART0_INIT ((rom_uart0_init)0x0008173e)

typedef void (*rom_uart0_putchar)(char c);
#define ROM_UART0_PUTCHAR ((rom_uart0_putchar)0x0008172e)

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

int entry(void *ctx) {

	// give the bootrom some time to ack entering this
	ROM_DELAY(100000);

	// disable watchdog
	WDTCON = 0xaa0;

	ROM_UART0_INIT();
	while(1) {
		ROM_UART0_PUTCHAR('x');
		ROM_DELAY(100000);
	}
}
