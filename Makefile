PREFIX  ?= riscv64-elf-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump

# AB5396B is RV32IMAC
ARCH    := -march=rv32imac_zicsr -mabi=ilp32
CFLAGS  := $(ARCH) -Wall -ffreestanding -nostdlib -I.
LDFLAGS := -T boot.ld -Wl,--no-relax -Wl,--gc-sections

all: my_boot.bin

my_boot.elf: startup.S main.c
	$(CC) $(CFLAGS) $(LDFLAGS) startup.S main.c -o $@

my_boot.bin: my_boot.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< > my_boot.dis

clean:
	rm -f my_boot.elf my_boot.bin my_boot.dis
