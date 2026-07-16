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

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

int main(void) {
	// disable watchdog
//	WDTCON = 0xaa0; // !!! WILL BRICK THE CHIP !!!
//
//	ROM_RESET_PIC(); // !!! OR IS IT THIS? !!!
//	ROM_SYSTEM_INIT(); // !!! OR THIS? !!!
//	ROM_UART0_INIT(); // !!! OR THIS? !!!

//	 while(1) { // !!! OR THIS? !!!
//		ROM_UART0_PUTCHAR('b');
//		ROM_DELAY(100000);
//	}

	uint8_t *addr = (uint8_t*)0x14000;
	*addr++ = 0x1a;
	*addr++ = 0x2b;
	*addr++ = 0x3c;
	*addr++ = 0x4e;
	return 0;
}
