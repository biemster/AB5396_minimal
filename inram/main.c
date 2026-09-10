#include <stdint.h>
#include "sfr.h"

/*
 * ============================================================================
 * AB5396B - CUSTOM OPEN-SOURCE XIP STAGE-1 DRIVER
 * ============================================================================
 */

/* 
 * The physical SPI flash offset that CPU address 0x10000000 maps to.
 * 0x2000 skips the boot headers
 */
#define XIP_FLASH_OFFSET 0x1000
#ifndef XIP_JUMP
#define XIP_JUMP 0
#endif

#define ISR7_CALLBACK   (*(volatile uint32_t *)(uintptr_t)0x00010044)

/*
 * --------------------------------------------------------------------------
 * ROM Functions
 * --------------------------------------------------------------------------
 */
typedef void (*rom_uart0_init)(void);
#define ROM_UART0_INIT ((rom_uart0_init)0x0008173e)

typedef void (*rom_uart0_putchar)(char c);
#define ROM_UART0_PUTCHAR ((rom_uart0_putchar)0x0008172e)

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

typedef void (*rom_system_init)(void);
#define ROM_SYSTEM_INIT ((rom_system_init)0x00084000)

/* We call the vendor's low-level DMA primitive directly */
typedef uint32_t (*rom_dma_t)(
	uintptr_t dest_ram, uintptr_t flash_offset, uint32_t size, 
	uint32_t lfsr_cfg, uint32_t lfsr_en
);
#define VENDOR_DMA ((rom_dma_t)0x0008964c)

/*
 * --------------------------------------------------------------------------
 * Format Helpers
 * --------------------------------------------------------------------------
 */
#ifndef REG32
#define REG32(addr)  (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

static void print_hex8(uint8_t v)
{
	static const char hex[] = "0123456789abcdef";
	ROM_UART0_PUTCHAR(hex[v >> 4]);
	ROM_UART0_PUTCHAR(hex[v & 0xf]);
}

static void print_hex32(uint32_t v)
{
	print_hex8(v >> 24);
	print_hex8(v >> 16);
	print_hex8(v >> 8);
	print_hex8(v & 0xff);
}

static void print_newline(void)
{
	ROM_UART0_PUTCHAR('\r');
	ROM_UART0_PUTCHAR('\n');
}

static void print_string(const char *str) {
	while (*str) {
		ROM_UART0_PUTCHAR(*str++);
	}
}

static void dump_system_state(const char *label) {
	print_string("\r\n--- STATE: ");
	print_string(label);
	print_string(" ---\r\n");

	/* Watchdog & Global Config */
	print_string("WDTCON:  "); print_hex32(WDTCON); print_newline();
	print_string("MEMCON:  "); print_hex32(MEMCON); print_newline();
	print_string("NMICON:  "); print_hex32(NMICON); print_newline();

	/* Proprietary Bluetrum Interrupt Controller (PIC) */
	print_string("PICCON:  "); print_hex32(PICCON); print_newline();
	print_string("PICEN:   "); print_hex32(PICEN); print_newline();
	print_string("PICPR:   "); print_hex32(PICPR); print_newline();
	print_string("PICADR:  "); print_hex32(PICADR); print_newline();
	print_string("PICPND:  "); print_hex32(PICPND); print_newline();

	/* Cache & Exceptions */
	print_string("CACHCON0:"); print_hex32(CACHCON0); print_newline();
	print_string("CACHCON1:"); print_hex32(CACHCON1); print_newline();
	print_string("EPICCON: "); print_hex32(EPICCON); print_newline();

	/* Misc */
	print_string("EFCON0:  "); print_hex32(EFCON0); print_newline();
	print_string("ISR7_CB: "); print_hex32(REG32(0x00010044)); print_newline();
	print_string("----------------------\r\n\r\n");
}

/*
 * --------------------------------------------------------------------------
 * CUSTOM CACHE FILLER
 * --------------------------------------------------------------------------
 */
__attribute__((noinline))
static uint32_t xip_callback(void)
{
	uint32_t fault_addr = ICADRMS;
	
	/* Shift CPU address to bypass headers and land on logical partition */
	uint32_t flash_offset = (fault_addr & 0x00ffffff) + XIP_FLASH_OFFSET;
	
	/* Calculate cache geometry */
	uint32_t page_num = fault_addr >> 9;
	uint32_t index    = page_num & 0x3f;
	uint32_t dest_ram = 0x00060000 + (index * 0x200);

	/* Unlock SRAM */
	ICINDEX = index;
	CACHCON1 = 0x10;
	ICTAG = 0;
	CACHCON1 = 0x04;

	/* Raw DMA Read (Unscrambled) */
	VENDOR_DMA((uintptr_t)dest_ram, (uintptr_t)flash_offset, 0x200, 0, 0);

	/* Lock SRAM & Set Valid Bit */
	ICTAG = page_num | 0x10000;
	CACHCON1 = 0x24;

	return 0;
}

/*
 * --------------------------------------------------------------------------
 * ENTRY
 * --------------------------------------------------------------------------
 */
int entry(void *ctx)
{
	(void)ctx;

	ROM_DELAY(100000);
	ROM_SYSTEM_INIT();
	ROM_UART0_INIT();

	ISR7_CALLBACK = (uint32_t)(uintptr_t)&xip_callback;

	/* Configure XIP Window & Reset Cache State */
	XIP_CMD   = 0x00000342;
	XIP_CTRL  = 1;
	XIP_BASE  = 0;
	XIP_LIMIT = 0;

	CACHCON0 = 0x00010001;
	CACHCON1 = 0;
	ICTAG    = 0;
	ICINDEX  = 0;
	ICADRMS  = 0;
	ICLOCK   = 0;

	/* Enable Interrupt Routing and Mapping */
	NMICON = 1;
	PICPR |= 1;
	MEMCON |= 0x00010000;

	/*
	 * ----------------------------------------------------------------------
	 * VERIFICATION: Read 1KB (1024 bytes) via XIP, or jump to it!
	 * ----------------------------------------------------------------------
	 */
#if XIP_JUMP
	print_newline();
	print_string("10000000: JUMP\r\n");

	/* DUMP STATE BEFORE JUMP */
	dump_system_state("IN-RAM STUB");

	/* 
	 * Cast the base of the XIP window to a function pointer 
	 * and execute it. 
	 */
	typedef void (*main_entry_t)(void);
	main_entry_t main_app = (main_entry_t)0x10000000;
	
	main_app();

	/* We should never reach this if main.bin loops forever */
	for (;;)
		;
#else
	volatile uint8_t *xip_ptr = (volatile uint8_t *)0x10000000;
	
	print_newline();
	ROM_UART0_PUTCHAR('>'); print_hex32(REG32(0x00011000));
	print_newline();
	ROM_UART0_PUTCHAR('>'); print_hex32(REG32(0x00011004));
	print_newline();
	
	for (int i = 0; i < 1024; i += 16) {
		
		/* 1. Print Address (e.g. 10000000: ) */
		print_hex32(0x10000000 + i);
		ROM_UART0_PUTCHAR(':');
		ROM_UART0_PUTCHAR(' ');

		/* 2. Print Hex Bytes (This triggers the XIP hardware faults!) */
		for (int j = 0; j < 16; j++) {
			print_hex8(xip_ptr[i + j]);
			if ((j % 2) == 1) ROM_UART0_PUTCHAR(' ');
		}

		ROM_UART0_PUTCHAR(' ');
		ROM_UART0_PUTCHAR('|');

		/* 3. Print ASCII Representation */
		for (int j = 0; j < 16; j++) {
			uint8_t c = xip_ptr[i + j];
			if (c >= 32 && c <= 126) {
				ROM_UART0_PUTCHAR(c);
			} else {
				ROM_UART0_PUTCHAR('.');
			}
		}
		
		ROM_UART0_PUTCHAR('|');
		print_newline();
	}

	print_newline();
#endif

	/* Halt safely */
	for (;;)
		;
		
	return 0;
}
