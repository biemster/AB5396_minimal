# Bluetrum AB5682 / AB5396 Radio Peripheral Reference Manual
**Document Version:** 1.0 (Reverse Engineering Draft)
**Architecture:** RISC-V (with proprietary extensions)
**Target Silicon:** Bluetrum AB5396

---

## 1. Architecture Overview
The Bluetrum Bluetooth subsystem is divided into two distinct, physically separated domains that communicate with the main RISC-V core:
1. **The Control Plane (Analog RF Frontend):** Configured via an internal SPI bus mapped at `0x8000`. Handles PLL tuning, LNA gain, filters, and analog power sequencing.
2. **The Data Plane (Baseband/MAC):** Configured via SFRs mapped at `0xF000` and an Exchange Memory (EM) block in SRAM at `0x10000`. The digital logic heavily relies on **CEVA RivieraWaves (RW) BLE MAC IP**.

*Note: Due to proprietary toolchain mismatches (stack alignment ABI) and missing object dependencies, linking the vendor's `libbtstack.a` is unfeasible. This document outlines the bare-metal control of the peripheral.*

---

## 2. Instruction Set Architecture Anomalies
When analyzing BootROM or vendor libraries, standard disassemblers (e.g., Ghidra) will fail due to undocumented custom RISC-V instructions used heavily in the radio drivers:
* **Custom Bit Manipulation (`OP` 0x33, `funct7` 0x61):** Uses the 5 bits of `rs2` as an immediate bit index. `funct3=4` is Bit Set, `funct3=3` is Bit Clear. Used extensively for `0xF000` SFR toggling.
* **Custom Unaligned Load/Store:** `opcode` 0x03 (`funct3=7`) for Unaligned LBU, and `opcode` 0x23 (`funct3=6` or `4`) for Unaligned SB/SW.

```c
// Tests: bluetrum a5, a5, a2 (funct7=0x61, funct3=4) -> Suspected BIT SET
uint32_t test_bluetrum_op_4(uint32_t val, uint32_t bit_idx) {
    uint32_t res;
    __asm__ volatile (
        ".insn r 0x33, 4, 0x61, %0, %1, %2"
        : "=r" (res) : "r" (val), "r" (bit_idx)
    );
    return res;
}

// Tests: bluetrum a5, a5, a2 (funct7=0x61, funct3=3) -> Suspected BIT CLEAR
uint32_t test_bluetrum_op_3(uint32_t val, uint32_t bit_idx) {
    uint32_t res;
    __asm__ volatile (
        ".insn r 0x33, 3, 0x61, %0, %1, %2"
        : "=r" (res) : "r" (val), "r" (bit_idx)
    );
    return res;
}

// If this prints 0x00000020, it is a Bit Set!
print_string("OP 4 (Set): ");
print_hex32( test_bluetrum_op_4(0x00000000, 5) );
print_string("\r\n");

// If this prints 0xFFFFFFDF, it is a Bit Clear!
print_string("OP 3 (Clr): ");
print_hex32( test_bluetrum_op_3(0xFFFFFFFF, 5) );
print_string("\r\n");
```

gives the following output:
```
OP 4 (Set): 00004000
OP 3 (Clr): ffffbfff
```

---

## 3. The Control Plane (Internal SPI)
The analog RF die is initialized mostly by the BootROM. Software interacts with it to apply post-boot calibration patches and to enable/disable the transmitter subsystem.

### 3.1 SPI Register Map (Base: `0x8000`)
| Address | Name | Description |
| :--- | :--- | :--- |
| `0x8070` | `RF_SPI_CTRL` | Bit 0: Start Transfer. Bit 1: Transfer Complete (Hardware clears when done). Bits 5-6: Reset/Busy flags. |
| `0x8074` | `RF_SPI_CFG` | Digital interface clocks/power. Initialized with `\|= 0x1B94`. |
| `0x81B4` | `RF_SPI_DAT` | 32-bit Data I/O. Written before a Write command, read after a Read command. |
| `0x81B8` | `RF_SPI_CMD` | 32-bit Command. Encodes the Register Address and R/W flags. |

### 3.2 SPI Command Encoding
Commands are issued to `0x81B8` by ORing the target RF register address with a command flag:
* **Read Flag:** `0x100` (Bit 8)
* **Write Flag:** `0x300` (Bits 8 & 9)

### 3.3 Verified Control Plane Driver
The following driver has been successfully tested on hardware to interact with the RF analog die:
```c
#define RF_SPI_CTRL *(volatile uint32_t*)0x8070
#define RF_SPI_CFG  *(volatile uint32_t*)0x8074
#define RF_SPI_DAT  *(volatile uint32_t*)0x81B4
#define RF_SPI_CMD  *(volatile uint32_t*)0x81B8

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
```

---

## 4. The Data Plane (Baseband MAC)
The Baseband (BB) engine operates at `0xF000` and utilizes CEVA RivieraWaves architecture. The hardware is largely autonomous; software schedules events via linked lists, and the hardware fires when the Native Clock matches the event target time.

### 4.1 Baseband Register Map (Base: `0xF000`)
| Address | Name | Description |
| :--- | :--- | :--- |
| `0xF000` | `BB_CTRL` | Baseband Control. Bit 18 (`0x40000`) = Global TX/MAC Enable. |
| `0xF00C` | `BB_INIT_1` | Initialization register. BootROM sets to `0x13A`. |
| `0xF010` | `BB_INT_STAT`| Interrupt Status. Bit 1=TX/RX Done, Bit 3=Timeout, Bit 4=Tick, Bit 5=Error. |
| `0xF018` | `BB_INT_CLR` | Interrupt Acknowledge. Write 1 to clear corresponding bit in `0xF010`. |
| `0xF020` | `BB_CLK` | Native Bluetooth Clock. **28-bit Down-Counter.** Ticks at ~1MHz. |
| `0xF024` | `BB_MAC_L` | Lower 32 bits of the Device MAC Address (BD_ADDR). |
| `0xF028` | `BB_MAC_H` | Upper 16 bits of the Device MAC Address. |
| `0xF02C` | `BB_GLOBAL_PTR`| RivieraWaves Global DMA/List pointer. *(Do not overwrite during active transmission!)* |
| `0xF060` | `BB_ERR_CODE`| Holds hardware error status when Interrupt Bit 5 fires. |
| `0xF0E0` | `BB_TASK_STRT`| Task Trigger/Config. Bit 12 = Start Task. Bit 13 = Whitening/Test Config. |

---
