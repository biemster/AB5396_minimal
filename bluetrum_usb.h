/**
 * @file bluetrum_usb.h
 * @brief Stable Hybrid USB Stack for Bluetrum AB5396 (MUSB HDRC Core)
 *        - Lean tick function (1 cycle when idle)
 *        - DMA executed in thread context to avoid bus arbitration deadlock
 */

#ifndef BLUETRUM_USB_H
#define BLUETRUM_USB_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* ==============================================================================
 * SECTION ATTRIBUTES (RAM EXECUTION)
 * ============================================================================== */
#define ISR_FUNC     __attribute__((section(".isr")))
#define USB_FUNC     __attribute__((section(".usb")))

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
		.wMaxPacketSize      = 0x0040,
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
 * DRIVER STATE & BUFFERS
 * ============================================================================== */
static uint8_t ep0_buf[64]    __attribute__((aligned(4)));
static uint8_t ep1_rx_buf[64] __attribute__((aligned(4)));
static uint8_t ep1_tx_buf[64] __attribute__((aligned(4)));
static uint8_t ep2_tx_buf[8]  __attribute__((aligned(4)));

typedef enum {
	EP0_STAGE_SETUP,
	EP0_STAGE_DATA_OUT
} ep0_stage_t;

static volatile ep0_stage_t g_ep0_stage       = EP0_STAGE_SETUP;
static volatile bool        g_ep0_setup_ready = false;
static volatile bool        g_ep0_data_ready  = false;
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
static void bt_usb_ep0_tx(const void* data, uint16_t len) {
	if (len > 64) len = 64; 
	if (len > 0 && data) {
		memcpy(ep0_buf, data, len);
		USBCON2 = len | 0x10000; /* Trigger EP0 DMA */
	}
	UINDEX = 0;
	UCSR0 = 0x0A; /* TxPktRdy (0x02) | DataEnd (0x08) */
}

USB_FUNC
static void bt_usb_ep0_stall(void) { 
	UINDEX = 0;
	UCSR0 = 0x20; /* SendStall (Bit 5 in CSR0) */
}

USB_FUNC
void bt_usb_init(void) {
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
	UTXMAXP = 8;    /* 64 bytes (64 >> 3) */
	UTXCSR1 = 0x48; /* ClrDataTog | FlushFIFO */
	UTXCSR2 = 0x20; /* Mode = TX */

	URXMAXP = 8;    /* 64 bytes (64 >> 3) */
	URXCSR1 = 0x90; /* ClrDataTog | FlushFIFO */
	URXCSR2 = 0x00; /* Mode = RX */

	UINDEX = 2;
	UTXMAXP = 1;    /* 8 bytes (8 >> 3) */
	UTXCSR1 = 0x48; /* ClrDataTog | FlushFIFO */
	UTXCSR2 = 0x20; /* Mode = TX */

	/* Restore UINDEX to 0 */
	UINDEX = 0;
	UFADDR = 0x80;

	/* Enable hardware interrupts */
	UINTRUSBE = 0x04; /* Bus Reset */
	UINTRTX1E = 0x01; /* EP0 Event */

	USBCON0 |= 0x04 | 0x02 | 0x78 | 0x01; 
	USBCON3 = 0x0f;
	USBCON1 |= 0x10000;

	/* Enable USB in Bluetrum's MMIO PIC */
	PICEN |= 0x80;
}

ISR_FUNC
void bt_usb_isr(void) {
	uint32_t saved_idx = UINDEX;

	uint32_t flags_usb = UINTRUSB; /* 0x218 */
	uint32_t flags_tx  = UINTRTX1; /* 0x208 */
	uint32_t flags_rx  = UINTRRX1; /* 0x210 */

	/* USB Bus Reset */
	if (flags_usb & 0x04) {
		UFADDR = 0x80;
		UINDEX = 0;
		UCSR0 = 0x48;
		g_pending_address = 0;
		g_ep0_stage       = EP0_STAGE_SETUP;
		g_usb_configured  = false;
		UINTRUSB       = flags_usb; /* Clear flag */
		UINDEX         = saved_idx;
		return;
	}

	/* EP0 Control Event */
	if (flags_tx & 0x01) {
		/* Latch pending address after status handshake finishes */
		if (g_pending_address) {
			UFADDR = g_pending_address;
			g_pending_address = 0;
		}

		UINDEX = 0;
		uint32_t csr0 = UCSR0;

		if (csr0 & 0x01) { /* RxPktRdy */
			int count = UCOUNT0;

			if (g_ep0_stage == EP0_STAGE_DATA_OUT) {
				if (count > 0) {
					memcpy(&g_line_coding, ep0_buf, count > (int)sizeof(g_line_coding) ? sizeof(g_line_coding) : (size_t)count);
				}
				g_ep0_data_ready = true;
			} else {
				if (count >= 8) {
					memcpy(&g_setup_pkt, ep0_buf, 8);
					g_ep0_setup_ready = true;
				}
			}

			/* Critical bootrom quirk: ALWAYS pop all bytes from UFIFO0 in both stages */
			while (count > 0) {
				(void)UFIFO0;
				count--;
			}
		}
		UINTRTX1 = flags_tx;
	}

	if (flags_rx) {
		UINTRRX1 = flags_rx;
	}

	UINDEX = saved_idx;
}

USB_FUNC
void bt_usb_tick(void) {
	/* Auto-drain EP1 OUT: immediately ACKs escape sequences from host on screen exit */
	if (g_usb_configured) {
		/* Protect UINDEX from ISR preemption */
		uint32_t pic = PICEN;
		PICEN = pic & ~0x80;
		uint32_t saved_idx = UINDEX;

		UINDEX = 1;
		if (URXCSR1 & 0x01) { /* RxPktRdy */
			int rx_count = URXCOUNT1;
			while (rx_count > 0) {
				(void)UFIFO1;
				rx_count--;
			}
			/* Re-arm EP1 RX DMA to accept and ACK subsequent packets */
			USBEP1RXADR = (uint32_t)ep1_rx_buf;
			URXCSR1 = 0x10;
		}

		UINDEX = saved_idx;
		PICEN = pic;
	}

	/* Handle second-stage OUT data for SET_LINE_CODING */
	if (g_ep0_data_ready) {
		g_ep0_data_ready = false;
		if (g_ep0_stage == EP0_STAGE_DATA_OUT) {
			UINDEX = 0;
			UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */
			g_ep0_stage = EP0_STAGE_SETUP;
		}
		return;
	}

	/* Handle incoming SETUP packets (Single-cycle check when idle) */
	if (!g_ep0_setup_ready) return;
	g_ep0_setup_ready = false;

	uint8_t req_type = g_setup_pkt.bmRequestType;
	uint8_t req      = g_setup_pkt.bRequest;

	if ((req_type & 0x60) == 0x00) { /* Standard Requests */
		switch (req) {
			case USB_REQ_GET_STATUS: {
				uint16_t status = 0x0000;
				uint8_t recipient = req_type & 0x1F;
				if (recipient == 0x00) {
					status = 0x0001; /* Device: Self-Powered */
				}
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

			case USB_REQ_SET_ADDRESS:
				g_pending_address = (g_setup_pkt.wValue & 0x7F) | 0x80;
				UINDEX = 0;
				UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */
				break;

			case USB_REQ_SET_CONFIGURATION:
				g_usb_configured = true;
				UINDEX = 0;
				UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */

				/* Arm EP1 RX so incoming data is ACKed immediately */
				USBEP1RXADR = (uint32_t)ep1_rx_buf;
				UINDEX = 1;
				URXCSR1 = 0x10;
				break;

			default:
				bt_usb_ep0_stall();
				break;
		}
	} else if ((req_type & 0x60) == 0x20) { /* CDC Class Requests */
		if (req == 0x20) { /* SET_LINE_CODING: Expect 7 bytes of OUT data */
			UINDEX = 0;
			UCSR0 = 0x40; /* ServicedRxPktRdy ONLY */
			g_ep0_stage = EP0_STAGE_DATA_OUT;
		} else if (req == 0x21) { /* GET_LINE_CODING: Send 7 bytes */
			bt_usb_ep0_tx(&g_line_coding, sizeof(g_line_coding));
		} else if (req == 0x22) { /* SET_CONTROL_LINE_STATE: No data stage */
			UINDEX = 0;
			UCSR0 = 0x48; /* ServicedRxPktRdy | DataEnd */
		} else {
			bt_usb_ep0_stall();
		}
	} else {
		bt_usb_ep0_stall();
	}
}

#endif /* BLUETRUM_USB_H */
