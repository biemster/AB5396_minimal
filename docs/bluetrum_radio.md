# Bluetrum AB5682 / AB5396 Radio Peripheral Reference Manual
**Document Version:** 1.0 (Reverse Engineering Draft)
**Architecture:** RISC-V (with proprietary extensions)
**Target Silicon:** Bluetrum AB5682, AB5396

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
#define RF_SPI_DAT  *(volatile uint32_t*)0x81B4
#define RF_SPI_CMD  *(volatile uint32_t*)0x81B8

void rf_spi_tf(void) {
    RF_SPI_CTRL |= 0x01;
    while ((RF_SPI_CTRL & 0x02) == 0); // Wait for transfer complete
}

uint32_t rf_reg_rd(uint8_t reg_addr) {
    RF_SPI_CMD = reg_addr | 0x100;
    rf_spi_tf();
    return RF_SPI_DAT;
}

void rf_reg_wr(uint8_t reg_addr, uint32_t value) {
    RF_SPI_DAT = value;
    RF_SPI_CMD = reg_addr | 0x300;
    rf_spi_tf();
}

void rf_hal_init(void) {
    *(volatile uint32_t*)0x8074 |= 0x1B94; // Enable clocks
    RF_SPI_CTRL &= ~0x60;                  // Clear reset flags
    rf_reg_wr(0xAA, 0x03);                 // LNA/Calibration patch
    rf_reg_wr(0x81, rf_reg_rd(0x81) | 0x02); // Enable RF Subsystem
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

## 5. Exchange Memory (EM) & Data Structures
The MAC reads descriptors from a dedicated SRAM block at base `0x10000`. Control Structures (CS) are placed at specific offsets.\
The CS blocks (CS0, CS1, CS2) are the payloads that describe what to do (channel, AA, CRC).\
The Event Descriptors (allocated by the BootROM at 0x10400) describe when to do it.\

An Event Descriptor typically contains:
1. A pointer to the next Event Descriptor.
2. The BB_CLK target timestamp (when to fire).
3. A pointer to the associated Control Structure (e.g., CS0 or CS2).
4. Status/IRQ routing bits.

### 5.1 Control Structure Offsets
The CEVA RW MAC expects multiple connection/advertising state structures:
* **CS Index 0 (`0x10E40`):** Suggested as standard Advertising CS.
* **CS Index 1 (`0x10E8E`):** Suggested as connection state CS (Offset is `0x4E`).
* **CS Index 2 (`0x10EDC`):** Direct Test Mode (DTM) / Raw TX CS. Used by `mgr_test_mode_start_tx`.

### 5.2 Link Layer EM Block Layout (CS Index 2 / `0x10EDC`)
```c
struct ll_em_block {
    uint16_t state;             // +0x00 (Set to 0x001C)
    uint32_t access_addr;       // +0x02 (BLE AA. e.g., 0x71764129 for Test, 0x8E89BED6 for Adv)
    uint32_t crc_init;          // +0x06 (CRC Init. e.g., 0x00555555)
    uint16_t rf_channel;        // +0x0A (Physical PLL index: 37=2402MHz, 38=2426MHz, 39=2480MHz)
    uint32_t tx_dma_ptr;        // +0x0C (Payload pointer. Hardware REQUIRES Bit 31 set: 0x80000000 | addr)
    uint16_t config_word_1;     // +0x10 (Set to 0x064A)
    uint32_t padding;           // +0x12
    uint16_t config_word_2;     // +0x16 (Set to 0x0960)
} __attribute__((packed)); // Warning: Use byte-wise or strict-offset writes to prevent padding issues
```

### 5.3 Auxiliary EM Configurations (Discovered in `ble_tx_test_do`)
* **TX Header Descriptors (`0x1124C`):** The hardware does *not* read the BLE packet header from the payload DMA pointer. The length and PDU type must be written to `0x1124C` as `(len << 8) | (pdu_type)`.
* **Inter-Frame Gap Timers (`0x10D70` - `0x10D8C`):** Array of timers used to handle auto-repeating bursts in test modes.

*Note: These are RAM address the BootROM uses, it's not known if the RF peripheral is hardcoded to use these or it's runtime configured.*

---

## 6. ROM and Vendor Function Reference

### 6.1 Vendor Library (`libbtstack.a`)
* `rf_hal_init` (`0x1783c`): Bootstraps the SPI interface and powers up the analog MAC.
* `rf_spi_tf` (`0x176b0`), `rf_reg_rd`, `rf_reg_wr`: Control plane access layer.
* `ble_tx_test_do` (`0x4f2a2`): Crucial function that revealed the EM layout, Aux Header registers (`0x1124C`), and the `0xF000` TX Enable (Bit 18) and Task Start (Bit 12) triggers.
* `mgr_test_mode_start_tx` (`0x4e556`): Maps logical BLE channels (0-39) to physical PLL index (e.g., Logical 39 -> Phys 0x27).
* `ble_isr` (`0x18a20`): Interrupt dispatcher. Acknowledges `0xF010` statuses.

### 6.2 BootROM (`0x80000` - `0x90000`)
* `FUN_00088cf6` (`0x88cf6`): Memory Allocator. Assigns Exchange Memory blocks.
* `FUN_00088dfc` (`0x88dfc`): Memory Free. Walks the linked list and marks blocks as `0xf00f` (freed).
* `FUN_00089944` (`0x89944`): CEVA RW Event Scheduler. Inserts allocated EM blocks into the linked list at `0x10400` sorted by target execution time.
* `FUN_0008a05a` (`0x8a05a`): Hardware Arming sequence. Called from ISR to pop events off the list and arm the target timer registers.

---

## 7. Experimental Findings & Errata

### 7.1 Failed Test: `0xF02C` Pointer Corruption
**Attempt:** Writing `EM_BASE_ADDR` directly to `BB_DMA_PTR` (`0xF02C`) to force a transmission.
**Result:** Complete MAC lockup. No interrupts fired. 
**Conclusion:** `0xF02C` is the Global Queue Pointer for the CEVA RivieraWaves architecture. Overwriting it destroys the linked-list state. The hardware inherently knows the locations of CS0, CS1, and CS2. Setting the "Start Task" bit triggers the hardware to fetch from the known CS2 (`0x10EDC`) offset automatically.

### 7.2 Failed Test: Native Clock Value Confusion
**Attempt:** Subtracting tick values of `BB_CLK` (`0xF020`) resulted in `4294966958` across 10,000 CPU cycles.
**Result:** Deemed an error until recognized as two's complement `-338`.
**Conclusion:** The Baseband Native Clock is a 28-bit **Down-Counter**. Masking with `0x0FFFFFFF` before subtraction yields correct positive tick advances (~1 tick per microsecond).

### 7.3 Failed Test: Infinite Loop on ISR Polling
**Attempt:** Polling `(BB_INT_STAT & 0x02) == 0` for TX Complete.
**Result:** CPU hung indefinitely.
**Conclusion:** Continuous TX test modes (like the one instantiated by `ble_tx_test_do`) use the IFG timers to continuously auto-repeat. The TX Complete interrupt is never flagged for individual packets in this mode. Bare-metal drivers must use a CPU-cycle timeout and manually abort the MAC (`BB_CTRL &= ~(1 << 18)`).

### 7.4 Failed Test: Embedded Payload Headers
**Attempt:** Placing the BLE PDU Type and Length bytes inside the buffer pointed to by `tx_dma_ptr`.
**Result:** Packets dropped over the air.
**Conclusion:** The payload buffer must contain **only** payload data. The MAC hardware prepends the Link Layer header by fetching it from the auxiliary register `0x1124C`.

---

## 8. Bare-Metal Transmission Implementation Proposal
By hijacking the Test Mode Control Structure (CS2 at `0x10EDC`), packets can be generated without invoking the vendor BootROM scheduler. 

*Prerequisites:*
Initialize Baseband MAC registers `0xF024` and `0xF028`.

*Transmission Sequence:*
1. Write Payload Length and PDU Type to `0x1124C`.
2. Configure CS2 (`0x10EDC`) with AA (`0x8E89BED6`), CRC, and Channel.
3. Write payload (excluding header) to `TX_BUF_ADDR`.
4. Enable MAC Global TX: `BB_CTRL |= 0x40000`.
5. Start Task + Enable Whitening: `BB_TASK_STRT |= 0x3000`.
6. Delay ~10ms for burst transmission.
7. Disable MAC Global TX and clear interrupts.
