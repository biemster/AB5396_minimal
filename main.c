#include <stdint.h>

// ---------------------------------------------------------
// The 8-byte Command Packet sent from Python (via pack_cmd)
// ---------------------------------------------------------
// NOTE: Sent Big-Endian over the wire, but mapped into memory as raw bytes.
// To use arg1 and arg3, you will need to swap endianness, e.g., __builtin_bswap32(cmd->arg1)
struct __attribute__((packed)) cmd_packet {
	uint8_t  cmd_id;       // Offset 0x00 (e.g., 0x00 for INIT, 0x01 for READ)
	uint32_t arg1;         // Offset 0x01 - 0x04 
	uint8_t  arg2;         // Offset 0x05
	uint16_t arg3;         // Offset 0x06 - 0x07 
};

// ---------------------------------------------------------
// The Master BootROM Context passed as ctx (a0) into entry()
// ---------------------------------------------------------
struct BootROM_Context {
	// [0x00] Command Response Buffer:
	// Pre-allocated by the BootROM. Used to send short variables back 
	// to the PC, like the 24-byte CodeKey/FlashID response.
	uint32_t *cmd_resp_buf;      

	// [0x04] Incoming Command Frame:
	// Direct pointer to the deframed packet sent from Python.
	struct cmd_packet *cmd;  

	// [0x08] Configuration Flags 1:
	// Likely Interface state flags (UART vs USB, connection speeds).
	// The factory loader reads these individually as bytes and packs them into a 32-bit int.
	uint8_t  flags_1[4];         
	
	// [0x0C - 0x1F] Unknown / Padding
	uint32_t _pad1[5];           
	
	// [0x20] Unknown Word
	uint32_t _pad2;              
	
	// [0x24] Max Block Size (uint16)
	// The factory loader reads this as a 16-bit halfword (lhu). 
	// It is almost certainly the negotiated USB/UART block size (e.g., 512 or 4096).
	uint16_t block_size;         

	// [0x26 - 0x2B] Unknown configuration bytes
	uint8_t  unk_26;             
	uint8_t  unk_27;             
	uint8_t  unk_28;             
	uint8_t  _pad3[3];           
	
	// [0x2C - 0x2F] Configuration Flags 2:
	// Read and packed similarly to flags_1.
	uint8_t  flags_2[4];         
	
	// [0x30 - 0x3B] Unknown / Padding
	uint32_t _pad4[3];           

	// [0x3C] Bulk Data Buffer:
	// This is incredibly important. When Python sends a MEM_WRITE block, 
	// the payload data lives here. When Python asks for a MEM_READ block, 
	// you put the Flash data here before transmitting!
	uint32_t *bulk_buf;          

	// [0x40] Boot Config (uint32)
	// Used during the UART baud-rate switch initialization routine.
	uint32_t unk_40;             
};

// ---------------------------------------------------------
// The BootROM TX function
// ---------------------------------------------------------
// Takes EXACTLY ONE argument: the payload length in a0!
typedef void (*rom_uart_send_packet_t)(uint32_t length);
#define ROM_UART_SEND_PACKET ((rom_uart_send_packet_t)0x000800a4)

int entry(struct BootROM_Context *ctx) {
	if (!ctx || !ctx->cmd) return 0;
	
	if (ctx->cmd->cmd_id == 0x00) { // INIT
		
		ctx->cmd_resp_buf[0] = 0x11223344; // Dummy Code key
		ctx->cmd_resp_buf[1] = 0x00AABBCC; // Dummy Flash ID
		
		ctx->cmd_resp_buf[2] = 0x00112233;
		ctx->cmd_resp_buf[3] = 0x44556677;
		ctx->cmd_resp_buf[4] = 0x8899AABB;
		ctx->cmd_resp_buf[5] = 0xCCDDEEFF;

		// Tell the BootROM to frame and send 24 bytes out of cmd_resp_buf
		ROM_UART_SEND_PACKET(24);
	}

	return 0;
}
