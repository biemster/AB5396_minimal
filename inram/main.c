#include <stdint.h>
#include "sfr.h"

typedef void (*rom_uart0_init)(void);
#define ROM_UART0_INIT ((rom_uart0_init)0x0008173e)

typedef void (*rom_uart0_putchar)(char c);
#define ROM_UART0_PUTCHAR ((rom_uart0_putchar)0x0008172e)

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

typedef void (*rom_goto_12010)(void);
#define ROM_GOTO_12010 ((rom_goto_12010)0x000813f2)

typedef void (*rom_USB_eventloop)(void);
#define ROM_USB_EVENTLOOP ((rom_USB_eventloop)0x00080c44)

typedef void (*rom_goto_stage1)(void);
#define ROM_GOTO_STAGE1 ((rom_goto_stage1)0x00010800)

uint8_t *bootrom_checkpoint = (uint8_t*)0x0001204b;

int entry(void *ctx) {

	// give the bootrom some time to ack entering this
	ROM_DELAY(100000);

	// disable watchdog
//	WDTCON = 0xaa0;

//	uint8_t bcp1 = *bootrom_checkpoint;
//	// ROM_GOTO_12010();
//	uint8_t bcp2 = *bootrom_checkpoint;

	ROM_UART0_INIT();
//	ROM_UART0_PUTCHAR('>');
//	ROM_UART0_PUTCHAR(' ');

//	ROM_UART0_PUTCHAR('0' + bcp1);
//	ROM_UART0_PUTCHAR(',');
//	ROM_UART0_PUTCHAR('0' + bcp2);
//	ROM_UART0_PUTCHAR(',');

	uint8_t *addr = (uint8_t*)0x10800;
	while(addr < 0x10810) {
		ROM_UART0_PUTCHAR(*addr++);
	}

//	addr = (uint8_t*)0x12000;
//	while(addr < 0x12020) {
//		ROM_UART0_PUTCHAR(*addr++);
//	}

//	PICADR = 0x80000;
//	ROM_GOTO_STAGE1();

	addr = (uint8_t*)0x14000;
	while(addr < 0x14004) {
		ROM_UART0_PUTCHAR(*addr++);
	}

	while(1) {
		ROM_UART0_PUTCHAR('x');
		ROM_DELAY(100000);
		WDTCON = 10; // feed
	}
}
