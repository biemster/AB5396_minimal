#include <stdint.h>
#include "sfr.h"

/*
 * ============================================================================
 * AB5396B - MINIMAL CUSTOM STAGE-1 BOOTLOADER
 * ============================================================================
 */

/* Physical SPI Flash offset where main.bin is located */
#ifndef MAIN_FLASH_OFFSET
#define MAIN_FLASH_OFFSET 0x1000
#endif

/* Set to 1 to test the wait loop (waits ~2 sec and safely returns to ROM).
 * Set to 0 for production (waits ~100ms, boots main.bin on timeout). */
#define TEST_MODE                    0
#define TEST_MODE_REASON_ADDR        0x11000
#define TEST_MODE_RETRY_COUNTER_ADDR 0x11004

#ifndef REG32
#define REG32(addr)  (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

#define ISR7_CALLBACK REG32(0x00010044)

/* ROM Functions */
typedef void (*rom_uart0_init)(void);
#define ROM_UART0_INIT ((rom_uart0_init)0x0008173e)

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

typedef void (*rom_system_init)(void);
#define ROM_SYSTEM_INIT ((rom_system_init)0x00084000)

typedef uint32_t (*rom_dma_t)(
	uintptr_t dest_ram, uintptr_t flash_offset, uint32_t size, 
	uint32_t lfsr_cfg, uint32_t lfsr_en
);
#define VENDOR_DMA ((rom_dma_t)0x0008964c)


/*
 * --------------------------------------------------------------------------
 * Non-Blocking UART Poll
 * --------------------------------------------------------------------------
 */
static int uart0_poll(void) {
	if ((UART0CON & 0x200) == 0) return -1;
	uint32_t data = UART0DATA;
	UART0CPND = 0x200;
	return (data & 0xff);
}


/*
 * --------------------------------------------------------------------------
 * CUSTOM CACHE FILLER
 * --------------------------------------------------------------------------
 */
__attribute__((noinline))
static uint32_t xip_callback(void) {
	uint32_t fault_addr = ICADRMS;
	
	/* Map CPU address to physical Flash */
	uint32_t flash_offset = (fault_addr & 0x00ffffff) + MAIN_FLASH_OFFSET;
	
	/* Cache Geometry */
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
 * STAGE 1 BOOTLOADER ENTRY
 * --------------------------------------------------------------------------
 */
void stage1(void) {
	ROM_DELAY(100000);
	ROM_SYSTEM_INIT();
	ROM_UART0_INIT();

	/*
	 * 1. HOST SYNC / MAGIC WORD CHECK
	 * Listen for the flashing tool's magic byte (0x5A / 'Z')
	 */
	for (int i = 0; i < 10; i++) { // retry 10 times
		int c;
		while ((c = uart0_poll()) != -1) {
			if (c == 0x5a) {
				/* MAGIC CAUGHT! Return safely to BootROM to allow flashing */
				REG32(TEST_MODE_REASON_ADDR) = 0xCA67CA67; // CAUGHT
				REG32(TEST_MODE_RETRY_COUNTER_ADDR) = (uint32_t)i;
				return;
			}
		}
		
		/* Small delay between polling cycles */
		ROM_DELAY(10000);
	}

#if TEST_MODE
	/* In TEST_MODE, always return safely to ROM to avoid bricking */
	REG32(TEST_MODE_REASON_ADDR) = 0x713E713E; // TIME
	return;
#endif

	/*
	 * 2. INITIALIZE XIP HARDWARE
	 * No magic word received; proceed to boot main firmware.
	 */
	ISR7_CALLBACK = (uint32_t)(uintptr_t)&xip_callback;

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

	NMICON = 1;
	PICPR |= 1;
	MEMCON |= 0x00010000;

	/*
	 * 3. BOOT MAIN APPLICATION
	 */
	typedef void (*main_entry_t)(void);
	main_entry_t main_app = (main_entry_t)0x10000000;

	main_app();

	/* Should never reach here */
	for(;;);
}
