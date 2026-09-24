.section .init, "ax"
.global _start
_start:
	la sp, _estack

.option push
.option norelax
	la gp, __global_pointer$
.option pop

	/* --- THE RAM TRAMPOLINE FIX ---
	   We cannot read XIP Page 64 while executing from XIP Page 0.
	   We must copy our payload to the stack (RAM) and run it there. */
	   
	/* 1. Copy the 28-byte payload to the stack */
	la t0, copy_payload_start
	la t1, copy_payload_end
	addi sp, sp, -32        /* Allocate 32 bytes on stack */
	mv t2, sp               /* t2 is destination on stack */
1:
	bge t0, t1, 2f
	lw t3, 0(t0)
	sw t3, 0(t2)
	addi t0, t0, 4
	addi t2, t2, 4
	j 1b
2:
	/* 2. Load arguments for the payload */
	la a0, _sidata
	la a1, _sdata
	la a2, _edata

	/* 3. Execute payload from the stack (RAM) */
	jalr sp

	/* 4. Restore stack */
	addi sp, sp, 32

	/* 5. Zero out .bss */
	la a0, _sbss
	la a1, _ebss
	bge a0, a1, 4f
3:
	sw zero, 0(a0)
	addi a0, a0, 4
	blt a0, a1, 3b
4:
	/* 6. Call libc initialization (sets up memory, statics, etc.) */
	call __libc_init_array
5:
	/* 7. Enter Main */
	call main

	/* Catch unexpected exit from main */
6:  j 6b

/* --- POSITION INDEPENDENT COPY LOOP --- 
   This code is copied to RAM and executed there. 
   It uses relative jumps so it doesn't care where it lives! */
.align 2
copy_payload_start:
1:
	bge a1, a2, 2f
	lw t0, 0(a0)
	sw t0, 0(a1)
	addi a0, a0, 4
	addi a1, a1, 4
	j 1b
2:
	ret
copy_payload_end:
