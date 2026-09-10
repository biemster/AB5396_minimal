#include <stdint.h>
#include "sfr.h"

typedef void (*rom_reset_PIC)(void);
#define ROM_RESET_PIC ((rom_reset_PIC)0x00080c0c)

typedef void (*rom_system_init)(void);
#define ROM_SYSTEM_INIT ((rom_system_init)0x00080b94)

typedef void (*rom_uart0_init)(void);
#define ROM_UART0_INIT ((rom_uart0_init)0x0008173e)

typedef void (*rom_uart0_putchar)(char c);
#define ROM_UART0_PUTCHAR ((rom_uart0_putchar)0x0008172e)

typedef char (*rom_uart0_getchar)(void);
#define ROM_UART0_GETCHAR ((rom_uart0_putchar)0x00081712)

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

int main(void) {
	ROM_SYSTEM_INIT();
	ROM_UART0_INIT();

	while(1) {
		ROM_UART0_PUTCHAR('b');
		ROM_DELAY(100000);
	}

	return 0;
}
