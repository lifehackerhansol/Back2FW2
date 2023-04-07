/*
	Back2FW2

	Copyright (C) 2010-2015 Taiju Yamada
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: MIT
*/

#include <stdio.h>
#include <string.h>

#include <nds.h>
#include <fat.h>

#include "fifoChannels.h"
#include "fw_operation.h"
#include "mshlreset.h"
#include "util.h"

#define FIRMWARE_SIZE 524288

bool isRegularDS = false;

static u8		*tmp_data9;
static u8		*tmp_data7;
static u32		size9, size7;
static u32		ARM9bootAddr;
static u32		ARM7bootAddr;
static bool		patched;

typedef void (*type_u32p)(u32*);

#define FILLBUF if(!(xIn&7))memcpy(curBlock,in+xIn,8),operation(curBlock);
static u32 process(const u8 *in, u8** _out, type_u32p operation)
{
	u32 curBlock[2] = { 0 };
	u32 blockSize = 0;
	u32 xLen = 0;

	u32 i = 0, j = 0;
	u32 xIn = 0, xOut = 0;
	u32 len = 0;
	u32 offset = 0;
	u32 windowOffset = 0;
	u8 d = 0;
	u16 data = 0;

	FILLBUF
	xIn=4;
	blockSize = curBlock[0] >> 8;

	if(!blockSize)return 0;

	*_out = (u8*)malloc(blockSize);
	if(!*_out)return 0;
	u8 *out=*_out;
	memset(out, 0xFF, blockSize);

	xLen = blockSize;
	while(xLen > 0)
	{
		d = ((u8*)(curBlock))[xIn&7];
		xIn++;
		FILLBUF

		for(i = 0; i < 8; i++)
		{
			if(d & 0x80)
			{
				data = ((u8*)(curBlock))[xIn&7] << 8;
				xIn++;
				FILLBUF
				data |= ((u8*)(curBlock))[xIn&7];
				xIn++;
				FILLBUF

				len = (data >> 12) + 3;
				offset = (data & 0xFFF);
				windowOffset = (xOut - offset - 1);

				for(j = 0; j < len; j++){
					out[xOut]=out[windowOffset];
					xOut++;
					windowOffset++;

					xLen--;
					if(!xLen)return blockSize;
				}
			}
			else
			{
				out[xOut]=((u8*)(curBlock))[xIn&7];
				xOut++;
				xIn++;
				FILLBUF

				xLen--;
				if(!xLen)return blockSize;
			}

			d = ((d << 1) & 0xFF);
		}
	}
	
	return blockSize;
}

static u16 getBootCodeCRC16(){
	u16 crc=0xffff;
	crc=swiCRC16(crc,tmp_data9,size9);
	crc=swiCRC16(crc,tmp_data7,size7);
	return crc;
}

void ret_menu9_GENs(void);

static int bootDSFirmware(u8 *data){
	u16 shift1 = 0, shift2 = 0, shift3 = 0, shift4 = 0;
	u32 part1addr = 0, part2addr = 0, part3addr = 0, part4addr = 0, part5addr = 0;
	u32 part1ram = 0, part2ram = 0;

	firmware_header_t *header = (firmware_header_t*)malloc(sizeof(firmware_header_t));

	memcpy(header, data, sizeof(firmware_header_t));

	if ((header->fw_identifier[0] != 'M') ||
		(header->fw_identifier[1] != 'A') ||
		(header->fw_identifier[2] != 'C')) 
	{
		free(data);
		free(header);
		return 4;
	}

	// only 12bits are used
	shift1 = ((header->shift_amounts >> 0) & 0x07);
	shift2 = ((header->shift_amounts >> 3) & 0x07);
	shift3 = ((header->shift_amounts >> 6) & 0x07);
	shift4 = ((header->shift_amounts >> 9) & 0x07);

	part1addr = (header->part1_rom_boot9_addr << (2 + shift1));
	part1ram = (0x02800000 - (header->part1_ram_boot9_addr << (2+shift2)));
	part2addr = (header->part2_rom_boot7_addr << (2+shift3));
	part2ram = (((header->shift_amounts&0x1000)?0x02800000:0x03810000) - (header->part2_ram_boot7_addr << (2+shift4)));
	part3addr = (header->part3_rom_gui9_addr << 3);
	part4addr = (header->part4_rom_wifi7_addr << 3);
	part5addr = (header->part5_data_gfx_addr << 3);

	ARM9bootAddr = part1ram;
	ARM7bootAddr = part2ram;

	initKeycode(read32(data+8), 1);
	crypt64BitDown((u32*)&data[0x18]);

	initKeycode(read32(data+8), 2);

	size9 = process(data + part1addr, &tmp_data9, crypt64BitDown);
	if (!tmp_data9){
		free(data);
		free(header);
		return 3;
	}

	size7 = process(data + part2addr, &tmp_data7, crypt64BitDown);
	if (!tmp_data7){
		free(tmp_data9);
		free(data);
		free(header);
		return 3;
	}

	u16 crc16_mine = getBootCodeCRC16();

	if (crc16_mine != header->part12_boot_crc16){
		iprintf("Firmware: ERROR: the boot code CRC16 (0x%04X) doesn't match the value in the firmware header (0x%04X)", crc16_mine, header->part12_boot_crc16);
		free(tmp_data7);
		free(tmp_data9);
		free(data);
		free(header);
		return 4;
	}

	iprintf("Firmware:\n");
	iprintf("- CRC : 0x%04X\n", header->part12_boot_crc16);
	iprintf("- header: \n");
	iprintf("   * size firmware %i\n", ((header->shift_amounts >> 12) & 0xF) * 128 * 1024);
	iprintf("   * ARM9 boot code address:     0x%08lX\n", part1addr);
	iprintf("   * ARM9 boot code RAM address: 0x%08lX\n", ARM9bootAddr);
	iprintf("   * ARM9 unpacked size:         0x%08lX (%ld) bytes\n", size9, size9);
	iprintf("   * ARM9 GUI code address:      0x%08lX\n", part3addr);
	iprintf("\n");
	iprintf("   * ARM7 boot code address:     0x%08lX\n", part2addr);
	iprintf("   * ARM7 boot code RAM address: 0x%08lX\n", ARM7bootAddr);
	iprintf("   * ARM7 WiFi code address:     0x%08lX\n", part4addr);
	iprintf("   * ARM7 unpacked size:         0x%08lX (%ld) bytes\n", size7, size7);
	iprintf("\n");
	iprintf("   * Data/GFX address:           0x%08lX\n", part5addr);

	patched = false;
	if(data[0x17C] != 0xFF)patched = true;

	if(patched){
		free(tmp_data7);
		free(tmp_data9);

		u32 patch_offset = 0x3FC80;
		if (data[0x17C] > 1)
			patch_offset = 0x3F680;

		memcpy(&header, data + patch_offset, sizeof(header));

		shift1 = ((header->shift_amounts >> 0) & 0x07);
		shift2 = ((header->shift_amounts >> 3) & 0x07);
		shift3 = ((header->shift_amounts >> 6) & 0x07);
		shift4 = ((header->shift_amounts >> 9) & 0x07);

		part1addr = (header->part1_rom_boot9_addr << (2 + shift1));
		part1ram = (0x02800000 - (header->part1_ram_boot9_addr << (2+shift2)));
		part2addr = (header->part2_rom_boot7_addr << (2+shift3));
		part2ram = (((header->shift_amounts&0x1000)?0x02800000:0x03810000) - (header->part2_ram_boot7_addr << (2+shift4)));

		ARM9bootAddr = part1ram;
		ARM7bootAddr = part2ram;

		size9 = process(data + part1addr, &tmp_data9, noop);
		if (!tmp_data9){
			free(data);
			free(header);
			return 3;
		}

		size7 = process(data + part2addr, &tmp_data7, noop);
		if (!tmp_data7){
			free(tmp_data9);
			free(data);
			free(header);
			return 3;
		};

		iprintf("\nFlashme:\n");
		iprintf("- header: \n");
		iprintf("   * ARM9 boot code address:     0x%08lX\n", part1addr);
		iprintf("   * ARM9 boot code RAM address: 0x%08lX\n", ARM9bootAddr);
		iprintf("   * ARM9 unpacked size:         0x%08lX (%ld) bytes\n", size9, size9);
		iprintf("\n");
		iprintf("   * ARM7 boot code address:     0x%08lX\n", part2addr);
		iprintf("   * ARM7 boot code RAM address: 0x%08lX\n", ARM7bootAddr);
		iprintf("   * ARM7 unpacked size:         0x%08lX (%ld) bytes\n", size7, size7);
	}

	free(data);
	free(header);
	u32 pad9=0x100-(size9&0xff);
	u32 pad7=0x100-(size7&0xff);

	u8 *pFileBuf=(u8*)malloc(0x200+size9+size7);

	//Build NDS image from tmp_dataX and sizeX
	memcpy(pFileBuf,ndshead,512);
	write32(pFileBuf+0x24,ARM9bootAddr);
	write32(pFileBuf+0x28,ARM9bootAddr);
	write32(pFileBuf+0x2c,size9/*+pad9*/);
	write32(pFileBuf+0x30,size9+pad9+0x200);
	write32(pFileBuf+0x34,ARM7bootAddr);
	write32(pFileBuf+0x38,ARM7bootAddr);
	write32(pFileBuf+0x3c,size7/*+pad7*/);
	write32(pFileBuf+0x80,0x200+size9+pad9+size7+pad7);
	write16(pFileBuf+0x15e,swiCRC16(0xffff,pFileBuf,0x15e));
	memcpy(pFileBuf+0x200,tmp_data9,size9);
	memcpy(pFileBuf+0x200+size9,tmp_data7,size7);

	iprintf("Press START to reboot.\n");
	while(true) {
		swiWaitForVBlank();
		scanKeys();
		if(keysDown() & KEY_START) break;
	}
	iprintf("Rebooting...\n");
	*(vu32*)0x023FFDF4=(u32)pFileBuf;
	DC_FlushAll();
	fifoSendValue32(FIFO_CONTROL, B2FW2_RETURN_NDS);
	ret_menu9_GENs();

	return 0;
}

int returnDSMenu(void) {
	if(!isRegularDS){
		iprintf("Rebooting to DSi Menu...\n");
		fifoSendValue32(FIFO_CONTROL, B2FW2_RETURN_DSI);
		while(1) swiWaitForVBlank();
	}
	u8 *p = (u8*)malloc(FIRMWARE_SIZE);
	if(!p) return 3;
	iprintf("Using GPL-free decoder.\n");
	iprintf("Getting Firmware...\n");
	fifoSendValue32(FIFO_BUFFER_ADDR, (u32)p);
	fifoSendValue32(FIFO_BUFFER_SIZE, FIRMWARE_SIZE);
	DC_FlushAll();
	fifoSendValue32(FIFO_CONTROL, B2FW2_DUMP_FW);
	fifoWaitValue32(FIFO_RETURN);
	fifoGetValue32(FIFO_RETURN);
	DC_InvalidateAll();
	iprintf("Decoding Firmware...\n");
	return bootDSFirmware(p);
}

int main(void) {

	consoleDemoInit();  //setup the sub screen for printing

	iprintf("Back2FW2\n");
	iprintf("Waiting for ARM7...\n");

	// First FIFO message sent from ARM7 will be whether the device is a regular DS or not.
	fifoWaitValue32(FIFO_RETURN);
	isRegularDS = fifoGetValue32(FIFO_RETURN);
	int ret = returnDSMenu();
	iprintf("Something failed... error %d\n", ret);
	while(1) {
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() & KEY_START) break;
	}

	return 0;
}
