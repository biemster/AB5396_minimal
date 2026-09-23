.section .init, "ax"
.global _start
_start:
	/* 1. Initialize the stack pointer */
	la sp, _estack

	/* 2. Initialize the Global Pointer (CRITICAL for static vars and libc!) */
.option push
.option norelax
	la gp, __global_pointer$
.option pop

	/* 3. Copy .data from XIP flash to RAM */
	la a0, _sidata
	la a1, _sdata
	la a2, _edata
	bge a1, a2, 2f
1:
	lw t0, 0(a0)
	sw t0, 0(a1)
	addi a0, a0, 4
	addi a1, a1, 4
	blt a1, a2, 1b
2:
	/* 4. Zero out .bss */
	la a0, _sbss
	la a1, _ebss
	bge a0, a1, 4f
3:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, 3b
4:
	/* 5. Call libc initialization (sets up memory, statics, etc.) */
	call __libc_init_array
5:
	/* 6. Enter Main */
	call main

	/* Catch unexpected exit from main */
6:  j 6b
