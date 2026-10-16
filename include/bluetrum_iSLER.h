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
	RF_SPI_CFG |= 0x29F8;
	
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


//
// PLL tests
//
/* Bluetrum custom DSP opcode emitted via .insn */
static inline uint32_t bluetrum_dsp_7x(uint32_t val, uint32_t param) {
	uint32_t res;
	register uint32_t rs2_reg asm("t0") = param; 
	__asm__ volatile (
		".insn r 0x33, 4, 0x73, %0, %1, %2"
		: "=r" (res) : "r" (val), "r" (rs2_reg)
	);
	return res;
}

typedef void (*rom_uart0_putchar)(char c);
static inline void mark(char c) { ((rom_uart0_putchar)0x0008172e)(c); }

/*
 * rf_filter_init() reads a runtime mode from vendor RAM.
 * We cannot assume that RAM configuration exists in this test.
 *
 * Start with mode zero, which clears the mode field. If the PLL
 * still fails, this field is one of the configuration variables
 * to investigate rather than an assumed universal default.
 */
#ifndef PLLTEST_FILTER_MODE
#define PLLTEST_FILTER_MODE 0u
#endif

#define PLL_LOCK_BIT       0x00800000u
#define PLL_AFC_TRIGGER    0x00008000u
#define PLL_AFC_CLEAR_BIT  0x40000000u
#define PLL_LOCK_TIMEOUT   0x00020000u

/*
 * Globally visible in the firmware map so the structure can be
 * inspected in your RAM dump after the function returns.
 */
typedef struct {
	uint32_t status_before_trigger;
	uint32_t status_first;
	uint32_t status_100;
	uint32_t status_1000;
	uint32_t status_last_poll;
	uint32_t status_before_stop;
	uint32_t status_after_stop;
	uint32_t status_seen_or;
	uint32_t poll_count;
	uint32_t locked;

	uint32_t reg_7c04;
	uint32_t reg_7c70;
	uint32_t reg_7c74;
	uint32_t reg_7c84;
	uint32_t reg_7c7c;
	uint32_t reg_7ce0;
	uint32_t reg_7ce4;
	uint32_t reg_7cec;
	uint32_t reg_7cf0;
	uint32_t reg_7d20;
	uint32_t reg_7dd0;
} pll_lock_trace_t;

volatile pll_lock_trace_t g_pll_lock_trace;

typedef struct {
	uint32_t c04, c70, c74, c7c, c84;
	uint32_t ce0, ce4, cec, cf0, d20, dd0;
} pll_snapshot_t;

volatile pll_snapshot_t pll_first_snapshot;
volatile pll_snapshot_t pll_latest_snapshot;
volatile uint32_t pll_snapshot_count;

static void pll_capture_snapshot(volatile pll_snapshot_t *s)
{
	s->c04 = REG32(0x7C04);
	s->c70 = REG32(0x7C70);
	s->c74 = REG32(0x7C74);
	s->c7c = REG32(0x7C7C);
	s->c84 = REG32(0x7C84);
	s->ce0 = REG32(0x7CE0);
	s->ce4 = REG32(0x7CE4);
	s->cec = REG32(0x7CEC);
	s->cf0 = REG32(0x7CF0);
	s->d20 = REG32(0x7D20);
	s->dd0 = REG32(0x7DD0);
}

/*
 * Approximate replacement for the vendor delay routine.
 *
 * These are CPU-loop counts, NOT calibrated microseconds.
 * If you have a known-working direct call to the vendor ROM
 * delay function, substitute it here after verifying its ABI.
 */
static void plltest_delay(uint32_t count)
{
	volatile uint32_t d;

	for (d = 0; d < count; ++d) {
		/* Volatile loop prevents this delay from being optimized away. */
	}
}


/*
 * Reimplementation of rf_ldo_en_fc(a0, a1, a2).
 *
 * The argument roles below follow the actual assembly:
 *
 *   a0 = enable/disable path
 *   a1 = VCO/configuration variant
 *   a2 = initial LDO-control toggle
 *
 * In particular, rf_ldo_en_fc(1, 0, 0) takes the ENABLE branch.
 * It must not execute the register-clearing branch that ends with
 * 7D20 &= 0xFFFFFFE2, 7CF0 &= ~1 and 7CE0 &= ~1.
 */
static void plltest_rf_ldo_en_fc(
	uint32_t enable,
	uint32_t variant,
	uint32_t initial_toggle)
{
	uint32_t v;

	/*
	 * The original assembly gates this initial toggle on a2.
	 * The D20 bit-0 set and CE4 transition are followed by two
	 * vendor delays with argument 6.
	 */
	if (enable != 0) {
		REG32(0x7D20) |= 0x00000001u;

		REG32(0x7CE4) &= 0xFFFFFFEFu;
		plltest_delay(6);

		REG32(0x7CE4) |= 0x00000010u;
		plltest_delay(6);
	}

	REG32(0x7CE4) |= 0x00000010u;

	if (enable == 0) {
		/*
		 * Exact clearing branch from the assembly.
		 * This is NOT the branch used by rf_pll_afc().
		 */
		REG32(0x7C84) &= 0xFA3FFFFFu;
		REG32(0x7C84) &= 0xFFFFF8FFu;
		REG32(0x7C84) |= 0x00000300u;
		REG32(0x7C84) &= 0x47FFFFFFu;

		REG32(0x7C74) &= 0xFFFFFDFFu;
		REG32(0x7D20) &= 0xFFFFFFE2u;
		REG32(0x7CF0) &= 0xFFFFFFFEu;
		REG32(0x7CE0) &= 0xFFFFFFFEu;

		return;
	}

	/*
	 * Enable branch. Both rf_ldo_en_fc(1, 1, 1) and
	 * rf_ldo_en_fc(1, 0, 0) enter here.
	 */
	REG32(0x7D20) |= 0x0000001Du;

	REG32(0x7C84) &= 0xFFFFF8FFu;
	REG32(0x7C84) |= 0x00000300u;
	REG32(0x7C84) |= 0x05C00000u;

	if (variant == 0) {
		/*
		 * a1 == 0: clear the upper configuration fields.
		 */
		REG32(0x7C84) &= 0x47FFFFFFu;
	} else {
		/*
		 * a1 != 0: delay(0x23), then set the high VCO fields.
		 */
		plltest_delay(0x23u);
		REG32(0x7C84) |= 0xB8000000u;
	}

	REG32(0x7C74) |= 0x00000200u;
	REG32(0x7CF0) |= 0x00000001u;
	REG32(0x7CE0) |= 0x00000001u;

	(void)v;
}


/*
 * Reimplementation of rf_pll_en(a0, a1, a2).
 *
 * The parameters a1 and a2 matter in the enable path:
 *   a2 != 0 -> clear bit 21 in 0x7CE8
 *   a1 != 0 -> clear bit 29 in 0x7CEC
 *
 * This distinction matters for the two calls in your original code:
 *   rf_pll_en(1, 0, 1)
 *   rf_pll_en(1, 0, 0)
 *
 * EDIT:a1 and a2 logic removed in update
 */
static void plltest_rf_pll_en(
	uint32_t enable, uint32_t arg1, uint32_t arg2)
{
	(void)arg1;
	(void)arg2;

	if (enable) {
		REG32(0x7C70) |= 0x40000200u;

		REG32(0x7C74) &= 0xFFFEFFFFu;
		REG32(0x7C74) |= 0x08004100u;

		REG32(0x7C84) |= 0x02000008u;
		REG32(0x7C84) &= 0xBFFF7FFFu;

		/* Both clears are unconditional in the vendor helper. */
		REG32(0x7CE8) &= 0xFFDFFFFFu;
		REG32(0x7CEC) &= 0xDFFFFFFFu;
	} else {
		REG32(0x7C70) &= 0xBFFFFDFFu;

		REG32(0x7C74) &= 0xFFFEFFFFu;
		REG32(0x7C74) &= 0xF7FFBEFFu;

		REG32(0x7C84) &= 0xBDFF7FF7u;

		REG32(0x7CE8) &= 0xFFDFFFFFu;
		REG32(0x7CEC) &= 0xDFFFFFFFu;
		REG32(0x7CEC) &= 0xF7FFFFFFu;
	}
}


/*
 * Reimplementation of rf_txrx_en(a0, a1).
 *
 * This mirrors the actual instruction sequence, including the
 * unconditional clears in 0x7CDC and 0x7CE8.
 */
static void plltest_rf_txrx_en(
	uint32_t param1,
	uint32_t param2)
{
	if (param2 == 0) {
		REG32(0x7D20) &= 0xFFFFFFDFu;
		REG32(0x7CD4) &= 0xFFFFFE00u;
	} else {
		REG32(0x7D20) |= 0x00000020u;

		REG32(0x7CD4) &= 0xFFFFFF7Fu;
		REG32(0x7CD4) |= 0x0000017Fu;
	}

	REG32(0x7CDC) &= 0xFFBFFFFFu;
	REG32(0x7CDC) &= 0xFF7FFFFFu;

	REG32(0x7CE8) &= 0xEFFFFFFFu;
	REG32(0x7CE8) &= 0xDFFFFFFFu;

	if (param1 == 0) {
		REG32(0x7D20) &= 0xFFFFFFFDu;
		REG32(0x7D20) &= 0xFFFFFFBFu;
		REG32(0x7CD4) &= 0xFFFFFDFFu;
	} else {
		REG32(0x7D20) |= 0x00000002u;
		REG32(0x7D20) |= 0x00000040u;
		REG32(0x7CD4) |= 0x00000200u;
	}
}


/*
 * Vendor rf_idle() state:
 *   rf_ldo_en_fc(1, 1, 1)
 *   rf_pll_en(0, 0, 0)
 *   rf_txrx_en(0, 0)
 */
static void plltest_rf_idle(void)
{
	plltest_rf_ldo_en_fc(1, 1, 1);
	plltest_rf_pll_en(0, 0, 0);
	plltest_rf_txrx_en(0, 0);
}


/*
 * Literal field updates based on the rf_filter_init() disassembly.
 *
 * These reproduce the field masks you extracted. The one explicit
 * assumption is PLLTEST_FILTER_MODE, used in place of vendor RAM.
 */
static void plltest_rf_filter_init(void)
{
	REG32(0x7CDC) =
		(0x9F122F3Cu & 0x0F000000u) |
		(REG32(0x7CDC) & 0xFEFFFFFFu);

	REG32(0x7CDC) =
		(0x9F122F3Cu & 0x00000F00u) |
		(REG32(0x7CDC) & 0xFFFFF0FFu);

	REG32(0x7CD4) =
		(0x00015C00u & 0x00003C00u) |
		(REG32(0x7CD4) & 0xFFFFFBFFu);

	REG32(0x7DC8) =
		(0x00000703u & 0x00000F00u) |
		(REG32(0x7DC8) & 0xFFFFF0FFu);

	REG32(0x7D20) =
		(0x00640000u & 0x00700000u) |
		(REG32(0x7D20) & 0xFFEFFFFFu);

	REG32(0x7D20) =
		(0x00640000u & 0x00070000u) |
		(REG32(0x7D20) & 0xFFFEFFFFu);

	REG32(0x7CD4) =
		(0x00015C00u & 0x0001C000u) |
		(REG32(0x7CD4) & 0xFFFFBFFFu);

	REG32(0x7CDC) =
		(0x9F122F3Cu & 0x001C0000u) |
		(REG32(0x7CDC) & 0xFFFBFFFFu);

	REG32(0x7CE8) =
		(0x0803B01Cu & 0x00003800u) |
		(REG32(0x7CE8) & 0xFFFFC7FFu);

	REG32(0x7CE8) =
		(0x0803B01Cu & 0x01C00000u) |
		(REG32(0x7CE8) & 0xFFBFFFFFu);

	REG32(0x7CE8) =
		(0x0803B01Cu & 0x0E000000u) |
		(REG32(0x7CE8) & 0xFDFFFFFFu);

	REG32(0x7CDC) =
		(0x9F122F3Cu & 0x90200080u) |
		(REG32(0x7CDC) & 0x6FDFFF7Fu);

	REG32(0x7CDC) =
		(0x9F122F3Cu & 0x0003F000u) |
		(REG32(0x7CDC) & 0xFFFFEFFFu);

	REG32(0x7CE8) =
		(0x0803B01Cu & 0x0000003Fu) |
		(REG32(0x7CE8) & 0xFFFFFFC0u);

	REG32(0x7CE8) =
		(0x0803B01Cu & 0x00020000u) |
		(REG32(0x7CE8) & 0xFFFDFFFFu);

	REG32(0x7DC8) =
		(0x00000703u & 0x0000F000u) |
		(REG32(0x7DC8) & 0xFFFFEFFFu);

	REG32(0x7CD0) =
		(0x00000040u & 0x00000070u) |
		(REG32(0x7CD0) & 0xFFFFFF8Fu);

	REG32(0x7CE0) |= 0x00000006u;

	REG32(0x7CEC) =
		((PLLTEST_FILTER_MODE & 7u) << 11) |
		(REG32(0x7CEC) & 0xFFFFC7FFu);

	REG32(0x7CDC) &= 0xFFFFFFF0u;

	REG32(0x7CE0) |= 0x00000078u;
}


/*
 * Minimal RF PLL lock experiment.
 *
 * Returns:
 *   1             if bit 23 of 0x7C7C was observed set
 *   before_stop   on timeout (status captured BEFORE trigger clear)
 *
 * g_pll_lock_trace.locked distinguishes a timeout whose captured
 * status happens to equal 1 from a successful lock.
 */
int minimal_pll_lock_test(uint8_t channel)
{
	uint32_t status;
	uint32_t i;
	uint32_t locked = 0;

	/*
	 * Clear the diagnostics so RAM dumps from this run cannot
	 * be confused with values left by a previous attempt.
	 */
	g_pll_lock_trace.status_before_trigger = 0;
	g_pll_lock_trace.status_first = 0;
	g_pll_lock_trace.status_100 = 0;
	g_pll_lock_trace.status_1000 = 0;
	g_pll_lock_trace.status_last_poll = 0;
	g_pll_lock_trace.status_before_stop = 0;
	g_pll_lock_trace.status_after_stop = 0;
	g_pll_lock_trace.status_seen_or = 0;
	g_pll_lock_trace.poll_count = 0;
	g_pll_lock_trace.locked = 0;

	g_pll_lock_trace.reg_7c04 = 0;
	g_pll_lock_trace.reg_7c70 = 0;
	g_pll_lock_trace.reg_7c74 = 0;
	g_pll_lock_trace.reg_7c84 = 0;
	g_pll_lock_trace.reg_7c7c = 0;
	g_pll_lock_trace.reg_7ce0 = 0;
	g_pll_lock_trace.reg_7ce4 = 0;
	g_pll_lock_trace.reg_7cec = 0;
	g_pll_lock_trace.reg_7cf0 = 0;
	g_pll_lock_trace.reg_7d20 = 0;
	g_pll_lock_trace.reg_7dd0 = 0;

	/*
	 * 1. Start the 26 MHz crystal.
	 * These are retained from your original test.
	 */
	REG32(0x03A4) &= 0xFFFFEFFFu;

	{
		uint8_t calib = 0x0A;

		REG32(0x03A4) |=
			((uint32_t)calib << 18) |
			((uint32_t)calib << 12);
	}

	REG32(0x03A4) &= 0xFFFFF7FFu;
	REG32(0x03A4) &= 0xFFFFFFE3u;
	REG32(0x03A4) |= 0x00000004u;

	REG32(0x03A4) |= (1u << 9);
	plltest_delay(1000);

	REG32(0x03A4) |= (1u << 10);
	plltest_delay(5000);

	REG32(0x03A4) |= (0x0Fu << 24);
	plltest_delay(100);

	mark('a');

	/*
	 * 2. Power domains and SPI bridge.
	 */
	REG32(0x03F8) &= 0xFFFFFFFBu;
	plltest_delay(100);
	REG32(0x03F8) |= 0x00000004u;
	REG32(0x03F8) |= 0x00000008u;

	REG32(0x8074) =
		bluetrum_dsp_7x(REG32(0x8074), 0) | 0x000029F8u;

	REG32(0x8070) &= 0xFFFFFF9Fu;

	mark('b');

	/*
	 * 3. Establish the known RF reset baseline BEFORE any
	 *    field-based rf_filter_init() updates.
	 *
	 * These six whole-register values are from rf_reset().
	 */
	REG32(0x7DAC) &= 0xBFFFFFFFu;
	REG32(0x7DD0) |= 0x00000001u;

	REG32(0x7C80) = 0xB8C4EC4Eu;
	REG32(0x7CCC) = 0x0013B13Bu;
	REG32(0x7D20) = 0x0444441Fu;
	REG32(0x7CD0) = 0x00000039u;
	REG32(0x7CD4) = 0x92490003u;
	REG32(0x7CDC) = 0x5511B53Cu;

	/*
	 * The initial rf_idle() call in rf_reset():
	 * keep this RF setup self-contained; do not call the vendor
	 * firmware's helper functions.
	 */
	plltest_rf_idle();

	mark('c');

	/*
	 * rf_reset() clears these 0x7D24 bits before filter init.
	 * rf_txpwr_set() is intentionally omitted because its values
	 * are derived from a vendor RAM table and are not needed to
	 * ask the PLL to lock.
	 */
	REG32(0x7D24) &= 0xFFFBFFFFu;
	REG32(0x7D24) &= 0xFFFFFEFFu;
	REG32(0x7D24) &= 0xFFFFEFFFu;
	REG32(0x7D24) &= 0xFFFEFFFFu;
	REG32(0x7D24) &= 0xFFF7FFFFu;

	plltest_rf_filter_init();

	REG32(0x7CDC) &= 0xDFFFFFFFu;
	REG32(0x7DA8) |= 0x00000001u;

	mark('d');

	/*
	 * 4. RF macrocell setup from rf_reset().
	 */
	REG32(0x7CFC) = 0x021C3434u;
	REG32(0x7D04) = 0x83A0C81Cu;

	REG32(0x7CFC) |= 0x20000000u;

	REG32(0x7D08) &= 0xFFFFFF00u;
	REG32(0x7D08) |= 0x00000008u;

	/*
	 * This is rf_ldo_en_fc(1, 0, 0):
	 * ENABLE branch, variant 0, no initial CE4 toggle.
	 */
	plltest_rf_ldo_en_fc(1, 0, 0);

	/*
	 * rf_pll_en(1, 0, 1).
	 *
	 * In the enable branch arg2=1 clears bit 21 in 0x7CE8.
	 * arg1=0 means this call does NOT conditionally clear
	 * bit 29 in 0x7CEC.
	 */
	plltest_rf_pll_en(1, 0, 1);

	/*
	 * rf_txrx_en(0, 1), matching the reset's RX setup.
	 */
	plltest_rf_txrx_en(0, 1);

	REG32(0x7CE8) |= 0x20000000u;
	REG32(0x7D74) &= 0xFFFEFFFFu;

	mark('e');

	/*
	 * The vendor performs RX DC calibration here and generates
	 * coefficients written to 0x7D80..0x7DA0. That calculation
	 * is not reproduced: its dynamic inputs are unavailable.
	 *
	 * Return the RF macrocell to the same idle helper state
	 * used by rf_reset() after that calibration stage.
	 */
	plltest_rf_idle();

//	REG32(0x7D74) &= 0xFFFFFF00u;
//	REG32(0x7D74) &= 0xFFFFFEFFu;
//
//	REG32(0x7D78) &= 0xFFDFFFFFu;
//	REG32(0x7D78) &= 0xFFEFFFFFu;

	/*
	 * rf_reset() runs rf_filter_init() again after RX calibration.
	 * Repeat the known literal field updates; mode defaults to 0.
	 */
	plltest_rf_filter_init();

	/*
	 * Preserve your reset-tail cleanup. The vendor clears bit 29
	 * of 0x7CFC and clears software control before returning from
	 * reset. The handler enables software control again afterwards.
	 */
	REG32(0x7CFC) &= 0xDFFFFFFFu;
	REG32(0x7DD0) &= 0xFFFFFFFEu;
	REG32(0x7D1C) |= 0x00000010u;

	/*
	 * RF SPI control / macrocell writes from your tested setup.
	 * These are kept separate from the PLL lock decision.
	 */
	REG32(0x8080) =
		(REG32(0x8080) & 0x80FFFFFFu) | 0x0A000000u;

	REG32(0x802C) = 0x000000D4u;

	REG32(0x8080) =
		(REG32(0x8080) & 0x80FFFFFFu) | 0x0D000000u;

	REG32(0x8074) =
		(REG32(0x8074) & 0xFFE0FFFFu) | 0x00020000u;

	REG32(0x81B8) = 0x00000181u;
	rf_spi_tf();

	rf_reg_wr(0x81, REG32(0x81B4) | 0x02u);

	/*
	 * 5. Reproduce the start of rf_pll_afc(channel, 0).
	 *
	 * This operation is explicit in vs_test_ctrl_req_handler:
	 *   rf_sw_ctrl(1)
	 *   rf_pll_afc(channel, 0)
	 *
	 * rf_reset() clears DD0 bit 0, so set it here, not just at
	 * the start of initialization.
	 */
	REG32(0x7DD0) |= 0x00000001u;

	REG32(0x7C04) =
		(REG32(0x7C04) & 0xFFFF00EFu) |
		((uint32_t)channel << 8);

	/*
	 * Exact helper calls in rf_pll_afc(channel, 0).
	 */
	plltest_rf_ldo_en_fc(1, 0, 0);
	plltest_rf_pll_en(1, 0, 0);
	plltest_rf_txrx_en(0, 0);

	/*
	 * Record the state after RF setup but before asserting AFC.
	 * These snapshots help separate a power/configuration issue
	 * from an AFC/status polling issue.
	 */
	g_pll_lock_trace.reg_7c04 = REG32(0x7C04);
	g_pll_lock_trace.reg_7c70 = REG32(0x7C70);
	g_pll_lock_trace.reg_7c74 = REG32(0x7C74);
	g_pll_lock_trace.reg_7c84 = REG32(0x7C84);
	g_pll_lock_trace.reg_7c7c = REG32(0x7C7C);
	g_pll_lock_trace.reg_7ce0 = REG32(0x7CE0);
	g_pll_lock_trace.reg_7ce4 = REG32(0x7CE4);
	g_pll_lock_trace.reg_7cec = REG32(0x7CEC);
	g_pll_lock_trace.reg_7cf0 = REG32(0x7CF0);
	g_pll_lock_trace.reg_7d20 = REG32(0x7D20);
	g_pll_lock_trace.reg_7dd0 = REG32(0x7DD0);

	g_pll_lock_trace.status_before_trigger = REG32(0x7C7C);

	if (pll_snapshot_count == 0) {
		pll_capture_snapshot(&pll_first_snapshot);
		mark('x');
	}
	
	pll_capture_snapshot(&pll_latest_snapshot);
	pll_snapshot_count++;

	if (pll_snapshot_count) {
		if(pll_first_snapshot.c04 != pll_latest_snapshot.c04) { mark('0'); }
		if(pll_first_snapshot.c70 != pll_latest_snapshot.c70) { mark('1'); }
		if(pll_first_snapshot.c74 != pll_latest_snapshot.c74) { mark('2'); }
		if(pll_first_snapshot.c7c != pll_latest_snapshot.c7c) { mark('3'); }
		if(pll_first_snapshot.c84 != pll_latest_snapshot.c84) { mark('4'); }
		if(pll_first_snapshot.ce0 != pll_latest_snapshot.ce0) { mark('5'); }
		if(pll_first_snapshot.ce4 != pll_latest_snapshot.ce4) { mark('6'); }
		if(pll_first_snapshot.cec != pll_latest_snapshot.cec) { mark('7'); }
		if(pll_first_snapshot.cf0 != pll_latest_snapshot.cf0) { mark('8'); }
		if(pll_first_snapshot.d20 != pll_latest_snapshot.d20) { mark('9'); }
		if(pll_first_snapshot.dd0 != pll_latest_snapshot.dd0) { mark('+'); }
	}

	mark('g');

	/*
	 * 6. Trigger AFC and poll for lock.
	 *
	 * Match the order in rf_pll_afc():
	 *   clear 0x7C84 bit 30
	 *   set   0x7C84 bit 15
	 *   poll  0x7C7C bit 23
	 *
	 * The wait is bounded to avoid an indefinite hang.
	 */
	REG32(0x7C84) &= ~PLL_AFC_CLEAR_BIT;

	REG32(0x7C84) |= PLL_AFC_TRIGGER;

	mark('p');

	for (i = 0; i < PLL_LOCK_TIMEOUT; ++i) {
		status = REG32(0x7C7C);

		if (i == 0) {
			g_pll_lock_trace.status_first = status;
		}

		if (i == 100) {
			g_pll_lock_trace.status_100 = status;
		}

		if (i == 1000) {
			g_pll_lock_trace.status_1000 = status;
		}

		g_pll_lock_trace.status_last_poll = status;
		g_pll_lock_trace.status_seen_or |= status;
		g_pll_lock_trace.poll_count = i + 1;

		if ((status & PLL_LOCK_BIT) != 0) {
			locked = 1;
			break;
		}
	}

	/*
	 * Capture the last live status BEFORE clearing the AFC trigger.
	 * Your original timeout path read the status only after stopping
	 * AFC, which may explain the different returned values.
	 */
	g_pll_lock_trace.status_before_stop = REG32(0x7C7C);
	g_pll_lock_trace.locked = locked;

	/*
	 * Clear bit 15 as the vendor does after lock. Do this for both
	 * success and timeout so the test leaves AFC in a bounded state.
	 */
	REG32(0x7C84) &= ~PLL_AFC_TRIGGER;

	g_pll_lock_trace.status_after_stop = REG32(0x7C7C);

	mark(locked ? 'l' : 'f');
	mark('s');

	// int res = g_pll_lock_trace.status_before_stop;
	int res = g_pll_lock_trace.status_seen_or;
	if (locked != 0) {
		return res;
		return 1;
	}

	/*
	 * Same diagnostic return convention as your original function,
	 * but return the pre-stop snapshot instead of post-stop status.
	 * Inspect g_pll_lock_trace for the entire status history.
	 */
	return res;
}


#endif // BLUETRUM_RADIO_H
