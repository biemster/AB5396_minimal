# Bluetrum AB5396B: XIP Cache & BootROM Architecture Reference

This document provides a comprehensive blueprint for reverse-engineering and implementing bare-metal XIP (Execute-In-Place) on Bluetrum RISC-V microcontrollers. Unlike standard chips with transparent SPI mapping, Bluetrum utilizes a **software-assisted hardware cache**. Accessing the XIP window triggers a hardware exception, forcing the CPU to run a ROM-based interrupt handler (`ISR7`), which calls a RAM-resident callback to manually fill the cache line via SPI DMA.

## 1. Core Architecture & Memory Map

*   **`0x10000000 - 0x1007FFFF`**: CPU XIP Virtual Memory Window.
*   **`0x00060000 - 0x00067FFF`**: Cache SRAM Backing Memory (32 KiB total on AB5396B).
*   **`0x00010000 - 0x0001DFFF`**: Base of standard RAM. (size still to be determined)
*   **`0x00080000 - 0x0008FFFF`**: Base of Mask ROM (BootROM).

## 2. Hardware Registers & Cache Controller

### XIP Controller (`0x54000` base)
Controls the SPI flash interface parameters for XIP mode.
*   `0x54000 (XIP_CMD)`: SPI command used for reads (e.g., `0x342` for standard dual/quad read, derived from eFuses).
*   `0x54004 (XIP_CTRL)`: Enable bit (set to `1`).
*   `0x54010 (XIP_BASE) / 0x54014 (XIP_LIMIT)`: Configures the addressable window (0/0 uses defaults).

### System & Memory Control
*   `0x3B8 (MEMCON)`: Bit 16 (`0x10000`) maps the SPI flash into the CPU address space.

### Cache Controller (`0x454` base)
*   `0x454 (CACHCON0)`: Master Cache Control (initialized with `0x10001`).
*   `0x458 (CACHCON1)`: Cache SRAM Arbiter / Command Register.
*   `0x45c (ICTAG)`: Cache Line Tag & Valid Register.
*   `0x460 (ICINDEX)`: Target Cache Line Index (0 - 63).
*   `0x464 (ICADRMS)`: **Cache Miss Address.** Contains the exact CPU address that faulted (e.g., `0x10000200`).
*   `0x468 (ICLOCK)`: Cache-line lock/allocation state.

## 3. The Cache Geometry & State Transitions

By mapping the cache index calculation in the BootROM, the hardware parameters are:
*   **Line Size**: 512 bytes (`0x200`). (Address shift = 9).
*   **Cache Size**: 64 lines (32 KiB). (Index mask = `0x3F`).
*   **Valid Bit**: Bit 16 (`0x10000`) of `ICTAG`.

**Hardware Cache Arbitration:**
To write to the `0x60000` SRAM via SPI DMA, you must explicitly negotiate with `CACHCON1`:
1.  **Unlock**: `ICINDEX = idx; CACHCON1 = 0x10; ICTAG = 0; CACHCON1 = 0x04;`
2.  **DMA Read**: Read 512 bytes to `0x60000 + (idx * 0x200)`.
3.  **Lock & Validate**: `ICTAG = page_num | 0x10000; CACHCON1 = 0x24;`

## 4. Hardware LFSR Scrambler & DRM Bypass

Bluetrum implements firmware DRM via an on-the-fly hardware LFSR inside the SPI DMA engine. 
The ROM provides a low-level DMA primitive at **`0x8964c`**:
`uint32_t dma(uintptr_t dest, uintptr_t flash_addr, uint32_t size, uint32_t lfsr_cfg, uint32_t lfsr_en)`

**The Bypass:** By routing our custom XIP callback through this ROM function and passing `lfsr_cfg = 0` and `lfsr_en = 0`, we bypass the encryption DRM entirely, allowing standard raw GCC binaries to be executed directly from flash.

## 5. BootROM Exceptions & Callbacks

The chip does not require replacing the entire interrupt vector table to support XIP. The ROM exposes configurable abstraction pointers in RAM:
*   `0x10044 (ISR7_CALLBACK)`: Invoked by the ROM exception handler on a cache miss.
*   `0x10050 (TRANSFER_CALLBACK)`: Used by the ROM for internal DMA logic (safely ignored by our custom callback).

*Crucial Rule:* The `ISR7` cache miss mechanism relies on the CPU's global exception state. **Never execute `csrw mstatus, zero`** in your application startup code. Doing so disables interrupts (`MIE=0`), which prevents the ROM from servicing the next XIP cache miss, resulting in a hard lockup during execution.

## 6. Identified BootROM Functions

Identifying these functions is the key to hijacking the boot process on any Bluetrum chip:

| Address (AB5396B) | Function Signature | Role & Deduced Functionality |
| :--- | :--- | :--- |
| **`0x00084000`** | `void system_init(void)` | **Core Init.** Initializes system clocks/peripherals, clears ROM BSS, and maps the exception vector table. |
| **`0x0008402c`** | `void install_paging_metadata(void *b)` | **Vendor Cache Allocator Init.** Distributes an 11-word struct over RAM (`0x10044 - 0x1006c`). More importantly, it writes `0x10001` to `CACHCON0`. We mimic this state manually. |
| **`0x00084024`** | `void install_default_callbacks(void)` | **Callback Installer.** Hardcodes `0x86738` into `0x10044` (ISR7 cache callback) and `0x8964c` into `0x10050` (SPI transfer callback). |
| **`0x00010126`** | `void xip_efuse_setup(void)` | **eFuse/XIP Config.** Reads eFuse configuration and pushes it into the `XIP_CMD` register (e.g., `0x342`) to set SPI read modes. |
| **`0x00086738`** | `void cache_miss_handler(void)` | **Default Cache Callback.** This is what the ROM places in `0x10044`. |
| **`0x000863a4`** | `void cache_page_loader(void)` | **Deep Cache Arbiter.** Contains the critical hardware sequencing logic (`CACHCON1 = 0x10 ... 0x04`) used to unlock SRAM, load pages, and lock them. |
| **`0x0008964c`** | `uint32_t dma(dest, flash_addr, size, lfsr_cfg, lfsr_en)` | **Low-level DMA Primitive.** Programs SPI/DMA to read flash. |

## 7. C Implementation: Stage-1 Boot Configuration

To replace the proprietary vendor bootloader, you must create a minimal `stage1()` that configures the hardware cache and injects the custom callback. This code must run purely from internal RAM.

```c
#define REG32(addr)  (*(volatile uint32_t *)(uintptr_t)(addr))

void setup_xip_and_boot(void) {
    /* 1. Hijack the ROM exception callback pointer. 
     * When the CPU hits an unmapped XIP address, ISR7 fires and calls this RAM address. */
    REG32(0x00010044) = (uint32_t)(uintptr_t)&xip_callback;

    /* 2. Configure the SPI Flash interface.
     * XIP_CMD must match your flash chip and eFuse state (0x342 = standard fast read).
     * XIP_CTRL = 1 enables the controller. BASE/LIMIT of 0 uses defaults. */
    REG32(0x00054000) = 0x00000342; /* XIP_CMD */
    REG32(0x00054004) = 1;          /* XIP_CTRL */
    REG32(0x00054010) = 0;          /* XIP_BASE */
    REG32(0x00054014) = 0;          /* XIP_LIMIT */

    /* 3. Reset the Cache Controller Hardware.
     * CACHCON0 = 0x10001 acts as the master cache enable.
     * We clear all lock, tag, and index states to ensure a clean slate. */
    REG32(0x00000454) = 0x00010001; /* CACHCON0 */
    REG32(0x00000458) = 0;          /* CACHCON1 */
    REG32(0x0000045C) = 0;          /* ICTAG */
    REG32(0x00000460) = 0;          /* ICINDEX */
    REG32(0x00000464) = 0;          /* ICADRMS */
    REG32(0x00000468) = 0;          /* ICLOCK */

    /* 4. Enable NMI and Programmable Interrupt Controller (PIC).
     * This is strictly required so that XIP access faults trigger ISR7 correctly. */
    REG32(0x00000408) = 1;          /* NMICON */
    REG32(0x00000448) |= 1;         /* PICPR */

    /* 5. Map the SPI Flash into the CPU Data/Instruction bus. */
    REG32(0x000003B8) |= 0x00010000; /* MEMCON */

    /* 6. Execute the main application from XIP */
    typedef void (*main_entry_t)(void);
    main_entry_t main_app = (main_entry_t)0x10000000;
    main_app();
}
```

## 8. C Implementation: The Cache Miss Callback

This function actually reads the flash. **It must be linked to RAM.** If it sits in XIP space, a cache miss will try to execute it, causing a recursive deadlock.

```c
__attribute__((noinline))
static uint32_t xip_callback(void) {
    /* 1. Fetch the exact CPU address that caused the cache miss (e.g. 0x10000200) */
    uint32_t fault_addr = REG32(0x00000464); /* ICADRMS */
    
    /* 2. Map CPU address to physical SPI flash offset.
     * We mask out the 0x10000000 base, and add our payload offset (bypassing BootROM headers) */
    uint32_t flash_offset = (fault_addr & 0x00ffffff) + MAIN_FLASH_OFFSET;
    
    /* 3. Calculate Cache Geometry 
     * Shift by 9 (512 bytes) to get the logical page.
     * Mask by 0x3F (64 entries) to get the physical cache slot. */
    uint32_t page_num = fault_addr >> 9;
    uint32_t index    = page_num & 0x3f;
    uint32_t dest_ram = 0x00060000 + (index * 0x200);

    /* 4. Unlock SRAM Backing for DMA.
     * CACHCON1 handles arbitration between the CPU and the SPI DMA.
     * 0x10 prepares to modify the tag. 0x04 unlocks the SRAM for DMA writing. */
    REG32(0x00000460) = index; /* ICINDEX */
    REG32(0x00000458) = 0x10;  /* CACHCON1 */
    REG32(0x0000045C) = 0;     /* ICTAG (Invalidate old line) */
    REG32(0x00000458) = 0x04;  /* CACHCON1 */

    /* 5. Perform Raw Unscrambled DMA Read.
     * We pass 0 for lfsr_cfg and lfsr_en to bypass the hardware decryption DRM entirely. */
    typedef uint32_t (*rom_dma_t)(uintptr_t dest, uintptr_t flash, uint32_t size, uint32_t lfsr_cfg, uint32_t lfsr_en);
    rom_dma_t vendor_dma = (rom_dma_t)0x0008964c;
    vendor_dma(dest_ram, flash_offset, 0x200, 0, 0);

    /* 6. Lock SRAM & Mark Line Valid.
     * Bit 16 (0x10000) acts as the "Valid" flag. 
     * CACHCON1 = 0x24 locks the SRAM and commits it back to the CPU. */
    REG32(0x0000045C) = page_num | 0x10000; /* ICTAG */
    REG32(0x00000458) = 0x24;               /* CACHCON1 */

    /* 7. Return to ISR7. 
     * The ROM exception handler will execute 'mret' and retry the faulted instruction. */
    return 0;
}
```

## 9. Porting & Reverse Engineering Methodology

If approaching a completely undocumented Bluetrum chip, follow these steps to find the addresses:

1.  **Locate the Final Jump:** Search the ROM dump for `jump/call 0x10000000`. Trace backwards to find the equivalents of `MEMCON` and interrupt enables.
2.  **Locate the Callback Pointer:** Search the ROM for `mret`. Find the interrupt dispatcher that leads to an indirect function call (`jalr`) using an address stored in RAM (e.g., `0x10044`).
3.  **Locate the Raw SPI Primitive:** Look for a ROM function taking 4-5 arguments that interacts with SPI configuration registers and DMA count registers. Verify it by extracting a 512-byte block to RAM and comparing it against a physical SPI dump.
4.  **Trace Cache Geometry:** Decompile the default callback. Look for `>> N` (determines line size) and `& M` (determines number of cache lines).
5.  **Build a RAM Probe:** Do not build a full stage-1 immediately. Build an in-RAM probe that verifies raw DMA, dumps cache registers, and tests filling a single cache line manually.
