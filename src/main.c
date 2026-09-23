#include <stdint.h>
#include <string.h>
#include "sfr.h"
#include "inc/ush.h"
#include "bluetrum_usb.h"

/* ROM prototypes */
typedef void (*rom_usb_bootloader)(void);
#define ROM_USB_BOOTLOADER ((rom_usb_bootloader)0x00080c44)

typedef void (*rom_clock_init)(void);
#define ROM_CLOCK_INIT ((rom_clock_init)0x00080b94)

typedef void (*rom_uart0_init)(void);
#define ROM_UART0_INIT ((rom_uart0_init)0x0008173e)

typedef void (*rom_uart0_putchar)(char c);
#define ROM_UART0_PUTCHAR ((rom_uart0_putchar)0x0008172e)

typedef char (*rom_uart0_getchar)(void);
#define ROM_UART0_GETCHAR ((rom_uart0_putchar)0x00081712)

typedef void (*rom_delay)(uint32_t cycles);
#define ROM_DELAY ((rom_delay)0x00080284)

/*
 * --------------------------------------------------------------------------
 * Format Helpers
 * --------------------------------------------------------------------------
 */
#ifndef REG32
#define REG32(addr)  (*(volatile uint32_t *)(uintptr_t)(addr))
#endif

static void uart0_print_hex8(uint8_t v) {
	static const char hex[] = "0123456789abcdef"; // keep this to test the XIP pager
	ROM_UART0_PUTCHAR(hex[v >> 4]);
	ROM_UART0_PUTCHAR(hex[v & 0xf]);
}

static void uart0_print_hex32(uint32_t v) {
	uart0_print_hex8(v >> 24);
	uart0_print_hex8(v >> 16);
	uart0_print_hex8(v >> 8);
	uart0_print_hex8(v & 0xff);
}

static void uart0_print_string(const char *str) {
	while (*str) {
		ROM_UART0_PUTCHAR(*str++);
	}
}

/* 
 * The "interrupt" attribute is critical for RISC-V so the compiler 
 * generates an `mret` and saves all registers
 */
#define VECTOR_SIZE 16
static uint32_t isr_vector_counter[VECTOR_SIZE] = {0};
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr0(void) { isr_vector_counter[0]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr1(void) { isr_vector_counter[1]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr2(void) { isr_vector_counter[2]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr3(void) { isr_vector_counter[3]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr4(void) { isr_vector_counter[4]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr5(void) { isr_vector_counter[5]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr6(void) { isr_vector_counter[6]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr7(void) { isr_vector_counter[7]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr8(void) { isr_vector_counter[8]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr9(void) { isr_vector_counter[9]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr10(void) { isr_vector_counter[10]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr11(void) { isr_vector_counter[11]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr12(void) { isr_vector_counter[12]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr13(void) { isr_vector_counter[13]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr14(void) { isr_vector_counter[14]++; }
__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr15(void) { isr_vector_counter[15]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr16(void) { isr_vector_counter[16]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr17(void) { isr_vector_counter[17]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr18(void) { isr_vector_counter[18]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr19(void) { isr_vector_counter[19]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr20(void) { isr_vector_counter[20]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr21(void) { isr_vector_counter[21]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr22(void) { isr_vector_counter[22]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr23(void) { isr_vector_counter[23]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr24(void) { isr_vector_counter[24]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr25(void) { isr_vector_counter[25]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr26(void) { isr_vector_counter[26]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr27(void) { isr_vector_counter[27]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr28(void) { isr_vector_counter[28]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr29(void) { isr_vector_counter[29]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr30(void) { isr_vector_counter[30]++; }
//__attribute__((section(".isr"))) __attribute__((interrupt)) void default_isr31(void) { isr_vector_counter[31]++; }

__attribute__((section(".isr")))
__attribute__((interrupt))
void usb_isr_wrapper(void) {
	bt_usb_isr(); // Call our stack's handler
}

/* No section attribute required, GCC naturally puts this in RAM (.data) */
__attribute__((aligned(256)))
void *isr_vector_table[VECTOR_SIZE] = {0};

uint32_t isr_vector_counter_get(size_t index) {
	return (index < VECTOR_SIZE) ? isr_vector_counter[index] : 0;
}

/* called from USH_ASSERT on failure */
void ush_assert_failed(const char *file, int line) {
	/* simple visible marker */
	ROM_UART0_PUTCHAR('!');
	/* print first few chars of file name (optional) */
	for (int i = 0; i < 12 && file && file[i]; ++i) {
		char c = file[i];
		ROM_UART0_PUTCHAR(c);
	}

	ROM_UART0_PUTCHAR(':');

	/* print line number as hex */
	uart0_print_hex32((uint32_t)line);
	ROM_UART0_PUTCHAR('\n');
	ROM_UART0_PUTCHAR('\r');

	/* stop so you can see the output (or replace with breakpoint) */
	while (1) {
		ROM_DELAY(100000);
	}
}

static int uart0_singlewire_inchar = -1;
static int uart0_poll(void) {
	if ((UART0CON & 0x200) == 0) return -1;
	uint32_t data = UART0DATA;
	UART0CPND = 0x200;
	uart0_singlewire_inchar = (data & 0xff);
	return uart0_singlewire_inchar;
}

static int ush_read_cb(struct ush_object *self, char *ch) {
	(void)self;
	int v = -1;
	if(bt_cdc_is_connected()) {
		v = bt_cdc_read_char();
	}
	else {
		v = uart0_poll();
		uart0_singlewire_inchar = v;
	}
	if (v < 0) return 0;    /* no data */

	*ch = (char)(v & 0xff);
	return 1;               /* a char was read */
}

static int ush_write_cb(struct ush_object *self, char c) {
	(void)self;
	if (bt_cdc_is_connected()) {
		bt_cdc_write_char(c); /* Blocking with safety timeout */
	}
	else {
		if(c != (char)(uart0_singlewire_inchar & 0xff)) {
			ROM_UART0_PUTCHAR(c);
		}
		uart0_singlewire_inchar = -1;
	}
	return 1;
}

/* Shell buffers and descriptor */
#define BUF_IN_SIZE    128
#define BUF_OUT_SIZE   128
#define PATH_MAX_SIZE  128

static char ush_in_buf[BUF_IN_SIZE];
static char ush_out_buf[BUF_OUT_SIZE];
static char g_hostname[32];

static const struct ush_io_interface ush_iface = {
	.read = ush_read_cb,
	.write = ush_write_cb,
};

static const struct ush_descriptor ush_desc = {
	.io = &ush_iface,
	.input_buffer = ush_in_buf,
	.input_buffer_size = sizeof(ush_in_buf),
	.output_buffer = ush_out_buf,
	.output_buffer_size = sizeof(ush_out_buf),
	.path_max_length = PATH_MAX_SIZE,
	.hostname = g_hostname,
};

// "isr_counts" command
extern void isr_counts_callback(struct ush_object *self, struct ush_file_descriptor const *file, int argc, char *argv[]);
extern void isr_counts_service(struct ush_object *self, struct ush_file_descriptor const *file);
static const struct ush_file_descriptor g_isr_counts_cmd_files[] = {
	{
		.name = "isr_counts",
		.description = "dump ISR vector counters",
		.help = "usage: isr_counts\r\n",
		.exec = isr_counts_callback,
		.process = isr_counts_service,
	},
};

// "radio_ctrl" command
extern void radio_ctrl_callback(struct ush_object *self, struct ush_file_descriptor const *file, int argc, char *argv[]);
extern void radio_ctrl_service(struct ush_object *self, struct ush_file_descriptor const *file);
static const struct ush_file_descriptor g_radio_cmd_files[] = {
	{
		.name = "radio_ctrl",
		.description = "dump Bluetrum RF registers",
		.help = "usage: radio_ctrl\r\n"
				"       radio_ctrl --init\r\n"
				"       radio_ctrl --adv [nadv]\r\n"
				"       radio_ctrl --clk\r\n"
				"       radio_ctrl --dumpregs\r\n",
		.exec = radio_ctrl_callback,
		.process = radio_ctrl_service,
	},
};

static struct ush_object g_ush;
static struct ush_node_object g_root;
static struct ush_node_object g_radio_cmd_node;
static struct ush_node_object g_isr_counts_cmd_node;

int main(void) {
	/* platform init */
	ROM_CLOCK_INIT();
	ROM_UART0_INIT();

	isr_vector_table[0] = (void *)default_isr0;
	isr_vector_table[1] = (void *)default_isr1;
	isr_vector_table[2] = (void *)default_isr2;
	isr_vector_table[3] = (void *)default_isr3;
	isr_vector_table[4] = (void *)default_isr4;
	isr_vector_table[5] = (void *)default_isr5;
	isr_vector_table[6] = (void *)default_isr6;
	isr_vector_table[7] = (void *)default_isr7;
	isr_vector_table[8] = (void *)0x00084020, // 0x20: ISR8 XIP Cache NMI handler, don't change!
	isr_vector_table[9] = (void *)default_isr9;
	isr_vector_table[10] = (void *)default_isr10;
	isr_vector_table[11] = (void *)default_isr11;
	isr_vector_table[12] = (void *)default_isr12;
	isr_vector_table[13] = (void *)default_isr13;
	isr_vector_table[14] = (void *)default_isr14;
	isr_vector_table[15] = (void *)usb_isr_wrapper;
//	isr_vector_table[16] = (void *)default_isr16;
//	isr_vector_table[17] = (void *)default_isr17;
//	isr_vector_table[18] = (void *)default_isr18;
//	isr_vector_table[19] = (void *)default_isr19;
//	isr_vector_table[20] = (void *)default_isr20;
//	isr_vector_table[21] = (void *)default_isr21;
//	isr_vector_table[22] = (void *)default_isr22;
//	isr_vector_table[23] = (void *)default_isr23;
//	isr_vector_table[24] = (void *)default_isr24;
//	isr_vector_table[25] = (void *)default_isr25;
//	isr_vector_table[26] = (void *)default_isr26;
//	isr_vector_table[27] = (void *)default_isr27;
//	isr_vector_table[28] = (void *)default_isr28;
//	isr_vector_table[29] = (void *)default_isr29;
//	isr_vector_table[30] = (void *)default_isr30;
//	isr_vector_table[31] = (void *)default_isr31;
	PICADR = (uint32_t)isr_vector_table;

	/* Init USB CDC ACM */
	bt_usb_init();
	PICPR |= 0x80; // Set interrupt priority/routing for USB
	PICEN |= 0x80; // Enable the USB interrupt line in the PIC
	PICCON |= 0x10007;

	/* microshell hostname */
	strcpy(g_hostname, "AB5396");

	/* initialize shell object and descriptor */
	memset(&g_ush, 0, sizeof(g_ush));
	ush_init(&g_ush, &ush_desc);
	ush_commands_add( &g_ush, &g_radio_cmd_node, g_radio_cmd_files, (sizeof(g_radio_cmd_files) / sizeof(g_radio_cmd_files[0])) );
	ush_commands_add( &g_ush, &g_isr_counts_cmd_node, g_isr_counts_cmd_files, (sizeof(g_isr_counts_cmd_files) / sizeof(g_isr_counts_cmd_files[0])) );

	/* mount root node (empty root for now) */
	ush_node_mount(&g_ush, "/", &g_root, NULL, 0);
	ush_printf(&g_ush, "~ %s ~\r\n", g_hostname);

	/* main loop: non-blocking service */
	while (1) {
		ush_service(&g_ush);

		/* other periodic tasks can run here */
		ROM_DELAY(100); // don't starve DMA masters (for USB for example)
		WDTCON = 10; // feed doggy
	}

	/* unreachable */
	return 0;
}
