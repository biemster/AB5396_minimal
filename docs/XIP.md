# Bluetrum AB5396B: XIP Cache & BootROM Architecture Reference

This document provides a comprehensive blueprint for reverse-engineering and implementing bare-metal XIP (Execute-In-Place) on Bluetrum RISC-V microcontrollers. Unlike standard chips with transparent SPI mapping, Bluetrum utilizes a **software-assisted hardware cache**. Accessing the XIP window triggers a hardware exception, forcing the CPU to run an interrupt handler (either in ROM or a custom one) to manually fill the cache line via SPI DMA.

## 1. Core Architecture & Memory Map

*   **`0x10000000 - 0x1007FFFF`**: CPU XIP Virtual Memory Window.
*   **`0x00060000 - 0x00067FFF`**: Cache SRAM Backing Memory (32 KiB total on AB5396B).
*   **`0x00010000 - 0x0001DFFF`**: Base of standard RAM (56 KiB total).
*   **`0x00080000 - 0x0008FFFF`**: Base of Mask ROM (BootROM).

## 2. Non-Standard RISC-V Deviations (CRITICAL)

The Bluetrum RISC-V core is heavily modified for embedded use and removes several standard RISC-V features. **Attempting to use standard RISC-V paradigms will cause fatal Illegal Instruction exceptions.**

1.  **No Standard CSRs**: Registers like `mstatus`, `mie`, `mtvec`, and `pmpcfg` do *not* exist in silicon. You cannot use `csrw`, `csrs`, or `csrr`. Interrupts and exceptions are managed entirely via proprietary Memory-Mapped I/O (MMIO) registers (the `PIC` block at `0x440`).
2.  **No Hardware Fences**: The CPU does not implement the `Zifencei` extension. Executing `__asm__ volatile ("fence.i")` or `("fence")` will crash the chip. Cache flushing is performed manually by clearing the cache controller's MMIO registers.

## 3. Hardware Registers & Cache Controller

### XIP Controller (`0x54000` base)
Controls the SPI flash interface parameters for XIP mode.
*   `0x54000 (XIP_CMD)`: SPI command used for reads (e.g., `0x342` for standard dual/quad read, derived from eFuses).
*   `0x54004 (XIP_CTRL)`: Enable bit (set to `1`).
*   `0x54010 (XIP_BASE) / 0x54014 (XIP_LIMIT)`: Configures the addressable window (0/0 uses defaults).

### System & Interrupt Control
*   `0x3B8 (MEMCON)`: Bit 16 (`0x10000`) maps the SPI flash into the CPU address space.
*   `0x408 (NMICON)`: NMI Control. Must be set to `1` to enable high-priority exceptions.
*   `0x440 (PICCON)`: Programmable Interrupt Controller state.
*   `0x448 (PICPR)`: PIC Priority. Bit 0 must be `1`.
*   `0x478 (EPICCON)`: Exception PIC Control (restored on ISR exit).

### Cache Controller (`0x454` base)
*   `0x454 (CACHCON0)`: Master Cache Control (initialized with `0x10001`).
*   `0x458 (CACHCON1)`: Cache SRAM Arbiter / Command Register.
*   `0x45C (ICTAG)`: Cache Line Tag & Valid Register.
*   `0x460 (ICINDEX)`: Target Cache Line Index (0 - 63).
*   `0x464 (ICADRMS)`: **Cache Miss Address.** Contains the exact CPU address that faulted.

## 4. Cache Geometry & Direct-Mapped Aliasing

*   **Line Size**: 512 bytes (`0x200`). (Address shift = 9).
*   **Cache Size**: 64 lines (32 KiB). (Index mask = `0x3F`).
*   **Valid Bit**: Bit 16 (`0x10000`) of `ICTAG`.

**The Aliasing / Thrashing Limitation:**
The cache is strictly **Direct-Mapped**. The hardware index is physically hardwired to `(Address >> 9) & 0x3F`. 
If the CPU executes an instruction from Page 0 (Slot 0), and that instruction tries to read data from Page 64 (also Slot 0), the chip will deadlock in an infinite loop of Cache Misses because the slot cannot hold code and data simultaneously.

*Solution:* You must separate code and data.
1.  **Compiler/Linker:** Place `*(.rodata)` and `*(.srodata)` in RAM (alongside `.data`). This guarantees XIP is only used for fetching instructions.
2.  **Startup Code:** When copying `.data` from XIP to RAM, the copy loop must be executed from a **RAM Trampoline** (e.g., copied to the Stack) so the instruction fetch doesn't collide with the XIP data read.

## 5. Required Compiler & Linker Flags

To safely compile heavy C-standard libraries (like `newlib-nano`) without crashing the custom memory model, use these `CFLAGS` and `LDFLAGS`:
*   `-mstrict-align`: Prevents hardware faults on unaligned `memset`/`strcpy`.
*   `-mcmodel=medany`: Forces PC-relative addressing, preventing the linker from assuming flash and RAM are close together.
*   `-msmall-data-limit=0`: Prevents the compiler from using the Global Pointer (`gp`) to incorrectly access XIP constants from RAM.
*   `-ffunction-sections -fdata-sections` & `-Wl,--gc-sections`: Strips unused code to keep the binary small.

## 6. Hardware LFSR Scrambler & DRM Bypass

Bluetrum implements firmware DRM via an on-the-fly hardware LFSR inside the SPI DMA engine. The BootROM provides a low-level DMA primitive at `0x8964c`.
**The Bypass:** By passing `lfsr_cfg = 0` and `lfsr_en = 0` to the DMA engine, we bypass the decryption DRM entirely, allowing standard raw GCC binaries to execute directly from flash.

## 7. Identified BootROM Functions

Identifying these functions is key to building a lightweight `stage1` bootloader:

| Address | Function Signature | Role & Deduced Functionality |
| :--- | :--- | :--- |
| **`0x84000`** | `void rom_system_init(void)` | **Core Init.** Initializes CPU vectors (`PICADR = 0x80000`). Must run first. |
| **`0x80b94`** | `void rom_clock_init(void)` | **PLL & Clock Init.** Switches CPU to high-speed PLL and fixes peripheral dividers. Required before UART init. |
| **`0x8402c`** | `void install_paging_metadata(void *b)` | Writes `0x10001` to `CACHCON0`. |
| **`0x84024`** | `void install_default_callbacks(void)` | Hardcodes `0x86738` into `0x10044` (ISR7 callback). |
| **`0x8964c`** | `uint32_t dma(dest, flash, size, cfg, en)` | **Low-level DMA Primitive.** Programs SPI/DMA to read flash. |

## 8. Stage-1 Bootloader Configuration

To replace the proprietary vendor bootloader, `stage1` runs from RAM, configures the hardware cache using ROM functions to save space, and jumps to `main.bin`.

```c
void setup_xip_and_boot(void) {
    /* 1. Hijack the ROM exception callback pointer (ISR7) */
    REG32(0x00010044) = (uint32_t)(uintptr_t)&xip_callback_stage1;

    /* 2. Configure the SPI Flash interface */
    XIP_CMD   = 0x00000342; 
    XIP_CTRL  = 1;
    XIP_BASE  = 0;
    XIP_LIMIT = 0;

    /* 3. Reset the Cache Controller Hardware */
    CACHCON0 = 0x00010001; 
    CACHCON1 = 0;
    ICTAG    = 0;
    ICINDEX  = 0;
    ICADRMS  = 0;
    ICLOCK   = 0;

    /* 4. Enable Proprietary Interrupt Controller */
    NMICON = 1;          
    PICPR |= 1;         

    /* 5. Map the SPI Flash into the CPU bus */
    MEMCON |= 0x00010000; 

    /* 6. Execute the main application from XIP */
    typedef void (*main_entry_t)(void);
    main_entry_t main_app = (main_entry_t)0x10000000;
    main_app();
}

__attribute__((noinline))
static uint32_t xip_callback_stage1(void) {
    uint32_t fault_addr = ICADRMS;
    uint32_t aligned_addr = fault_addr & ~0x1FF;
    uint32_t flash_offset = (aligned_addr & 0x00ffffff) + MAIN_FLASH_OFFSET;
    uint32_t page_num = aligned_addr >> 9;
    uint32_t index    = page_num & 0x3f;
    uint32_t dest_ram = 0x00060000 + (index * 0x200);

    /* Unlock SRAM Backing for DMA (seems optional, not sure) */
    ICINDEX = index;
    CACHCON1 = 0x10;
    ICTAG = 0;
    CACHCON1 = 0x04;

    /* Perform Raw Unscrambled DMA Read via ROM */
    typedef uint32_t (*rom_dma_t)(uintptr_t, uintptr_t, uint32_t, uint32_t, uint32_t);
    ((rom_dma_t)0x0008964c)(dest_ram, flash_offset, 0x200, 0, 0);

    /* Lock SRAM & Commit Valid Tag.
     * CRITICAL: Re-assert ICINDEX because DMA clobbers it! */
    ICINDEX = index;              
    ICTAG = page_num | 0x10000;
    CACHCON1 = 0x24;

    return 0;
}
```

## 9. Main Application: Standalone Custom XIP ISR

Once `main.bin` boots, you can completely discard the BootROM by pointing the exception vector to your own bare-metal ISR and handling the raw SPI DMA manually.

```c
/* Raw SPI Flash Reader (Bypasses LFSR DRM natively) */
static inline __attribute__((always_inline)) 
void spi_read_raw(uint32_t dest_ram, uint32_t flash_offset, uint32_t size) {
    uint8_t spi_cmd_buf[4] __attribute__((aligned(4)));

    /* A. Assert Chip Select (Low) */
    if ((FUNCMCON0 & 0xF0) == 0x10) GPIOGCLR = 4;
    else GPIOECLR = 2;

    /* B. Prepare the SPI command (0x03 = Standard Read) */
    spi_cmd_buf[0] = 0x03; 
    spi_cmd_buf[1] = (uint8_t)(flash_offset >> 16);
    spi_cmd_buf[2] = (uint8_t)(flash_offset >> 8);
    spi_cmd_buf[3] = (uint8_t)(flash_offset);

    /* C. DMA 1: Send Command + Address to Flash */
    SPI0CON &= ~0x10;
    SPI0DMAADR = (uint32_t)spi_cmd_buf;
    SPI0DMACNT = 4;
    while ((SPI0CON & (1 << 16)) == 0); 
    
    /* D. DMA 2: Stream Data Payload directly to Cache SRAM */
    SPI0CON |= 0x310;
    SPI0DMAADR = dest_ram;
    SPI0DMACNT = size;
    while ((SPI0CON & (1 << 16)) == 0);

    /* E. Cleanup SPI / DMA State & Release CS */
    SPI0CON &= ~0x300;
    if ((FUNCMCON0 & 0xF0) == 0x10) GPIOGSET = 4;
    else GPIOESET = 2;
}

/* Standalone Exception Handler for XIP Cache Misses */
__attribute__((interrupt))
void xip_isr(void) {
    /* Hardware Cache Exception Acknowledge */
    CACHCON1 = 0x20002;

    uint32_t fault_addr = ICADRMS;
    uint32_t aligned_addr = fault_addr & ~0x1FF;
    uint32_t flash_offset = (aligned_addr & 0x00ffffff) + XIP_FLASH_OFFSET;
    
    uint32_t page_num = aligned_addr >> 9;
    uint32_t index = page_num & 0x3F;
    uint32_t dest_ram = 0x00060000 + (index * 0x200);

    /* Raw DMA Read */
    spi_read_raw(dest_ram, flash_offset, 0x200);

    /* Lock SRAM & Commit Valid Tag */
    ICINDEX = index;
    ICTAG = page_num | 0x10000;
    CACHCON1 = 0x24;
    
    /* Proprietary Hardware Exception Return.
     * Clears the PIC pending state and restores context. */
    uint32_t piccon = PICCON;
    PICCON = piccon & 0xFFFFFFFE;
    EPICCON = piccon;
}
```

## 10. Porting & Reverse Engineering Methodology

If approaching a completely undocumented Bluetrum chip, follow these steps:

1.  **Locate the Final Jump:** Search the ROM dump for `jump/call 0x10000000`. Trace backwards to find the equivalents of `MEMCON`, `NMICON`, and interrupt enables.
2.  **Locate the Callback Pointer:** Search the ROM for `mret` (or equivalent PIC context restores). Find the interrupt dispatcher that leads to an indirect function call (`jalr`) using an address stored in RAM (e.g., `0x10044`).
3.  **Locate the Raw SPI Primitive:** Look for a ROM function taking 4-5 arguments that interacts with SPI configuration registers and DMA count registers. Verify it by extracting a 512-byte block to RAM and comparing it against a physical SPI dump.
4.  **Trace Cache Geometry:** Decompile the default callback. Look for `>> N` (determines line size) and `& M` (determines number of cache lines).
5.  **Build a RAM Probe:** Do not build a full stage-1 immediately. Build an in-RAM probe that verifies raw DMA, dumps cache registers, and tests filling a single cache line manually.
