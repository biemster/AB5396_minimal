OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY {
	XIP (rx)  : ORIGIN = 0x10000000, LENGTH = 0x00080000
	RAM (rwx) : ORIGIN = 0x00012000, LENGTH = 0x0000E000
}

SECTIONS {
	.text : {
		KEEP(*(.init))
		*(.text .text.*)
		*(.rodata .rodata.* .srodata .srodata.*)
		
		. = ALIGN(4);
		/* libc initialization arrays */
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
		
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
		KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
		PROVIDE_HIDDEN (__init_array_end = .);

		. = ALIGN(4);
	} > XIP

	.data : {
		. = ALIGN(4);
		_sdata = .;
		
		/* Define the Global Pointer for linker relaxation */
		PROVIDE( __global_pointer$ = . + 0x800 );
		
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
		*(.sbss .sbss.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
		
		/* Define the 'end' symbols required by newlib's _sbrk (malloc) */
		PROVIDE( end = . );
		PROVIDE( _end = . );
	} > RAM

	_estack = ORIGIN(RAM) + LENGTH(RAM);
}
