/**
 * @file bluetrum_usb.h
 * @brief USB CDC ACM Stack for Bluetrum AB5396 (Mentor MUSB HDRC Core)
 *        - Split-context architecture using Interrupt-driven Edge Sync
 */

#ifndef BLUETRUM_USB_H
#define BLUETRUM_USB_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ==============================================================================
 * SECTION ATTRIBUTES (RAM EXECUTION)
 * ============================================================================== */
#ifndef ISR_FUNC
#define ISR_FUNC     __attribute__((section(".isr")))
#endif

#ifndef USB_FUNC
#define USB_FUNC     __attribute__((section(".usb")))
#endif

/* ==============================================================================
 * RING BUFFER SIZES (POWER OF TWO)
 * ============================================================================== */
#define BT_CDC_RX_BUF_SIZE    256
#define BT_CDC_TX_BUF_SIZE    256
#define BT_CDC_TX_TIMEOUT     100000

/* ==============================================================================
 * USB STANDARD DEFINITIONS & STRUCTS
 * ============================================================================== */
typedef struct __attribute__((packed)) {
	uint8_t  bmRequestType;
	uint8_t  bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} bt_usb_setup_packet_t;

#define USB_REQ_GET_STATUS        0x00
#define USB_REQ_SET_ADDRESS       0x05
#define USB_REQ_GET_DESCRIPTOR    0x06
#define USB_REQ_SET_CONFIGURATION 0x09

#define USB_DESC_TYPE_DEVICE      0x01
#define USB_DESC_TYPE_CONFIG      0x02
#define USB_DESC_TYPE_STRING      0x03

typedef struct __attribute__((packed)) {
	uint32_t dwDTERate;
	uint8_t  bCharFormat;
	uint8_t  bParityType;
	uint8_t  bDataBits;
} usb_cdc_line_coding_t;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
} usb_device_descriptor_t;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t wTotalLength;
	uint8_t  bNumInterfaces;
	uint8_t  bConfigurationValue;
	uint8_t  iConfiguration;
	uint8_t  bmAttributes;
	uint8_t  bMaxPower;
} usb_config_descriptor_t;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bInterfaceNumber;
	uint8_t  bAlternateSetting;
	uint8_t  bNumEndpoints;
	uint8_t  bInterfaceClass;
	uint8_t  bInterfaceSubClass;
	uint8_t  bInterfaceProtocol;
	uint8_t  iInterface;
} usb_interface_descriptor_t;

typedef struct __attribute__((packed)) {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint8_t  bEndpointAddress;
	uint8_t  bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t  bInterval;
} usb_endpoint_descriptor_t;

/* Composite CDC ACM Configuration (62 bytes total) */
typedef struct __attribute__((packed)) {
	usb_config_descriptor_t    config;
	usb_interface_descriptor_t itf0;
	uint8_t                    cdc_header[5];
	uint8_t                    cdc_acm[4];
	uint8_t                    cdc_union[5];
	usb_endpoint_descriptor_t  ep2_in; /* Required by Linux ttyACM driver */
	usb_interface_descriptor_t itf1;
	usb_endpoint_descriptor_t  ep1_out;
	usb_endpoint_descriptor_t  ep1_in;
} cdc_config_descriptor_t;

/* ==============================================================================
 * DESCRIPTOR INSTANTIATIONS
 * ============================================================================== */
static const usb_device_descriptor_t dev_desc = {
	.bLength            = sizeof(usb_device_descriptor_t),
	.bDescriptorType    = USB_DESC_TYPE_DEVICE,
	.bcdUSB             = 0x0110,
	.bDeviceClass       = 0x02,
	.bDeviceSubClass    = 0x00,
	.bDeviceProtocol    = 0x00,
	.bMaxPacketSize0    = 0x40,
	.idVendor           = 0x1209,
	.idProduct          = 0x5396,
	.bcdDevice          = 0x0100,
	.iManufacturer      = 0x01,
	.iProduct           = 0x02,
	.iSerialNumber      = 0x03,
	.bNumConfigurations = 0x01
};

static const cdc_config_descriptor_t conf_desc = {
	.config = {
		.bLength             = sizeof(usb_config_descriptor_t),
		.bDescriptorType     = USB_DESC_TYPE_CONFIG,
		.wTotalLength        = sizeof(cdc_config_descriptor_t),
		.bNumInterfaces      = 0x02,
		.bConfigurationValue = 0x01,
		.iConfiguration      = 0x00,
		.bmAttributes        = 0xC0, /* Self-Powered */
		.bMaxPower           = 0x32
	},
	.itf0 = {
		.bLength             = sizeof(usb_interface_descriptor_t),
		.bDescriptorType     = 0x04,
		.bInterfaceNumber    = 0x00,
		.bAlternateSetting   = 0x00,
		.bNumEndpoints       = 0x01,
		.bInterfaceClass     = 0x02, /* CDC Comm */
		.bInterfaceSubClass  = 0x02, /* ACM */
		.bInterfaceProtocol  = 0x01,
		.iInterface          = 0x00
	},
	.cdc_header = { 0x05, 0x24, 0x00, 0x10, 0x01 },
	.cdc_acm    = { 0x04, 0x24, 0x02, 0x02 },
	.cdc_union  = { 0x05, 0x24, 0x06, 0x00, 0x01 },
	.ep2_in = {
		.bLength             = sizeof(usb_endpoint_descriptor_t),
		.bDescriptorType     = 0x05,
		.bEndpointAddress    = 0x82,
		.bmAttributes        = 0x03, /* Interrupt */
		.wMaxPacketSize      = 0x0008,
		.bInterval           = 0x10
	},
	.itf1 = {
		.bLength             = sizeof(usb_interface_descriptor_t),
		.bDescriptorType     = 0x04,
		.bInterfaceNumber    = 0x01,
		.bAlternateSetting   = 0x00,
		.bNumEndpoints       = 0x02,
		.bInterfaceClass     = 0x0A, /* CDC Data */
		.bInterfaceSubClass  = 0x00,
		.bInterfaceProtocol  = 0x00,
		.iInterface          = 0x00
	},
	.ep1_out = {
		.bLength             = sizeof(usb_endpoint_descriptor_t),
		.bDescriptorType     = 0x05,
		.bEndpointAddress    = 0x01,
		.bmAttributes        = 0x02, /* Bulk OUT */
		.wMaxPacketSize      = 0x0020, /* 32 bytes to match physical bank size */
		.bInterval           = 0x00
	},
	.ep1_in = {
		.bLength             = sizeof(usb_endpoint_descriptor_t),
		.bDescriptorType     = 0x05,
		.bEndpointAddress    = 0x81,
		.bmAttributes        = 0x02, /* Bulk IN */
		.wMaxPacketSize      = 0x0040,
		.bInterval           = 0x00
	}
};

typedef struct __attribute__((packed)) { uint8_t bLength; uint8_t bDescriptorType; uint16_t wLangId; } str_lang_t;
typedef struct __attribute__((packed)) { uint8_t bLength; uint8_t bDescriptorType; uint16_t bString[8]; } str_mfr_t;
typedef struct __attribute__((packed)) { uint8_t bLength; uint8_t bDescriptorType; uint16_t bString[7]; } str_prod_t;

static const str_lang_t str_lang_desc = { sizeof(str_lang_t), USB_DESC_TYPE_STRING, 0x0409 };
static const str_mfr_t  str_mfr_desc  = { sizeof(str_mfr_t),  USB_DESC_TYPE_STRING, {'B','l','u','e','t','r','u','m'} };
static const str_prod_t str_prod_desc = { sizeof(str_prod_t), USB_DESC_TYPE_STRING, {'C','D','C',' ','A','C','M'} };

/* ==============================================================================
 * DRIVER STATE & HARDWARE BUFFERS (Volatile to prevent CPU caching over DMA)
 * ============================================================================== */
static uint8_t ep0_buf[64]    __attribute__((aligned(4)));
static uint8_t ep1_rx_buf[64] __attribute__((aligned(4)));
static uint8_t ep1_tx_buf[64] __attribute__((aligned(4)));
static uint8_t ep2_tx_buf[8]  __attribute__((aligned(4)));

/* USB EP1 Double-Buffering Hardware Bank Tracker */
static volatile uint8_t  g_rx_bank = 0;

/* Software Stream FIFOs */
static volatile uint8_t  g_cdc_rx_buf[BT_CDC_RX_BUF_SIZE];
static volatile uint16_t g_cdc_rx_head = 0;
static volatile uint16_t g_cdc_rx_tail = 0;

static volatile uint8_t  g_cdc_tx_buf[BT_CDC_TX_BUF_SIZE];
static volatile uint16_t g_cdc_tx_head = 0;
static volatile uint16_t g_cdc_tx_tail = 0;

/* Interrupt-driven sync flags */
static volatile bool     g_ep1_tx_busy  = false;
static volatile bool     g_ep1_rx_ready = false;

typedef enum {
	EP0_STAGE_SETUP,
	EP0_STAGE_DATA_OUT
} ep0_stage_t;

static volatile ep0_stage_t g_ep0_stage       = EP0_STAGE_SETUP;
static volatile bool        g_usb_configured  = false;

static bt_usb_setup_packet_t g_setup_pkt;
static uint8_t               g_pending_address = 0;

static usb_cdc_line_coding_t g_line_coding = {
	.dwDTERate   = 115200,
	.bCharFormat = 0,
	.bParityType = 0,
	.bDataBits   = 8
};

/* ==============================================================================
 * CORE FUNCTIONS (EXECUTED IN RAM)
 * ============================================================================== */

USB_FUNC
void bt_usb_init(void) {
	/* Reset Software FIFOs */
	g_cdc_rx_head  = 0;
	g_cdc_rx_tail  = 0;
	g_cdc_tx_head  = 0;
	g_cdc_tx_tail  = 0;
	g_ep1_tx_busy  = false;
	g_ep1_rx_ready = false;
	g_rx_bank      = 0; /* Reset Bank toggle */

	/* Global Bluetrum reset */
	USBCON0 = 0x20;
	USBCON1 = 0;

	/* Configure DMA base addresses */
	USBEP0ADR   = (uint32_t)ep0_buf;
	USBEP1RXADR = (uint32_t)ep1_rx_buf;
	USBEP1TXADR = (uint32_t)ep1_tx_buf;
	USBEP2TXADR = (uint32_t)ep2_tx_buf;

	/* Initialize EP0 (UINDEX = 0) */
	UINDEX = 0;
	UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */

	/* Configure EP1/EP2 descriptors in MUSB to quiet the bus */
	UINDEX = 1;
	UTXMAXP = 4;    /* 32 bytes (32 >> 3), is max on this chip */
	UTXCSR1 = 0x48; /* ClrDataTog | FlushFIFO */
	UTXCSR2 = 0x20; /* Mode = TX */
	UTXTYPE = 0x21; /* Protocol Bulk (0b10 << 4) | Target EP1 (need to figure out what to write there for non-Bulk endpoints) */

	URXMAXP = 4;    /* 32 bytes (32 >> 3), is max on this chip */
	URXCSR1 = 0x90; /* ClrDataTog | FlushFIFO */
	URXCSR2 = 0x00; /* Mode = RX */
	URXTYPE = 0x21; /* Protocol Bulk (0b10 << 4) | Target EP1 (need to figure out what to write there for non-Bulk endpoints) */

	UINDEX = 2;
	UTXMAXP = 1;    /* 8 bytes (8 >> 3) */
	UTXCSR1 = 0x48; /* ClrDataTog | FlushFIFO */
	UTXCSR2 = 0x20; /* Mode = TX */

	/* Restore UINDEX to 0 */
	UINDEX = 0;
	UFADDR = 0x80;

	/* Enable hardware interrupts */
	UINTRUSBE = 0x04; /* Bus Reset */
	UINTRTX1E = 0x03; /* Bit 0: EP0 Event | Bit 1: EP1 TX Done Event */
	UINTRRX1E = 0x02; /* Bit 1: EP1 RX Packet Ready Event */

	USBCON0 |= 0x04 | 0x02 | 0x78 | 0x01; 
	USBCON3 = 0x0f;
	USBCON1 |= 0x10000;

	/* Enable USB in Bluetrum's MMIO PIC */
	PICEN |= 0x80;
}

ISR_FUNC
static void bt_usb_ep0_tx(const void* data, uint16_t len) {
	if (len > 64) len = 64; 
	if (len > 0 && data) {
		memcpy((void*)ep0_buf, data, len);
		USBCON2 = len | 0x10000; /* Trigger EP0 DMA */
		while (USBCON2 & 0x10000); /* Wait for DMA to fill the hardware FIFO */
	}
	UINDEX = 0;
	UCSR0 = 0x0A; /* TxPktRdy (0x02) | DataEnd (0x08) */
}

ISR_FUNC
static void bt_usb_ep0_stall(void) { 
	UINDEX = 0;
	UCSR0 = 0x20; /* SendStall (Bit 5 in CSR0) */
}

USB_FUNC
static void bt_usb_ep1_kick(void) {
	uint16_t count = 0;

	/* Always fill from base, capped at the 32-byte FIFO limit */
	while ((g_cdc_tx_head != g_cdc_tx_tail) && (count < 32)) {
		ep1_tx_buf[count++] = g_cdc_tx_buf[g_cdc_tx_tail];
		g_cdc_tx_tail = (g_cdc_tx_tail + 1) & (BT_CDC_TX_BUF_SIZE - 1);
	}

	if (count == 0) {
		g_ep1_tx_busy = false;
		return;
	}

	uint32_t pic = PICEN;
	PICEN = pic & ~0x80;

	uint32_t saved_idx = UINDEX;
	UINDEX = 1;

	/* DMA always reads from base */
	USBEP1TXADR = (uint32_t)ep1_tx_buf;
	USBCON2 = count | 0x20000; 

	/* Wait for DMA to fill the FIFO */
	while (USBCON2 & 0x20000); 

	/* Tell MUSB the packet is ready */
	UTXCSR1 = 0x01;

	UINDEX = saved_idx;
	PICEN = pic;

	/* Tiny bus arbitration barrier matching BootROM delay(3) (~10-15 cycles) */
	for (volatile int d = 0; d < 6; d++);
}

ISR_FUNC
void bt_usb_isr(void) {
	uint32_t saved_idx = UINDEX;

	/* Reading MUSB interrupt registers clears them automatically */
	uint32_t flags_usb = UINTRUSB;
	uint32_t flags_tx  = UINTRTX1;
	uint32_t flags_rx  = UINTRRX1;

	/* USB Bus Reset */
	if (flags_usb & 0x04) {
		UFADDR = 0x80;
		UINDEX = 0;
		UCSR0 = 0x48;
		
		g_pending_address = 0;
		g_ep0_stage       = EP0_STAGE_SETUP;
		g_usb_configured  = false;

		g_ep1_tx_busy     = false;
		g_ep1_rx_ready    = false;
		g_rx_bank         = 0; /* Always start on Bank 0 after reset */
		g_cdc_rx_head     = 0;
		g_cdc_rx_tail     = 0;
		g_cdc_tx_head     = 0;
		g_cdc_tx_tail     = 0;

		UINTRUSB       = flags_usb; /* Clear flag */
		UINDEX = saved_idx;
		return;
	}

	/* EP1 RX Packet Ready */
	if (flags_rx & 0x02) {
		UINDEX = 1;
		if (URXCSR1 & 0x01) {
			int rx_count = URXCOUNT1;
			if (rx_count > 32) rx_count = 32;

			/* Select bank based on global tracking */
			volatile uint8_t *src = (g_rx_bank == 0) ? (ep1_rx_buf + 0x00) 
													 : (ep1_rx_buf + 0x20);
			g_rx_bank ^= 1;

			for (int i = 0; i < rx_count; i++) {
				uint16_t next = (g_cdc_rx_head + 1) & (BT_CDC_RX_BUF_SIZE - 1);
				if (next != g_cdc_rx_tail) {
					g_cdc_rx_buf[g_cdc_rx_head] = src[i];
					g_cdc_rx_head = next;
				}
			}

			USBEP1RXADR = (uint32_t)ep1_rx_buf;
			URXCSR1 = 0x10;
		}
	}

	/* EP1 TX Packet Completion (Host ACKed previous packet) */
	if (flags_tx & 0x02) {
		bt_usb_ep1_kick();
	}

	/* EP0 Control Event */
	if (flags_tx & 0x01) {
		/* Latch pending address after status handshake finishes */
		if (g_pending_address) {
			UFADDR = g_pending_address;
			g_pending_address = 0;
		}

		UINDEX = 0;
		if (UCSR0 & 0x01) { /* RxPktRdy */
			int count = UCOUNT0;
			
			/* 1. Extract data from DMA buffer (already filled by hardware) */
			if (g_ep0_stage == EP0_STAGE_DATA_OUT) {
				if (count > 0) {
					memcpy(&g_line_coding, (void*)ep0_buf, count > (int)sizeof(g_line_coding) ? sizeof(g_line_coding) : (size_t)count);
				}
			} else {
				if (count >= 8) {
					memcpy(&g_setup_pkt, (void*)ep0_buf, 8);
				}
			}

			/* 2. Pop UFIFO0 to physically clear the hardware FIFO *BEFORE* clearing RxPktRdy */
			int pop_count = count;
			while (pop_count > 0) {
				(void)UFIFO0;
				pop_count--;
			}

			/* 3. Advance state machine by writing to UCSR0 */
			if (g_ep0_stage == EP0_STAGE_DATA_OUT) {
				UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */
				g_ep0_stage = EP0_STAGE_SETUP;
			} else {
				if (count >= 8) {
					uint8_t req_type = g_setup_pkt.bmRequestType;
					uint8_t req      = g_setup_pkt.bRequest;

					if ((req_type & 0x60) == 0x00 && req == USB_REQ_SET_ADDRESS) {
						g_pending_address = (g_setup_pkt.wValue & 0x7F) | 0x80;
						UCSR0 = 0x48;
					}
					else if ((req_type & 0x60) == 0x00 && req == USB_REQ_SET_CONFIGURATION) {
						g_usb_configured = true;
						g_rx_bank        = 0; /* Reset Bank toggle to 0 */
						UCSR0 = 0x48;
						UINDEX = 1;
						URXCSR1 = 0x10;
						UINDEX = 0; /* Restore UINDEX to EP0 */
					}
					else if ((req_type & 0x60) == 0x20 && req == 0x20) { /* SET_LINE_CODING */
						UCSR0 = 0x40; /* ServicedRxPktRdy ONLY */
						g_ep0_stage = EP0_STAGE_DATA_OUT;
					}
					else if ((req_type & 0x60) == 0x20 && req == 0x22) { /* SET_CTRL_LINE_STATE */
						UCSR0 = 0x48;
					}
					else {
						/* ---- Handle IN-Data Transfers immediately ---- */
						if ((req_type & 0x60) == 0x00) { /* Standard Requests */
							switch (req) {
								case USB_REQ_GET_STATUS: {
									uint16_t status = 0x0001; /* Device: Self-Powered */
									bt_usb_ep0_tx(&status, 2);
									break;
								}
								case USB_REQ_GET_DESCRIPTOR: {
									uint8_t desc_type = (g_setup_pkt.wValue >> 8);
									uint8_t desc_idx  = (g_setup_pkt.wValue & 0xFF);
									const uint8_t* ptr = NULL;
									uint16_t len = 0;

									if (desc_type == USB_DESC_TYPE_DEVICE) {
										ptr = (const uint8_t*)&dev_desc;
										len = sizeof(dev_desc);
									} else if (desc_type == USB_DESC_TYPE_CONFIG) {
										ptr = (const uint8_t*)&conf_desc;
										len = sizeof(conf_desc);
									} else if (desc_type == USB_DESC_TYPE_STRING) {
										if (desc_idx == 0)      { ptr = (const uint8_t*)&str_lang_desc; len = sizeof(str_lang_desc); }
										else if (desc_idx == 1) { ptr = (const uint8_t*)&str_mfr_desc;  len = sizeof(str_mfr_desc);  }
										else if (desc_idx == 2) { ptr = (const uint8_t*)&str_prod_desc; len = sizeof(str_prod_desc); }
									}

									if (ptr) {
										if (len > g_setup_pkt.wLength) len = g_setup_pkt.wLength;
										bt_usb_ep0_tx(ptr, len); 
									} else {
										bt_usb_ep0_stall();
									}
									break;
								}
								default:
									bt_usb_ep0_stall();
									break;
							}
						} else if ((req_type & 0x60) == 0x20) {
							if (req == 0x21) { /* GET_LINE_CODING */
								bt_usb_ep0_tx(&g_line_coding, sizeof(g_line_coding));
							} else {
								bt_usb_ep0_stall();
							}
						} else {
							bt_usb_ep0_stall();
						}
					}
				}
			}
		}
	}

	UINDEX = saved_idx;
}

/* ==============================================================================
 * CDC ACM APPLICATION API
 * ============================================================================== */

USB_FUNC
static inline bool bt_cdc_is_connected(void) {
	return g_usb_configured;
}

USB_FUNC
void bt_cdc_write_char(char c) {
	if (!g_usb_configured) return;
	uint32_t timeout = BT_CDC_TX_TIMEOUT;

	/* Wait if software ring buffer is full */
	while (((g_cdc_tx_head + 1) & (BT_CDC_TX_BUF_SIZE - 1)) == g_cdc_tx_tail) {
		if (--timeout == 0) return;
	}

	/* Push character and advance head */
	g_cdc_tx_buf[g_cdc_tx_head] = (uint8_t)c;
	g_cdc_tx_head = (g_cdc_tx_head + 1) & (BT_CDC_TX_BUF_SIZE - 1);

	/* Claim hardware and kickstart if sleeping */
	if (!g_ep1_tx_busy) {
		g_ep1_tx_busy = true;
		bt_usb_ep1_kick();
	}
}

USB_FUNC
void bt_cdc_write(const void* data, size_t len) {
	if (!g_usb_configured || len == 0) return;

	const uint8_t* ptr = (const uint8_t*)data;
	uint32_t timeout = BT_CDC_TX_TIMEOUT;

	while (len > 0) {
		uint16_t head = g_cdc_tx_head;
		uint16_t tail = *(volatile uint16_t*)&g_cdc_tx_tail; 
		uint16_t free_space = (tail - head - 1) & (BT_CDC_TX_BUF_SIZE - 1);

		if (free_space == 0) {
			if (--timeout == 0) return;
			continue;
		}

		uint16_t chunk = (len < free_space) ? len : free_space;
		for (uint16_t i = 0; i < chunk; i++) {
			g_cdc_tx_buf[head] = *ptr++;
			head = (head + 1) & (BT_CDC_TX_BUF_SIZE - 1);
		}
		g_cdc_tx_head = head;

		if (!g_ep1_tx_busy) {
			g_ep1_tx_busy = true;
			bt_usb_ep1_kick();
		}

		len -= chunk;
		timeout = BT_CDC_TX_TIMEOUT;
	}
}

USB_FUNC
int bt_cdc_read_char(void) {
	/* Lock-free empty check */
	if (g_cdc_rx_head == g_cdc_rx_tail) {
		return -1;
	}

	uint8_t ch = g_cdc_rx_buf[g_cdc_rx_tail];
	g_cdc_rx_tail = (g_cdc_rx_tail + 1) & (BT_CDC_RX_BUF_SIZE - 1);
	return (int)ch;
}

#endif /* BLUETRUM_USB_H */
