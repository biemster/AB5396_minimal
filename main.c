#include <stdint.h>
#include <string.h>
#include "sfr.h"
#include "inc/ush.h"

/* ROM prototypes */
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

static void print_hex8(uint8_t v) {
	static const char hex[] = "0123456789abcdef"; // keep this to test the XIP pager
	ROM_UART0_PUTCHAR(hex[v >> 4]);
	ROM_UART0_PUTCHAR(hex[v & 0xf]);
}

static void print_hex32(uint32_t v) {
	print_hex8(v >> 24);
	print_hex8(v >> 16);
	print_hex8(v >> 8);
	print_hex8(v & 0xff);
}

static void print_string(const char *str) {
	while (*str) {
		ROM_UART0_PUTCHAR(*str++);
	}
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
	print_hex32((uint32_t)line);
	ROM_UART0_PUTCHAR('\n');
	ROM_UART0_PUTCHAR('\r');

	/* stop so you can see the output (or replace with breakpoint) */
	while (1) {
		ROM_DELAY(100000);
	}
}

/* uart0_poll as above */
static int uart0_poll(void) {
	if ((UART0CON & 0x200) == 0) return -1;
	uint32_t data = UART0DATA;
	UART0CPND = 0x200;
	return (data & 0xff);
}

static int ush_read_cb(struct ush_object *self, char *ch)
{
	(void)self;
	int v = uart0_poll();   /* returns -1 if nothing */
	if (v < 0) return 0;    /* no data */
	*ch = (char)(v & 0xff);
	return 1;               /* a char was read */
}

static int ush_write_cb(struct ush_object *self, char c)
{
	(void)self;
	ROM_UART0_PUTCHAR(c);
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

static struct ush_object g_ush;
static struct ush_node_object g_root;

int main(void) {
	/* platform init */
	ROM_CLOCK_INIT();
	ROM_UART0_INIT();

	/* hostname debug */
	strcpy(g_hostname, "AB5396");

	/* initialize shell object and descriptor */
	memset(&g_ush, 0, sizeof(g_ush));
	ush_init(&g_ush, &ush_desc);

	/* mount root node (empty root for now) */
	ush_node_mount(&g_ush, "/", &g_root, NULL, 0);

	ush_printf(&g_ush, "~ %s ~\r\n", g_hostname);

	/* main loop: non-blocking service */
	while (1) {
		ush_service(&g_ush);
		
		/* other periodic tasks can run here */
		ROM_DELAY(1000);
		WDTCON = 10; // feed doggy
	}

	/* unreachable */
	return 0;
}
