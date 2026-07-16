OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
    /* The AB5396 BootROM downloads the loader directly into RAM at 0x12000 */
    RAM (rwx) : ORIGIN = 0x12000, LENGTH = 64K
}

SECTIONS
{
    /* Start mapping sections at the beginning of the RAM region */
    . = ORIGIN(RAM);

    .text : 
    {
        /* MUST be first: This ensures _start and the magic interface 
           header end up exactly at 0x12000 and 0x12010 respectively. */
        KEEP(*(.init))
        
        . = ALIGN(4);
        *(.text)
        *(.text.*)
    } > RAM

    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata.*)
        *(.srodata)       /* RISC-V small rodata */
        *(.srodata.*)
    } > RAM

    .data :
    {
        . = ALIGN(4);
        *(.data)
        *(.data.*)
        *(.sdata)         /* RISC-V small data */
        *(.sdata.*)
    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)          /* RISC-V small bss */
        *(.sbss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /* Discard standard libc/startup sections we don't need for a bare-metal loader */
    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}
