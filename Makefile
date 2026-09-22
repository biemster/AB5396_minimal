PREFIX  ?= riscv64-elf-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump

BLUETRUM_TOOLS := ../bluetrum-tools
MICROSHELL_DIR ?= ../microshell

# Define the physical SPI Flash offset where main.bin will be flashed
MAIN_FLASH_OFFSET ?= 0x00001000

ARCH    := -march=rv32imac_zicsr -mabi=ilp32
CFLAGS  := $(ARCH) -mstrict-align -Wall -ffreestanding -nostartfiles -ffunction-sections -fdata-sections -Iinclude -I$(MICROSHELL_DIR)/src -DUSH_CONFIG_CUSTOM_FILE=\"ush_config.h\"
LDFLAGS := -mstrict-align --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,--undefined=g_ush_buildin_commands -Wl,--no-warn-rwx-segments -lc -lgcc

# Microshell source files (the repo stores sources in src/src and commands in src/src/commands)
MICRO_SRCS := $(MICROSHELL_DIR)/src/src/ush.c \
			$(MICROSHELL_DIR)/src/src/ush_read.c \
			$(MICROSHELL_DIR)/src/src/ush_read_utils.c \
			$(MICROSHELL_DIR)/src/src/ush_read_char.c \
			$(MICROSHELL_DIR)/src/src/ush_parse.c \
			$(MICROSHELL_DIR)/src/src/ush_parse_char.c \
			$(MICROSHELL_DIR)/src/src/ush_parse_utils.c \
			$(MICROSHELL_DIR)/src/src/ush_write.c \
			$(MICROSHELL_DIR)/src/src/ush_write_utils.c \
			$(MICROSHELL_DIR)/src/src/ush_prompt.c \
			$(MICROSHELL_DIR)/src/src/ush_reset.c \
			$(MICROSHELL_DIR)/src/src/ush_file.c \
			$(MICROSHELL_DIR)/src/src/ush_node.c \
			$(MICROSHELL_DIR)/src/src/ush_node_utils.c \
			$(MICROSHELL_DIR)/src/src/ush_node_mount.c \
			$(MICROSHELL_DIR)/src/src/ush_utils.c \
			$(MICROSHELL_DIR)/src/src/ush_commands.c \
			$(MICROSHELL_DIR)/src/src/ush_process.c \
			$(MICROSHELL_DIR)/src/src/ush_autocomp.c \
			$(MICROSHELL_DIR)/src/src/ush_autocomp_utils.c \
			$(MICROSHELL_DIR)/src/src/ush_autocomp_state.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_cd.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_help.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_ls.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_pwd.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_cat.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_xxd.c \
			$(MICROSHELL_DIR)/src/src/commands/ush_cmd_echo.c \
			microshell/ush_cmd_radio.c \
			microshell/ush_cmd_isrcounts.c

MICRO_OBJS := $(MICRO_SRCS:.c=.o)

# --- STAGE 1 (BOOTLOADER) ---
boot.elf:  linker/boot.ld startup/startup_boot.S src/boot.c
	@echo -e "\n*** COMPILING STAGE 1 BOOTLOADER ***"
	$(CC) $(CFLAGS) -DMAIN_FLASH_OFFSET=$(MAIN_FLASH_OFFSET) -T $^ -o $@

boot.bin: boot.elf
	$(OBJCOPY) -O binary $< $@

# Wrapper and Warning Rule
LUCK.bin: boot.bin src/boot.c
	@echo -e "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	@echo "WARNING: new stage1 bootloader LUCK.bin has been generated!"
	@echo "Ensure that it starts with a way to exit stage1(), for example"
	@echo "by listening for a magic byte on uart or checking a GPIO."
	@echo "Mistakes in Stage 1 can brick the chip!"
	@echo -e "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
	python $(BLUETRUM_TOOLS)/mkheader.py -b --chipid 4c55434b01000000 boot.bin LUCK.bin

# --- STAGE 2 (MAIN XIP APP) ---
# Compile microshell objects (only once if up-to-date)
$(MICRO_OBJS): %.o: %.c
	@echo "CC $<"
	$(CC) $(CFLAGS) -c $< -o $@

main.elf: linker/main.ld startup/startup_main.S src/main.c $(MICRO_OBJS)
	@echo -e "\n*** COMPILING MAIN APPLICATION ***"
	$(CC) $(CFLAGS) -T $^ $(LDFLAGS) -o $@

main.bin: main.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< > main.lst

all: main.bin LUCK.bin
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

clean:
	rm -f *.elf *.bin main.lst $(MICRO_OBJS)
