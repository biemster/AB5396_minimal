OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY {
	RAM (rwx) : ORIGIN = 0x10800, LENGTH = 0x1800
}

SECTIONS {
	.text : {
		KEEP(*(.init))
		*(.text .text.*)
		*(.rodata .rodata.*)
	} > RAM
	
	.data : { *(.data .data.*) *(.sdata .sdata.*) } > RAM
	.bss : { *(.bss .bss.*) *(COMMON) } > RAM
	
	_estack = ORIGIN(RAM) + LENGTH(RAM);
}
