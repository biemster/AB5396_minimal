.section .init, "ax"
.global _start
_start:
	tail stage1  /* Use 'tail' instead of 'call' to preserve the ROM's return address */
