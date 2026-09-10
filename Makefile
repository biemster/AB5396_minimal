PREFIX  ?= riscv64-elf-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump

BLUETRUM_TOOLS := ${HOME}/temp/bluetrum/bluetrum-tools

# Define the physical SPI Flash offset where main.bin will be flashed
MAIN_FLASH_OFFSET ?= 0x00001000

ARCH    := -march=rv32imac_zicsr -mabi=ilp32
CFLAGS  := $(ARCH) -Wall -ffreestanding -nostdlib -I.

all: LUCK.bin main.bin
	@echo ""
	@echo "========================================================"
	@echo "                 BUILD SUCCESSFUL                       "
	@echo "========================================================"
	@echo "Flash Instructions:"
	@echo "  1. Flash 'LUCK.bin' to physical offset 0x00000000"
	@echo "     (Needed only once if the physical offset for"
	@echo "      main.bin does not change)"
	@echo "  2. Flash 'main.bin' to physical offset $(MAIN_FLASH_OFFSET)"
	@echo "========================================================"

# --- STAGE 1 (BOOTLOADER) ---
boot.elf: startup_boot.S boot.c boot.ld
	@echo -e "\n*** COMPILING STAGE 1 BOOTLOADER ***"
	$(CC) $(CFLAGS) -DMAIN_FLASH_OFFSET=$(MAIN_FLASH_OFFSET) -T boot.ld startup_boot.S boot.c -o $@

boot.bin: boot.elf
	$(OBJCOPY) -O binary $< $@

# Wrapper and Warning Rule
LUCK.bin: boot.bin boot.c
	@echo -e "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	@echo "WARNING: new stage1 bootloader LUCK.bin has been generated!"
	@echo "Ensure that it starts with a way to exit stage1(), for example"
	@echo "by listening for a magic byte on uart or checking a GPIO."
	@echo "Mistakes in Stage 1 can brick the chip!"
	@echo -e "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
	python $(BLUETRUM_TOOLS)/mkheader.py -b --chipid 4c55434b01000000 boot.bin LUCK.bin

# --- STAGE 2 (MAIN XIP APP) ---
main.elf: startup_main.S main.c main.ld
	@echo -e "\n*** COMPILING MAIN APPLICATION ***"
	$(CC) $(CFLAGS) -T main.ld startup_main.S main.c -o $@

main.bin: main.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< > main.lst

clean:
	rm -f *.elf *.bin main.lst
