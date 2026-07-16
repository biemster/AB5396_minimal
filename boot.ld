OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
	/* The Boot ROM uses 0x10000 - 0x107FF for its own buffers.
		 Our bootloader is loaded to 0x10800. We give it a safe 30KB window. */
	RAM (rwx) : ORIGIN = 0x10800, LENGTH = 0x7800
}

SECTIONS
{
	.text :
	{
		/* The assembly startup routine */
		KEEP(*(.init))
		
		/* The C program code */
		*(.text .text.*)
		*(.rodata .rodata.*)
		. = ALIGN(4);
	} > RAM

	.data :
	{
		. = ALIGN(4);
		_sdata = .;
		*(.data .data.*)
		*(.sdata .sdata.*)
		. = ALIGN(4);
		_edata = .;
	} > RAM

	.bss :
	{
		. = ALIGN(4);
		_sbss = .;
		*(.sbss .sbss.*)
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > RAM

	/* Set stack pointer to the top of our RAM region */
	_estack = ORIGIN(RAM) + LENGTH(RAM);
}
