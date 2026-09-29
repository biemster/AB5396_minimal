#ifndef BLUETRUM_RADIO_H
#define BLUETRUM_RADIO_H

#include <stdint.h>
#include <string.h>
#include "sfr.h"

// --- SoS Memory Mapped Registers ---
#ifndef REG32
#define REG32(addr)    (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

#define RF_SPI_CTRL    REG32(0x8070)
#define RF_SPI_CFG     REG32(0x8074)
#define RF_SPI_DAT     REG32(0x81B4)
#define RF_SPI_CMD     REG32(0x81B8)

// --- Baseband SFRs ---
#define BB_REG(offset) REG32(0xF000 + (offset))
#define BB_CTRL        BB_REG(0x000)
#define BB_INT_MASK    BB_REG(0x00C)
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


/* --- Digital Baseband Registers (CEVA RivieraWaves) --- */
#define RWBLE_BASE      0xF000
#define RWBLE_CTRL      *(volatile uint32_t*)(RWBLE_BASE + 0x00)
#define RWBLE_INTACK    *(volatile uint32_t*)(RWBLE_BASE + 0x18)
#define RWBLE_TEST_CTRL *(volatile uint32_t*)(RWBLE_BASE + 0xE0)

/* --- Exchange Memory (EM) Layout --- */
#define EM_BASE         0x10000
// Base address of the Control Structure (CS) used for TX
#define EM_CS_TX        (EM_BASE + 0x0EDC)
// Packet buffer address derived from `0x11000 + 0x24C` in the ASM
#define EM_TX_BUF       (EM_BASE + 0x124C)


/**
 * Transmits a raw BLE frame using the MAC's Hardware Test Mode.
 * 
 * @param channel BLE Channel index (0-39). For ADV use 37, 38, or 39.
 * @param payload Pointer to your custom BLE payload.
 * @param len     Length of the payload.
 */
void baremetal_ble_tx_custom_frame(uint8_t channel, uint8_t* payload, uint8_t len) {
	
	// ---------------------------------------------------------
	// 1. Reset & Enable Digital Baseband (from hwble_reset/init)
	// ---------------------------------------------------------
	RWBLE_CTRL &= ~0x101;                      // Assert baseband soft-reset
	for(volatile int i = 0; i < 1000; i++);    // Wait (replaces custom1() loop)
	
	*(volatile uint32_t*)(RWBLE_BASE + 0x0C) = 0;
	RWBLE_INTACK = 0xFFFFFFFF;                 // Clear all pending RW interrupts

	RWBLE_CTRL = (RWBLE_CTRL & ~0xF1) | 0xE0;  // Enable MAC clocks/logic
	*(volatile uint32_t*)(RWBLE_BASE + 0xF0) = 0x96;
	*(volatile uint32_t*)(RWBLE_BASE + 0x0C) = 0x13A;

	// ---------------------------------------------------------
	// 2. Setup the Packet Buffer in Exchange Memory (EM)
	// ---------------------------------------------------------
	volatile uint16_t* tx_hdr = (volatile uint16_t*)EM_TX_BUF;
	volatile uint8_t*  tx_buf = (volatile uint8_t*)(EM_TX_BUF + 2);

	// Header: [15:8] = Length, [7:0] = Type 
	// Type 0x02 is ADV_NONCONN_IND, which is perfectly valid for a beacon/test
	*tx_hdr = (len << 8) | 0x02;

	// Copy the raw payload bytes into Exchange Memory
	for(int i = 0; i < len; i++) {
		tx_buf[i] = payload[i];
	}

	// ---------------------------------------------------------
	// 3. Setup the Control Structure (CS) at 0x10EDC
	// ---------------------------------------------------------
	// The baseband hardware reads this struct via DMA to configure the radio
	volatile uint16_t* cs = (volatile uint16_t*)EM_CS_TX;

	cs[0]  = 0x001C;            // [0x10EDC] Type/Ctrl: 0x1C = TX Test Mode
	cs[1]  = 0xBED6;            // [0x10EDE] Sync Word Low  (BLE ADV Sync: 0x8E89BED6)
	cs[2]  = 0x8E89;            // [0x10EE0] Sync Word High
	cs[3]  = 0x5555;            // [0x10EE2] CRC Init High  (Standard BLE ADV CRC: 0x555555)
	cs[4]  = 0x0055;            // [0x10EE4] CRC Init Low
	cs[5]  = channel & 0x3F;    // [0x10EE6] Channel Freq Index
	cs[6]  = 0x0300;            // [0x10EE8] TX Flags
	cs[7]  = 0x8001;            // [0x10EEA] EM Buffer Ptr (Magic value derived from ASM)
	cs[8]  = 0x064A;            // [0x10EEC] Hardware Timing Magic Number
	cs[11] = 0x0960;            // [0x10EF2] Hardware Timing Magic Number
	cs[12] = 0x0000;            // [0x10EF4] 
	cs[14] = 0x0000;            // [0x10EF8]

	// ---------------------------------------------------------
	// 4. Fire the Baseband Hardware Trigger
	// ---------------------------------------------------------
	// RWBLE_TEST_CTRL (0xF0E0) activates DTM TX.
	// The decompilation of ble_tx_test_do does: 0xF0E0 |= (1 << 13) | channel
	RWBLE_TEST_CTRL = (1 << 13) | (channel & 0x3F);
	
	// The hardware will now autonomously and continuously transmit the 
	// frame in the buffer. To stop it, simply write 0 to RWBLE_TEST_CTRL.
}

#endif // BLUETRUM_RADIO_H
