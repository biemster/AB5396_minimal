# Bluetrum AB53xx / AB5396 USB Core Architecture & Reference Manual

## 1. Architectural Overview

The USB peripheral on Bluetrum BLE SoCs (such as the AB5396) consists of an industry-standard **Mentor Graphics Inventra MUSB (MHDRC)** controller core wrapped by a **Bluetrum proprietary DMA streaming engine**.

```text
  ┌─────────────────────────────────────────────────────────────┐
  │                 Bluetrum System Bus (AHB)                   │
  └──────────────┬───────────────────────────────┬──────────────┘
                 │                               │
  ┌──────────────▼──────────────┐ ┌──────────────▼──────────────┐
  │ Bluetrum Proprietary DMA    │ │  Mentor Graphics Inventra   │
  │ Streaming Wrapper           │ │  MUSB Core (MHDRC)          │
  │ Registers: 0x300 - 0x32C    │ │  Registers: 0x200 - 0x2BC   │
  │                             │ │                             │
  │ • USBCON0-3 Control         │ │ • UFADDR, UPOWER, UDEVCTL   │
  │ • USBEP0-3ADR DMA Buffers   │ │ • UINTR*, UINDEX            │
  │ • Buffer-to-FIFO Bursting   │ │ • Indexed UCSR / URXCSR     │
  └──────────────┬──────────────┘ └──────────────┬──────────────┘
                 │                               │
                 └──────────────┬────────────────┘
                                │
                  ┌─────────────▼─────────────┐
                  │    Integrated USB PHY     │
                  │    (DP / DM Transceiver)  │
                  └───────────────────────────┘
```

### 1.1 Memory-Mapped Address Spacing
Standard MUSB implementations place registers at consecutive byte offsets (`0x00`, `0x01`, `0x02`...). Bluetrum maps each 8-bit and 16-bit MUSB register onto a **32-bit aligned grid**, multiplying byte offsets by 4:
$$\text{Bluetrum Address} = 0x200 + (\text{MUSB Standard Offset} \times 4)$$

### 1.2 System Bus & Interrupt Architecture
The AB5396 uses a 32-bit RISC-V core with a **proprietary memory-mapped Programmable Interrupt Controller (PIC)** rather than standard RISC-V architectural CSRs (`mstatus`, `mie`):
* `PICCON` (`0x440`): Master interrupt controller configuration.
* `PICEN`  (`0x444`): Individual peripheral interrupt enable bits (`Bit 7 (0x80)` = USB).
* `PICPR`  (`0x448`): Peripheral interrupt priority register.

---

## 2. Register Reference

### 2.1 Bluetrum DMA & PHY Wrapper (`0x300 - 0x32C`)

| Address | Name | Access | Reset | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0x00000300` | `USBCON0` | R/W | `0x20` | USB PHY and Controller Master Control |
| `0x00000304` | `USBCON1` | R/W | `0x00` | USB DMA Master Control (`Bit 16`: DMA Enable) |
| `0x00000308` | `USBCON2` | R/W | `0x00` | DMA Transfer Length & Endpoint Trigger |
| `0x0000030C` | `USBCON3` | R/W | `0x00` | FIFO Routing Configuration (Normal: `0x0F`) |
| `0x00000314` | `USBEP0ADR` | R/W | `0x00` | Endpoint 0 DMA SRAM Buffer Physical Address |
| `0x00000318` | `USBEP1RXADR` | R/W | `0x00` | Endpoint 1 RX DMA SRAM Buffer Physical Address |
| `0x0000031c` | `USBEP1TXADR` | R/W | `0x00` | Endpoint 1 TX DMA SRAM Buffer Physical Address |
| `0x00000320` | `USBEP2RXADR` | R/W | `0x00` | Endpoint 2 RX DMA SRAM Buffer Physical Address |
| `0x00000324` | `USBEP2TXADR` | R/W | `0x00` | Endpoint 2 TX DMA SRAM Buffer Physical Address |
| `0x00000328` | `USBEP3RXADR` | R/W | `0x00` | Endpoint 3 RX DMA SRAM Buffer Physical Address |
| `0x0000032c` | `USBEP3TXADR` | R/W | `0x00` | Endpoint 3 TX DMA SRAM Buffer Physical Address |

#### `USBCON0` (`0x300`) — PHY & Core Power Control
* `Bit 0`: Core clock enable.
* `Bit 1`: Internal PHY enable.
* `Bit 2`: USB D+ pull-up enable (Software connect).
* `Bit 3..6`: PHY tuning and transceiver biasing (`0x78`).
* `Bit 5`: USB Core Reset.

#### `USBCON2` (`0x308`) — DMA Transfer Stride / Trigger
Writing to this register begins an automatic memory-to-FIFO (or FIFO-to-memory) burst transfer.
* `Bits 0..15`: Transfer Length in bytes (`len & 0xFFFF`).
* `Bit 16 (0x00010000)`: Trigger DMA for **Endpoint 0**.
* `Bit 17 (0x00020000)`: Trigger DMA for **Endpoint 1**.
* `Bit 18 (0x00040000)`: Trigger DMA for **Endpoint 2**.
* `Bit 19 (0x00080000)`: Trigger DMA for **Endpoint 3**.

---

### 2.2 Mentor Graphics MUSB Core Registers (`0x200 - 0x2BC`)

#### Common Control Registers

| Address | Name | Access | Description |
| :--- | :--- | :--- | :--- |
| `0x00000200` | `UFADDR` | R/W | Function Address (`Bits 0..6`: Address, `Bit 7`: Active Enable) |
| `0x00000204` | `UPOWER` | R/W | Bus Power / Suspend / Resume status |
| `0x00000208` | `UINTRTX1` | R/C | TX Interrupt Status (`Bit 0`: EP0, `Bit 1`: EP1 TX, `Bit 2`: EP2 TX) |
| `0x0000020C` | `UINTRTX2` | R/C | TX Interrupt Status Upper (Endpoints 8–15) |
| `0x00000210` | `UINTRRX1` | R/C | RX Interrupt Status (`Bit 1`: EP1 RX, `Bit 2`: EP2 RX) |
| `0x00000214` | `UINTRRX2` | R/C | RX Interrupt Status Upper (Endpoints 8–15) |
| `0x00000218` | `UINTRUSB` | R/C | USB Common Bus Interrupts (`Bit 0`: Suspend, `Bit 1`: Resume, `Bit 2`: Reset) |
| `0x0000021C` | `UINTRTX1E` | R/W | TX Interrupt Enable (`Bit 0`: EP0, `Bit 1`: EP1 TX, `Bit 2`: EP2 TX) |
| `0x00000220` | `UINTRTX2E` | R/W | TX Interrupt Enable Upper |
| `0x00000224` | `UINTRRX1E` | R/W | RX Interrupt Enable (`Bit 1`: EP1 RX, `Bit 2`: EP2 RX) |
| `0x00000228` | `UINTRRX2E` | R/W | RX Interrupt Enable Upper |
| `0x0000022C` | `UINTRUSBE` | R/W | USB Common Interrupt Enable (`Bit 2`: Reset Enable) |
| `0x00000230` | `UFRAME1` | RO | USB Frame Number (Low byte) |
| `0x00000234` | `UFRAME2` | RO | USB Frame Number (High byte) |
| `0x00000238` | `UINDEX` | R/W | **Endpoint Window Selector (0–15)** |
| `0x0000023C` | `UDEVCTL` | RO | OTG / Controller Device Status |

---

### 2.3 Indexed Endpoint Register Window (`0x240 - 0x26C`)
The registers at offsets `0x240` through `0x26C` are dynamically mapped to the endpoint number set in **`UINDEX` (`0x238`)**.

```text
UINDEX == 0 (Endpoint 0 Window):
  0x244 -> UCSR0
  0x258 -> UCOUNT0

UINDEX >= 1 (Generic Endpoints 1..15 Window):
  0x240 -> UTXMAXP     0x24C -> URXMAXP
  0x244 -> UTXCSR1     0x250 -> URXCSR1
  0x248 -> UTXCSR2     0x254 -> URXCSR2
  0x258 -> URXCOUNT1   0x25C -> URXCOUNT2
  0x260 -> UTXTYPE     0x268 -> URXTYPE
  0x264 -> UTXINTERVAL 0x26C -> URXINTERVAL
```

#### `UCSR0` (`0x244`, when `UINDEX == 0`) — Endpoint 0 Control/Status
* `Bit 0 (0x01)`: `RxPktRdy` (RO). A packet has arrived in the FIFO.
* `Bit 1 (0x02)`: `TxPktRdy` (W1S). Data is loaded in FIFO, transmit to host.
* `Bit 2 (0x04)`: `SentStall` (RO). Core generated a STALL handshake.
* `Bit 3 (0x08)`: `DataEnd` (W1S). Terminate Data Stage (used with `TxPktRdy` or `ServicedRxPktRdy`).
* `Bit 4 (0x10)`: `SetupEnd` (RO). Premature control transfer completion.
* `Bit 5 (0x20)`: `SendStall` (W1S). Stall the current transaction.
* `Bit 6 (0x40)`: `ServicedRxPktRdy` (W1C). Software acknowledges and clears incoming packet.
* `Bit 7 (0x80)`: `ServicedSetupEnd` (W1C). Clear `SetupEnd` bit.

#### `UTXCSR1` (`0x244`, when `UINDEX >= 1`) — Endpoint TX Control/Status Low
* `Bit 0 (0x01)`: `TxPktRdy`. Set by software when packet is loaded; cleared by hardware when ACKed by host.
* `Bit 1 (0x02)`: `FifoNotEmpty`. At least one packet is pending in TX FIFO.
* `Bit 2 (0x04)`: `UnderRun`. TX underrun error in ISO mode.
* `Bit 3 (0x08)`: `FlushFIFO`. Flush the next packet from the TX FIFO.
* `Bit 4 (0x10)`: `SendStall`. Issue a STALL handshake to IN tokens.
* `Bit 5 (0x20)`: `SentStall`. Set when STALL handshake was issued.
* `Bit 6 (0x40)`: `ClrDataTog`. Reset data toggle to `DATA0`.
* `Bit 7 (0x80)`: `IncompTx`. Incomplete ISO transmission.

#### `UTXCSR2` (`0x248`, when `UINDEX >= 1`) — Endpoint TX Control/Status High
* `Bit 5 (0x20)`: `Mode`. Direction selector (**`1` = Transmit/TX**, `0` = Receive/RX).
* `Bit 6 (0x40)`: `FrcDataTog`. Force data toggle sequence.
* `Bit 7 (0x80)`: `AutoSet`. Automatically assert `TxPktRdy` when max packet size is loaded into FIFO.

#### `URXCSR1` (`0x250`, when `UINDEX >= 1`) — Endpoint RX Control/Status Low
* `Bit 0 (0x01)`: `RxPktRdy`. Set by hardware when packet is received in FIFO. Cleared by software.
* `Bit 1 (0x02)`: `FifoFull`. RX FIFO is full.
* `Bit 2 (0x04)`: `OverRun`. RX FIFO overrun occurred.
* `Bit 3 (0x08)`: `DataError`. CRC or bitstuff error.
* `Bit 4 (0x10)`: `FlushFIFO` / **Bluetrum DMA RX Trigger**. Writing `0x10` re-arms the Bluetrum DMA engine for the next packet.
* `Bit 5 (0x20)`: `SendStall`. Send STALL handshake to OUT tokens.
* `Bit 6 (0x40)`: `SentStall`. Set when STALL handshake was issued.
* `Bit 7 (0x80)`: `ClrDataTog`. Reset data toggle to `DATA0`.

#### `URXCSR2` (`0x254`, when `UINDEX >= 1`) — Endpoint RX Control/Status High
* `Bit 5 (0x20)`: `DisNyet`. Disable NYET responses in High-Speed mode.
* `Bit 6 (0x40)`: `AutoReq`. Automatically generate IN requests in host mode.
* `Bit 7 (0x80)`: `AutoClr`. Automatically clear `RxPktRdy` when packet is completely read by DMA.

#### `UTXMAXP` / `URXMAXP` (`0x240` / `0x24C`, when `UINDEX >= 1`)
Defines the maximum packet payload size in 8-byte increments:
$$\text{Register Value} = \frac{\text{Max Packet Size in Bytes}}{8}$$
*(For 32 bytes $\rightarrow$ `32 >> 3 = 4`; for 8 bytes $\rightarrow$ `8 >> 3 = 1`).*

---

### 2.4 FIFO Access Registers (`0x280 - 0x2BC`)

| Address | Name | Width | Description |
| :--- | :--- | :--- | :--- |
| `0x00000280` | `UFIFO0` | 32-bit | Direct hardware FIFO read/write for Endpoint 0 |
| `0x00000284` | `UFIFO1` | 32-bit | Direct hardware FIFO read/write for Endpoint 1 |
| `0x00000288` | `UFIFO2` | 32-bit | Direct hardware FIFO read/write for Endpoint 2 |
| `0x0000028C` | `UFIFO3` | 32-bit | Direct hardware FIFO read/write for Endpoint 3 |
| `0x00000290`..`0x2BC` | `UFIFO4`..`15` | 32-bit | FIFOs for remaining endpoints (if synthesized) |

---

## 3. Reverse-Engineering Insights & Hardware Quirks

### 3.1 The `UINDEX` Preemption Race Condition
The registers at `0x240..0x26C` are an indexed window controlled by `UINDEX` (`0x238`).
Every ISR entry must save `UINDEX` and restore it upon exit. Main-thread accesses to indexed registers must disable the USB interrupt via `PICEN` to prevent the ISR from swapping the window during execution.

### 3.2 The DMA Bus-Arbitration Constraint & CPU Starvation
The internal AHB bus arbiter on the AB5396 prioritizes the CPU during instruction fetches and within machine-mode interrupt context. This causes two critical failure modes:
1. **EP0 Descriptor Aborts during Enumeration:** If `main()` executes a tight zero-wait loop (`while(1)`) without yielding the bus, the CPU continuously saturates AHB arbitrations. When the host issues an EP0 IN token, the DMA cannot burst descriptor data into `UFIFO0` in time, causing host timeouts and `EPROTO` (`-71`) enumeration failures.
2. **TX Burst Desynchronization:** Asserting `UTXCSR1 = 0x01` (`TxPktRdy`) immediately after triggering `USBCON2` can signal packet readiness before the DMA has physically completed its write burst across the bus.

**The Solution:**
* The event loop in `main()` must yield the bus (using small pacing delays, `wfi`, or a cooperative delay loop).
* Software must insert a tiny execution barrier of ~10–15 CPU cycles (`delay(3)` in the BootROM at `0x00080284`) immediately after asserting `UTXCSR1 = 0x01` to allow the memory pipeline and MUSB latch to settle.

### 3.3 Asymmetric Hardware FIFO Sizing and RAM Banking
Bluetrum allocates 64 bytes total of physical SRAM to the generic endpoint FIFOs. The MUSB core splits this into **two 32-byte Double Packet Buffering (DPB) banks**.

Critically, **RX and TX behave asymmetrically** with respect to the Bluetrum DMA engine:

```text
  RX PATH (Hardware-Driven Banking):
  ──────────────────────────────────
  Packet 1 (Bank 0) ──► Hardware DMA ──► USBEP1RXADR + 0x00
  Packet 2 (Bank 1) ──► Hardware DMA ──► USBEP1RXADR + 0x20
  (Software MUST toggle a pointer: g_rx_bank ^= 1)

  TX PATH (Software-Driven, No Auto-Banking):
  ──────────────────────────────────────────
  Any Packet ────────► Hardware DMA ──► USBEP1TXADR + 0x00
  (DMA always streams strictly from USBEP1TXADR; software must NOT alternate offsets)
```

1. **RX Path (Auto-Offsetting):** The DMA engine automatically routes incoming packets to alternating 32-byte RAM addresses:
   * **Bank 0:** `USBEP1RXADR + 0x00`
   * **Bank 1:** `USBEP1RXADR + 0x20`
   Software **must track `g_rx_bank`** to read incoming data from alternating 32-byte slices.
2. **TX Path (Strict Base Addressing):** The TX DMA controller does *not* auto-bank. It reads directly and strictly from the base physical address programmed into `USBEP1TXADR`. If software attempts to alternate TX payloads between `+0x00` and `+0x20`, the DMA will re-read `+0x00` on every packet, sending even characters twice and dropping odd characters completely (`dd----..//$$ll`).
3. **Payload Limits:** Both directions must cap maximum transfer units at **32 bytes** (`wMaxPacketSize = 32`, `UTXMAXP = 4`, `URXMAXP = 4`) to prevent buffer overruns into adjacent endpoint regions.

---

## 4. Endpoint 0 Control Transfer Engine

### 4.1 State Machine Flow

```
                      ┌────────────────┐
                      │    IDLE        │
                      └───────┬────────┘
                              │ SETUP Token arrives
                              │ RxPktRdy = 1
                              ▼
                      ┌────────────────┐
                      │  DECODE SETUP  │
                      └───────┬────────┘
             ┌────────────────┼────────────────┐
             ▼                ▼                ▼
     [ Control Read ]  [ Control Write ] [ Control Write ]
       (IN Stage)       (No Data Stage)   (OUT Data Stage)
      wLength > 0        wLength == 0       wLength > 0
             │                │                │
             │                │                ▼
             │                │          UCSR0 = 0x40
             │                │          Wait for OUT packet
             │                │          Read data from ep0_buf
             │                │          Drain UFIFO0
             │                │                │
             ▼                ▼                ▼
       Trigger DMA        UCSR0 = 0x48    UCSR0 = 0x48
       USBCON2 = len|0x10K (ServicedRx    (ServicedRx
       UCSR0 = 0x0A        | DataEnd)     | DataEnd)
       (TxPktRdy|DataEnd)     │                │
             │                ▼                ▼
             ▼            [ STATUS: IN ]   [ STATUS: IN ]
       [ STATUS: OUT ]    Hardware sends   Hardware sends
       Hardware sends     0-byte ACK       0-byte ACK
       0-byte ACK             │                │
             │                └────────┬───────┘
             └─────────────────────────┼──────────────────┐
                                       ▼                  ▼
                                ┌──────────────┐   ┌──────────────┐
                                │ SET_ADDRESS? │   │    DONE      │
                                └──────┬───────┘   └──────────────┘
                                       │ Status finishes
                                       ▼
                                UFADDR = addr|0x80
```

### 4.2 Handling Specific Control Requests

#### Standard `GET_DESCRIPTOR`
1. Load response into `ep0_buf`.
2. Write transfer length: `BT_USBCON2 = len | 0x10000;`.
3. Set Control Register: `BT_UCSR0 = 0x0A;` (`TxPktRdy | DataEnd`).

#### Standard `SET_ADDRESS`
1. Store requested address: `g_pending_address = (wValue & 0x7F) | 0x80;`.
2. Acknowledge SETUP phase: `BT_UCSR0 = 0x48;` (`ServicedRxPktRdy | DataEnd`).
3. When the status phase completes (acknowledged by host), MUSB triggers an EP0 TX interrupt. The ISR writes:
   ```c
   BT_UFADDR = g_pending_address;
   g_pending_address = 0;
   ```

#### Standard `GET_STATUS`
Host queries device, interface, or endpoint status (`wLength = 2`):
```c
uint16_t status = 0x0001; /* Bit 0 = Self-Powered */
bt_usb_ep0_tx(&status, 2);
```

#### CDC `SET_LINE_CODING` (Two-Stage OUT Transfer)
1. **Stage 1 (SETUP packet arrives):** Acknowledge SETUP packet without asserting `DataEnd`:
   ```c
   BT_UCSR0 = 0x40; /* ServicedRxPktRdy only */
   g_ep0_stage = EP0_STAGE_DATA_OUT;
   ```
2. **Stage 2 (OUT data packet arrives):** Read 7 bytes from `ep0_buf`, pop `UFIFO0`, then close the transfer:
   ```c
   BT_UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */
   g_ep0_stage = EP0_STAGE_SETUP;
   ```

---

## 5. Non-EP0 Endpoints (Bulk, Interrupt, Isochronous)

### 5.1 Endpoint Types (`UTXTYPE` / `URXTYPE`)
Registers `0x260` (`UTXTYPE`) and `0x268` (`URXTYPE`) set the protocol mode when `UINDEX >= 1`:

| Bits | Field | Settings |
| :--- | :--- | :--- |
| `5..4` | Transfer Type | `00` = Control, `01` = Isochronous, `10` = Bulk, `11` = Interrupt |
| `3..0` | Target Endpoint | Associated endpoint hardware index |

### 5.2 Polling Intervals (`UTXINTERVAL` / `URXINTERVAL`)
Registers `0x264` (`UTXINTERVAL`) and `0x26C` (`URXINTERVAL`) configure the transfer interval:
* **Bulk Endpoints:** Set to `0`.
* **Interrupt Endpoints:** Polling period in frames ($2^{n-1}$ for high-speed, $n$ ms for full-speed).
* **Isochronous Endpoints:** Set to `1` (polled every frame).

---

### 5.3 Endpoint Initialization Examples

#### Configuring Endpoint 1 as Bulk IN / Bulk OUT (CDC Data)
```c
/* Ensure descriptor wMaxPacketSize is set to 0x0020 (32 bytes) */

/* Select EP1 */
BT_UINDEX = 1;

/* Configure Bulk IN (TX) */
BT_UTXMAXP = 4;        /* 32 bytes (32 >> 3) */
BT_UTXCSR1 = 0x48;     /* Clear data toggle, flush FIFO */
BT_UTXCSR2 = 0x20;     /* Mode = TX */
BT_UTXTYPE = 0x21;     /* Bulk transfer (0b10 << 4) | EP1 */
BT_UTXINTERVAL = 0;
BT_USBEP1TXADR = (uint32_t)ep1_tx_buf;

/* Configure Bulk OUT (RX) */
BT_URXMAXP = 4;        /* 32 bytes (32 >> 3) */
BT_URXCSR1 = 0x90;     /* Clear data toggle, flush FIFO */
BT_URXCSR2 = 0x00;     /* Mode = RX */
BT_URXTYPE = 0x21;     /* Bulk transfer (0b10 << 4) | EP1 */
BT_URXINTERVAL = 0;
BT_USBEP1RXADR = (uint32_t)ep1_rx_buf;
```

#### Configuring Endpoint 2 as Interrupt IN (CDC Notifications / HID)
```c
/* Select EP2 */
BT_UINDEX = 2;

/* Configure Interrupt IN (TX) */
BT_UTXMAXP = 8 >> 3;   /* 8 bytes */
BT_UTXCSR1 = 0x48;     /* Clear toggle, flush FIFO */
BT_UTXCSR2 = 0x20;     /* Mode = TX */
BT_UTXTYPE = 0x32;     /* Interrupt transfer (0b11 << 4) | EP2 */
BT_UTXINTERVAL = 16;   /* 16 ms polling period */
BT_USBEP2TXADR = (uint32_t)ep2_tx_buf;
```

#### Isochronous Endpoints (UAC / Audio Streaming)
While the MUSB core supports Isochronous mode, the behavior of Bluetrum's DMA wrapper under ISO packet deadlines is partially unmapped:
1. `UTXTYPE` must be set to `0x11` (ISO mode).
2. For audio OUT streaming, `URXTYPE` is set to `0x11` and `URXINTERVAL = 1`.
3. Double buffering is typically controlled by `URXCSR2` / `UTXCSR2` auto-registers.

---

### 5.4 Data Transmission and Reception

Software tracks the hardware RX toggle globally:
```c
static volatile uint8_t g_rx_bank = 0;
```

#### Transmitting Data (Bulk IN)
TX always writes to base `ep1_tx_buf` (`+0x00`), restricted to a maximum of 32 bytes, followed by the pipeline arbitration delay:

```c
void ep1_tx_kick(void) {
    uint16_t count = 0;

    /* 1. Always fill from base USB buffer address, up to 32 bytes max */
    while ((g_cdc_tx_head != g_cdc_tx_tail) && (count < 32)) {
        ep1_tx_buf[count++] = g_cdc_tx_buf[g_cdc_tx_tail];
        g_cdc_tx_tail = (g_cdc_tx_tail + 1) & (BT_CDC_TX_BUF_SIZE - 1);
    }

    if (count == 0) {
        g_ep1_tx_busy = false;
        return;
    }

    uint32_t pic = BT_PICEN;
    BT_PICEN = pic & ~0x80;
    uint32_t saved_idx = BT_UINDEX;

    BT_UINDEX = 1;

    /* 2. DMA always bursts from base */
    BT_USBEP1TXADR = (uint32_t)ep1_tx_buf;
    BT_USBCON2 = count | 0x20000; 

    /* Wait for DMA burst completion */
    while (BT_USBCON2 & 0x20000); 

    /* 3. Signal packet ready to MUSB core */
    BT_UTXCSR1 = 0x01;

    BT_UINDEX = saved_idx;
    BT_PICEN = pic;

    /* 4. Bus arbitration barrier (matching BootROM delay(3), ~10-15 cycles) */
    for (volatile int d = 0; d < 6; d++);
}
```

#### Receiving Data (Bulk OUT)
RX reads from alternating 32-byte RAM offsets (`+0x00` vs `+0x20`) on every packet:

```c
/* Inside ISR when URXCSR1 & 0x01 (RxPktRdy) is asserted: */
int rx_count = BT_URXCOUNT1;
if (rx_count > 32) rx_count = 32;

/* Pick active hardware bank */
volatile uint8_t *src = (g_rx_bank == 0) ? (ep1_rx_buf + 0x00) 
                                         : (ep1_rx_buf + 0x20);
g_rx_bank ^= 1;

for (int i = 0; i < rx_count; i++) {
    process_byte(src[i]);
}

BT_USBEP1RXADR = (uint32_t)ep1_rx_buf;
BT_URXCSR1 = 0x10; /* Re-arm Bluetrum DMA */
```

---

## 6. Software Architecture Guide

A stable Bluetrum USB implementation must use the **Split-Context Execution Model**:

```
 [ Hardware Events ]
          │
          ▼
   bt_usb_isr()  ──(RAM / .isr)──► Reads Status Flags
                                  Drains Hardware UFIFO0
                                  Latches Setup Packet
                                  Saves & Restores UINDEX
                                  Sets g_ep0_setup_ready
                                  Exits (mret releases bus)
          │
          ▼
   bt_usb_tick() ──(RAM / .usb)──► Runs in Thread Context (main loop)
                                  Evaluates Request Type
                                  Drives EP0 State Transitions
                                  Executes DMA via BT_USBCON2
                                  Consumes / ACKs EP1 Bulk OUT
```

### 6.1 Interrupt Handler Reference Implementation
```c
BT_ISR_FUNC
void bt_usb_isr(void) {
    uint32_t saved_idx = BT_UINDEX;

    uint32_t flags_usb = BT_UINTRUSB; /* Bus Reset */
    uint32_t flags_tx  = BT_UINTRTX1; /* EP0 / TX Flags */
    uint32_t flags_rx  = BT_UINTRRX1; /* RX Flags */

    /* Handle Bus Reset */
    if (flags_usb & 0x04) {
        BT_UFADDR = 0x80;
        BT_UINDEX = 0;
        BT_UCSR0  = 0x48;

        g_rx_bank = 0; /* Hardware FIFO banks reset to Bank 0 */
        g_ep1_tx_busy = false;

        BT_UINTRUSB = flags_usb;
        BT_UINDEX = saved_idx;
        return;
    }

    /* EP1 RX Packet Ready */
    if (flags_rx & 0x02) {
        BT_UINDEX = 1;
        if (BT_URXCSR1 & 0x01) {
            int rx_count = BT_URXCOUNT1;
            if (rx_count > 32) rx_count = 32;

            volatile uint8_t *src = (g_rx_bank == 0) ? (ep1_rx_buf + 0x00) 
                                                     : (ep1_rx_buf + 0x20);
            g_rx_bank ^= 1;

            for (int i = 0; i < rx_count; i++) {
                uint16_t next = (g_cdc_rx_head + 1) & (BT_CDC_RX_BUF_SIZE - 1);
                if (next != g_cdc_rx_tail) {
                    g_cdc_rx_buf[g_cdc_rx_head] = src[i];
                    g_cdc_rx_head = next;
                }
            }

            BT_USBEP1RXADR = (uint32_t)ep1_rx_buf;
            BT_URXCSR1 = 0x10; /* Re-arm */
        }
        BT_UINTRRX1 = flags_rx;
    }

    /* EP1 TX Packet Completion */
    if (flags_tx & 0x02) {
        ep1_tx_kick();
        BT_UINTRTX1 = flags_tx;
    }

    BT_UINDEX = saved_idx;
}
```

*(Note: `g_rx_bank = 0;` must also be executed inside the EP0 `SET_CONFIGURATION` handler to stay synchronized with the core's data toggle/bank reset).*
