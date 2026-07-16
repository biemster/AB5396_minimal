TARGET = main

TOOLCHAIN = riscv64-elf
CC = $(TOOLCHAIN)-gcc
OBJCOPY = $(TOOLCHAIN)-objcopy
OBJDUMP = $(TOOLCHAIN)-objdump

MINICHLINK = $(HOME)/temp/ch32fun/minichlink/minichlink

ARCH_FLAGS = -march=rv32imc -mabi=ilp32
CFLAGS = $(ARCH_FLAGS) -fPIC -Wall
LDFLAGS = $(ARCH_FLAGS) -mno-relax -nostdlib -T ram.ld -Wl,-Map,$(TARGET).map

SRCS  = $(TARGET).c
SRCS += startup.s

LOCAL_OBJS = $(SRCS:.c=.o)
LOCAL_OBJS := $(LOCAL_OBJS:.s=.o)
PRECOMPILED_OBJS = $(wildcard libs/*.o)
OBJS = $(LOCAL_OBJS) $(PRECOMPILED_OBJS)

.PHONY: all clean

all: $(TARGET).bin $(TARGET).lst

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(TARGET).lst: $(TARGET).elf
	@ $(OBJDUMP) \
		--source \
		--all-headers \
		--demangle \
		--line-numbers \
		--wide "$<" > "$@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.s
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LOCAL_OBJS) $(TARGET).elf $(TARGET).bin $(TARGET).map $(TARGET).lst

upload:
	$(MINICHLINK) -f
	(sleep 1 && $(MINICHLINK) -k5 > /dev/null 2>&1) &
	python upload_fw_to_ram.py --firmware $(TARGET).bin
