#ifndef BLUETRUM_RADIO_H
#define BLUETRUM_RADIO_H

#include <stdint.h>
#include <string.h>
#include "sfr.h"

// --- SoS Memory Mapped Registers ---
#ifndef REG32
#define REG32(addr)    (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

#define BB_CLKCON      REG32(0x03A8)

#define RF_SPI_CTRL    REG32(0x8070)
#define RF_SPI_CFG     REG32(0x8074)
#define RF_SPI_DAT     REG32(0x81B4)
#define RF_SPI_CMD     REG32(0x81B8)

// --- Baseband SFRs ---
#define BB_REG(offset) REG32(0xF000 + (offset))
#define BB_CTRL        BB_REG(0x000)
#define BB_INT_STAT    BB_REG(0x010)
#define BB_INT_EN      BB_REG(0x014)
#define BB_INT_CLR     BB_REG(0x018)
#define BB_CLK         BB_REG(0x020)
#define BB_MAC1        BB_REG(0x024)
#define BB_MAC2        BB_REG(0x028)
#define BB_GLOBAL_PTR  BB_REG(0x02C)
#define BB_ERR_CODE    BB_REG(0x060)
#define BB_TASK_START  BB_REG(0x0E0)

// --- EM and TX Buffers ---
#define EM_CS2_BASE       0x10EDC
#define TX_BUF_ADDR       0x10300
#define EM_HW_EVENT_NODE  0x10400 // Where the MAC hardware looks for schedules
#define EM_DUMMY_BUF      0x11000

struct ll_em_block {
	uint16_t state;
	uint32_t access_addr;
	uint32_t crc_init;
	uint16_t rf_channel;
	uint32_t tx_dma_ptr;
	uint16_t unk_10;
	uint32_t padding;
	uint16_t unk_16;
	uint32_t unused1;
	uint32_t unused2;
} __attribute__((packed));


// --- RF SPI primitives ---

/* Trigger the SPI transfer and wait for completion. */
int rf_spi_tf(void) {
	uint32_t timeout = 1000000;

	RF_SPI_CTRL |= 0x01;

	while ((RF_SPI_CTRL & 0x02) == 0) {
		if (timeout-- == 0) {
			return 0;
		}
	}

	return 1;
}

/* Read an RF macrocell register. */
int rf_reg_rd(uint8_t reg_addr, uint32_t *value) {
	RF_SPI_CMD = (uint32_t)reg_addr | 0x100;

	if (rf_spi_tf() == 0) {
		return 0;
	}

	*value = RF_SPI_DAT;
	return 1;
}

/* Write an RF macrocell register. */
void rf_reg_wr(uint8_t reg_addr, uint32_t value) {
	RF_SPI_DAT = value;
	RF_SPI_CMD = (uint32_t)reg_addr | 0x300;
	rf_spi_tf();
}

// --- RF Initialization ---

void rf_init(void) {
	// 1. Enable RF digital interfaces and clocks
	// Extracted directly from the assembly: a4 = 0x2000 - 0x46c = 0x1b94
	RF_SPI_CFG |= 0x1B94;
	
	// 2. Clear reset/suspend bits on the SPI controller
	RF_SPI_CTRL &= ~0x60;
	
	// 3. Post-Bootrom calibration patches
	rf_reg_wr(0xAA, 0x03);
	
	// 4. Enable RF die subsystem (Bit 1 of Reg 0x81)
	uint32_t reg81 = 0;
	if(rf_reg_rd(0x81, &reg81)) {
		rf_reg_wr(0x81, reg81 | 0x02);
	}
}

// --- Link Layer / Baseband Utilities ---

// Extracted from rf_cacl_rssi
// param_1 is a hardware metric (usually AGC gain or raw magnitude)
// param_2 is a hardware metric containing LNA state
int32_t rf_calc_rssi(int32_t raw_rssi, uint32_t lna_state) {
	uint32_t lna_idx = lna_state >> 4;
	uint32_t offset = 0;
	
	if (lna_idx >= 7) {
		offset = (lna_idx * 6) - 0x28;
	}
	
	// The formula reconstructed from Ghidra:
	// RSSI = (raw_rssi - 40 + (lna_state & 0xF) * -3) - offset
	return (raw_rssi - 0x28 + ((lna_state & 0xF) * -3)) - offset;
}

// Extracted from rf_txpwr_dbm_get
// Returns the dBm value of a Link Layer power index
int8_t rf_txpwr_dbm_get(uint8_t pwr_idx) {
	// 0 = 0 dBm, 1 = -3 dBm, 2 = -6 dBm, 3 = -9 dBm
	// (Assuming rf_ctl base offset is 0 for simplicity)
	return (pwr_idx & 3) * -3;
}

// BLE
// 1. Initialize the Baseband Hardware Engine
// Extracted byte-for-byte from BootROM hwble_init. 
// Without this, the DMA and State Machine are dead.
void ble_baseband_init(void) {
	BB_CTRL = (BB_CTRL & 0xFFFFFF0F) | 0xE0;
	BB_REG(0x00C) = 0x13A;
	BB_REG(0x090) = 0x07;
	BB_REG(0x0B0) = 0x1CD4;
	BB_REG(0x0B4) = 0x1CE0;
	BB_REG(0x0B8) = 0x202;
	BB_REG(0x0F0) = 0x96;
	BB_REG(0x110) = 0x801E;

	// Set the Hardware MAC Address (11:22:33:44:55:66)
	BB_MAC1 = 0x44332211; // Lower 4 bytes
	BB_MAC2 = 0x00006655; // Upper 2 bytes
}

// 2. Transmit a raw Link Layer packet
void ble_send_adv_dtm(uint8_t phys_channel) {
	// Clear stale state
	BB_CTRL &= ~(1 << 18);
	BB_INT_CLR = 0xFFFFFFFF; 

	// Setup CS2 (DTM / Manual Mode Control Structure)
	volatile uint16_t* cs2 = (volatile uint16_t*)EM_CS2_BASE;
	cs2[0]  = 0x001C;                               // State
	cs2[1]  = 0xBED6;                               // AA Lower
	*(volatile uint32_t*)&cs2[2] = 0x55558E89;      // AA Upper | CRC Lower
	cs2[4]  = 0x0055;                               // CRC Upper
	cs2[5]  = phys_channel;                         // Channel (37, 38, or 39)
	*(volatile uint32_t*)&cs2[6] = 0x80000000 | TX_BUF_ADDR; // DMA Payload Ptr
	cs2[8]  = 0x064A;                               // Config 1
	cs2[11] = 0x0960;                               // Config 2

	// Valid BLE Advertisement Payload (ADV_NONCONN_IND)
	uint8_t adv_payload[] = {
		0x11, 0x22, 0x33, 0x44, 0x55, 0x66, // Address LSB first
		0x02, 0x01, 0x06,                   // AD 1: Flags
		0x05, 0x09, 'B', 'A', 'R', 'E'      // AD 2: Name
	};
	uint8_t len = sizeof(adv_payload);
	
	// Copy to EM SRAM
	volatile uint8_t *tx_buf = (volatile uint8_t *)TX_BUF_ADDR;
	for (int i = 0; i < len; i++) {
		tx_buf[i] = adv_payload[i];
	}

	// Write Aux Header (Type 0x02 = ADV_NONCONN_IND)
	*(volatile uint16_t*)(0x1124C) = (len << 8) | 0x02;

	// Enable MAC Global TX
	BB_CTRL |= (1 << 18);

	// Fire! (Manual Type 3 Bypass)
	// Because hardware_init() is active, the clock is ticking, and the hardware WILL transition.
	uint32_t task_strt = BB_TASK_START;
	task_strt &= ~(1 << 13);
	task_strt |=  (1 << 13); // Test Mode bit
	BB_TASK_START = task_strt;
	
	task_strt |= (1 << 12);  // Start Task Bit
	BB_TASK_START = task_strt;

	// Poll for TX Done (Bit 1 in BB_INT_STAT)
	uint32_t timeout = 20000;
	while (timeout--) {
		if (BB_INT_STAT & 0x02) {
			break; // Boom!
		}
	}

	// Acknowledge and stop
	BB_INT_CLR = 0xFFFFFFFF;
	BB_CTRL &= ~(1 << 18);
}


void hunt_radio_interrupt(uint32_t *result) {
	// 1. Enable ALL interrupts in the PIC so they latch into PICPND
	PICENSET = 0xFFFFFFFF; 
	
	// 2. Clear any old baseband states
	BB_INT_CLR = 0xFFFFFFFF;
	
	// 3. Record baseline noise (e.g., standard timers that might be ticking)
	uint32_t baseline_pnd = PICPND;

	// 4. Trigger the packet transmission
	rf_init();
	ble_baseband_init();
	ble_send_adv_dtm(37);
	
	// 5. Poll for the hardware line to go high
	uint32_t timeout = 500000;
	while (timeout--) {
		uint32_t current_pnd = PICPND;
		
		// Mask out the baseline noise (UART, SysTick, etc.)
		current_pnd &= ~(baseline_pnd);
		
		if (current_pnd != 0) {
			// WE CAUGHT IT!
			// If current_pnd == 0x00000800, then the Radio IRQ is Bit 11.
			uint32_t bb_status = BB_INT_STAT;
			
			// Print or log `current_pnd` and `bb_status` here.
			result[0] = current_pnd;
			result[1] = bb_status;
			
			// Acknowledge the interrupt on the MAC side
			BB_INT_CLR = bb_status;
			break;
		}
	}
}

uint32_t poke_ceva_mac(uint32_t *result) {
	// 1. Stop MAC and clear interrupts
	BB_CTRL &= ~(1 << 18);
	BB_INT_CLR = 0xFFFFFFFF; 
	
	// 2. Build the Control Structure (CS) for a simple RX listen
	volatile uint16_t* cs = (volatile uint16_t*)(uintptr_t)EM_CS2_BASE;
	cs[0]  = 0x001D;                          // State: 0x1D = RX Mode
	cs[1]  = 0xBED6;                          // AA Lower (Dummy)
	*(volatile uint32_t*)&cs[2] = 0x55558E89; // AA Upper + CRC
	cs[4]  = 0x0055;                          // CRC Upper
	cs[5]  = 37;                              // Channel
	*(volatile uint32_t*)&cs[6] = 0;          // DMA Ptr (0 is fine for RX)
	cs[8]  = 0x0000;                          // Config 1
	cs[11] = 0x0000;                          // Config 2

	// 3. Point the Hardware Global Pointer DIRECTLY to the CS!
	// In bypass mode, it doesn't want an Event Node. It wants the CS.
	BB_GLOBAL_PTR = EM_CS2_BASE;

	// 4. Enable Global MAC TX/RX hardware
	BB_CTRL |= (1 << 18);

	// 5. Fire! Force Manual Execution Bypass
	uint32_t task = BB_TASK_START;
	task &= ~(1 << 13); // Clear Continuous Test Mode
	task |=  (1 << 12); // Start Task!
	BB_TASK_START = task;

	// 6. Wait for a reaction
	uint32_t caught_stat = 0;
	uint32_t timeout = 500000; // ~500ms timeout
	
	while (--timeout) {
		caught_stat = BB_INT_STAT;
		if (caught_stat != 0) {
			break; // WE GOT A REACTION!
		}
	}

	// 7. Capture Error Code
	uint32_t err = BB_ERR_CODE;

	// 8. Cleanup
	BB_INT_CLR = 0xFFFFFFFF;
	BB_CTRL &= ~(1 << 18);

	result[0] = caught_stat;
	result[1] = err;
	result[2] = timeout;
}

#endif // BLUETRUM_RADIO_H
