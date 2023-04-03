/*
	Back2FW2

	Copyright (C) 2010-2015 Taiju Yamada
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: MIT
*/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <nds.h>

typedef struct {
	u16	part3_rom_gui9_addr;		// 000h
	u16	part4_rom_wifi7_addr;		// 002h
	u16	part34_gui_wifi_crc16;		// 004h
	u16	part12_boot_crc16;			// 006h
	u8	fw_identifier[4];			// 008h
	u16	part1_rom_boot9_addr;		// 00Ch
	u16	part1_ram_boot9_addr;		// 00Eh
	u16	part2_rom_boot7_addr;		// 010h
	u16	part2_ram_boot7_addr;		// 012h
	u16	shift_amounts;				// 014h
	u16	part5_data_gfx_addr;		// 016h

	u8	fw_timestamp[5];			// 018h
	u8	console_type;				// 01Dh
	u16	unused1;					// 01Eh
	u16	user_settings_offset;		// 020h
	u16	unknown1;					// 022h
	u16	unknown2;					// 024h
	u16	part5_crc16;				// 026h
	u16	unused2;					// 028h	- FFh filled 
} firmware_header_t;

void noop(u32 *ptr);
bool initKeycode(u32 idCode, int level);
void crypt64BitDown(u32 *ptr);

#ifdef __cplusplus
}
#endif
