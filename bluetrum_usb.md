# Bluetrum AB53xx / AB5396 USB Core Architecture & Reference Manual

## 1. Architectural Overview

The USB peripheral on Bluetrum BLE SoCs (such as the AB5396) consists of an industry-standard **Mentor Graphics Inventra MUSB (MHDRC)** controller core wrapped by a **Bluetrum proprietary DMA streaming engine**.

```
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

```
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
*(e.g., 64 bytes $\rightarrow$ `64 >> 3 = 8`; 8 bytes $\rightarrow$ `8 >> 3 = 1`).*

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
* If `main()` sets `UINDEX = 1` to transmit on EP1, and an interrupt preempts it to service EP0, the ISR writes `UINDEX = 0`.
* When the ISR returns, `main()` continues, writing to `0x244` under the assumption that it is addressing `UTXCSR1`. It actually overwrites `UCSR0`, corrupting EP0 control and dropping EP1 frames.

**The Rule:**
1. Every ISR entry must save `UINDEX` and restore it upon exit.
2. Main-thread accesses to indexed registers must disable the USB interrupt via `BT_PICEN`:
```c
uint32_t pic = BT_PICEN;
BT_PICEN = pic & ~0x80; /* Disable USB IRQ */
uint32_t saved = BT_UINDEX;

BT_UINDEX = target_ep;
/* ... execute endpoint register operations ... */

BT_UINDEX = saved;
BT_PICEN = pic;         /* Restore USB IRQ */
```

---

### 3.2 The DMA Bus-Arbitration Constraint (Why Pure ISR Mode Fails)
If software attempts to complete a control transfer by calling `bt_usb_ep0_tx()` inside `bt_usb_isr()`, the host aborts with:
```
usb 1-4: device descriptor read/64, error -71
```

**Root Cause:**
1. The AB5396 bus matrix locks priority to the CPU while executing within machine-mode interrupt context.
2. When `BT_USBCON2 = len | 0x10000;` triggers the DMA engine, the DMA cannot burst data from `ep0_buf` into `UFIFO0` while the CPU is still holding the bus.
3. The CPU immediately executes `BT_UCSR0 = 0x0A;` (`TxPktRdy`), signaling the MUSB transceiver that valid data is in the FIFO.
4. The host issues an IN token, reads an empty or incomplete FIFO, and flags an `EPROTO` (`-71`) protocol error.

**The Solution (Hybrid Architecture):**
The ISR latches SETUP packets and drains `UFIFO0`, then marks a flag (`g_ep0_setup_ready`). The transfer is initiated in thread context (`bt_usb_tick()`) after `mret` releases bus mastery.

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
/* Select EP1 */
BT_UINDEX = 1;

/* Configure Bulk IN (TX) */
BT_UTXMAXP = 64 >> 3;  /* 64 bytes */
BT_UTXCSR1 = 0x48;     /* Clear data toggle, flush FIFO */
BT_UTXCSR2 = 0x20;     /* Mode = TX */
BT_UTXTYPE = 0x21;     /* Bulk transfer (0b10 << 4) | EP1 */
BT_UTXINTERVAL = 0;
BT_USBEP1TXADR = (uint32_t)ep1_tx_buf;

/* Configure Bulk OUT (RX) */
BT_URXMAXP = 64 >> 3;  /* 64 bytes */
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

#### Transmitting Data (Bulk / Interrupt IN)
```c
void ep_tx(uint8_t ep_num, const void *data, uint16_t len) {
    uint32_t pic = BT_PICEN;
    BT_PICEN = pic & ~0x80; /* Disable USB IRQ to avoid UINDEX corruption */
    uint32_t saved = BT_UINDEX;

    BT_UINDEX = ep_num;

    /* Wait for previous packet completion (TxPktRdy clears when ACKed) */
    while (BT_UTXCSR1 & 0x01);

    memcpy(tx_buffer, data, len);

    /* Point DMA and trigger transfer: Bit (16 + ep_num) */
    BT_USBEP1TXADR = (uint32_t)tx_buffer;
    BT_USBCON2 = len | (1 << (16 + ep_num));

    /* Mark packet ready */
    BT_UTXCSR1 = 0x01;

    BT_UINDEX = saved;
    BT_PICEN = pic;
}
```

#### Receiving Data (Bulk OUT)
```c
int ep_rx(uint8_t ep_num, void *dest, uint16_t max_len) {
    int received = 0;
    uint32_t pic = BT_PICEN;
    BT_PICEN = pic & ~0x80;
    uint32_t saved = BT_UINDEX;

    BT_UINDEX = ep_num;

    if (BT_URXCSR1 & 0x01) { /* RxPktRdy asserted */
        uint16_t count = (BT_URXCOUNT1 & 0xFF) | ((BT_URXCOUNT2 & 0x07) << 8);
        if (count > max_len) count = max_len;

        memcpy(dest, rx_buffer, count);
        received = count;

        /* Drain hardware FIFO */
        while (count > 0) {
            (void)BT_UFIFO1;
            count--;
        }

        /* Re-arm DMA and clear RxPktRdy */
        BT_USBEP1RXADR = (uint32_t)rx_buffer;
        BT_URXCSR1 = 0x10; /* Writing 0x10 re-arms Bluetrum RX DMA */
    }

    BT_UINDEX = saved;
    BT_PICEN = pic;
    return received;
}
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
        BT_UCSR0 = 0x48;
        g_pending_address = 0;
        g_ep0_stage = EP0_STAGE_SETUP;
        BT_UINTRUSB = flags_usb;
        BT_UINDEX = saved_idx;
        return;
    }

    /* Handle Endpoint 0 Interrupt */
    if (flags_tx & 0x01) {
        if (g_pending_address) {
            BT_UFADDR = g_pending_address;
            g_pending_address = 0;
        }

        BT_UINDEX = 0;
        uint32_t csr0 = BT_UCSR0;

        if (csr0 & 0x01) { /* RxPktRdy */
            int count = BT_UCOUNT0;

            if (g_ep0_stage == EP0_STAGE_DATA_OUT) {
                if (count > 0) {
                    memcpy(&g_line_coding, ep0_buf, count > 7 ? 7 : count);
                }
                g_ep0_data_ready = true;
            } else {
                if (count >= 8) {
                    memcpy(&g_setup_pkt, ep0_buf, 8);
                    g_ep0_setup_ready = true;
                }
            }

            /* Drain UFIFO0 */
            while (count > 0) {
                (void)BT_UFIFO0;
                count--;
            }
        }
        BT_UINTRTX1 = flags_tx;
    }

    if (flags_rx) {
        BT_UINTRRX1 = flags_rx;
    }

    BT_UINDEX = saved_idx;
}
```

### 6.2 Main Loop Worker Reference Implementation
```c
BT_USB_FUNC
void bt_usb_tick(void) {
    /* 1. EP1 OUT Drain (Prevents screen/terminal exit lockups) */
    if (g_usb_configured) {
        uint32_t pic = BT_PICEN;
        BT_PICEN = pic & ~0x80;
        uint32_t saved_idx = BT_UINDEX;

        BT_UINDEX = 1;
        if (BT_URXCSR1 & 0x01) {
            int rx_count = BT_URXCOUNT1;
            while (rx_count > 0) {
                (void)BT_UFIFO1;
                rx_count--;
            }
            BT_USBEP1RXADR = (uint32_t)ep1_rx_buf;
            BT_URXCSR1 = 0x10;
        }

        BT_UINDEX = saved_idx;
        BT_PICEN = pic;
    }

    /* 2. Control OUT Data Stage Handling */
    if (g_ep0_data_ready) {
        g_ep0_data_ready = false;
        if (g_ep0_stage == EP0_STAGE_DATA_OUT) {
            BT_UINDEX = 0;
            BT_UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */
            g_ep0_stage = EP0_STAGE_SETUP;
        }
        return;
    }

    /* 3. Fast Exit if No Setup Packet Arrived */
    if (!g_ep0_setup_ready) return;
    g_ep0_setup_ready = false;

    /* Execute standard enumeration decode (GET_DESCRIPTOR, SET_ADDRESS, etc.) */
    decode_and_respond(&g_setup_pkt);
}
```
