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
#define BB_INT_STAT    BB_REG(0x010)
#define BB_INT_EN      BB_REG(0x014)
#define BB_INT_CLR     BB_REG(0x018)
#define BB_NATIVE_CLK  BB_REG(0x020)
#define BB_MAC1        BB_REG(0x024)
#define BB_MAC2        BB_REG(0x028)
#define BB_DMA_PTR     BB_REG(0x02C)
#define BB_TASK_START  BB_REG(0x0E0)

// --- EM and TX Buffers ---
#define EM_BASE_ADDR   0x10EDC
#define TX_BUF_ADDR    0x10300

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

struct ble_tx_buffer {
	uint16_t header;       // [15:8] Length, [7:0] PDU Type
	uint8_t payload[37];
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
void ble_send_raw_packet_dtm(uint8_t phys_channel, uint8_t pdu_type, const uint8_t *payload, uint8_t len) {
	// 1. Auxiliary Header Registers
	*(volatile uint16_t*)(0x1124C) = (len << 8) | (pdu_type & 0x0F);
	
	// 2. Exact EM Block configuration for CS2 (0x10EDC)
	volatile uint8_t* cs2 = (volatile uint8_t*)0x10EDC;
	*(volatile uint16_t*)(cs2 + 0x00) = 0x001C;
	*(volatile uint16_t*)(cs2 + 0x02) = 0xBED6;       // AA Lower
	*(volatile uint32_t*)(cs2 + 0x04) = 0x55558E89;   // AA Upper | CRC Lower
	*(volatile uint16_t*)(cs2 + 0x08) = 0x0055;       // CRC Upper
	*(volatile uint16_t*)(cs2 + 0x0A) = phys_channel; 
	*(volatile uint32_t*)(cs2 + 0x0C) = 0x80000000 | TX_BUF_ADDR; 
	*(volatile uint16_t*)(cs2 + 0x10) = 0x064A; 
	*(volatile uint16_t*)(cs2 + 0x16) = 0x0960;
	
	// 3. Write payload (ensure 32-bit alignment if required by DMA)
	memcpy((void*)TX_BUF_ADDR, payload, len);

	// DO NOT OVERWRITE BB_DMA_PTR (0xF02C) HERE!

	// 4. Enable Global TX
	BB_CTRL |= 0x40000; 

	// 5. Fire Transmitter: Set Bit 12, Clear Bit 13
	uint32_t task_strt = BB_REG(0x0E0);
	task_strt &= ~(1 << 13);
	task_strt |= (1 << 12);
	BB_REG(0x0E0) = task_strt;

	// 6. Manual cycle-delay abort (since DTM loops infinitely)
	for(volatile uint32_t i = 0; i < 50000; i++); 

	// 7. Abort and Clear
	BB_CTRL &= ~0x40000;
	BB_INT_CLR = 0xFF; // Clear all pending to be safe
}
