OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY {
	/* XIP Flash memory window */
	XIP (rx)  : ORIGIN = 0x10000000, LENGTH = 0x00080000
	/* General purpose RAM */
	RAM (rwx) : ORIGIN = 0x00012000, LENGTH = 0x0000E000
}

SECTIONS {
	.text : {
		KEEP(*(.init))
		*(.text .text.*)
		*(.rodata .rodata.*)
		. = ALIGN(4);
	} > XIP

	/* .data must be loaded from XIP to RAM during startup */
	.data : {
		. = ALIGN(4);
		_sdata = .;
		*(.data .data.*)
		*(.sdata .sdata.*)
		. = ALIGN(4);
		_edata = .;
	} > RAM AT> XIP
	
	_sidata = LOADADDR(.data);

	.bss : {
		. = ALIGN(4);
		_sbss = .;
		*(.bss .bss.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} > RAM

	_estack = ORIGIN(RAM) + LENGTH(RAM);
}
