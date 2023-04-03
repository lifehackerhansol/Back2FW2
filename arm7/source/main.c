/*
	Back2FW2

	Copyright (C) 2010-2015 Taiju Yamada
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: MIT
*/

#include <nds.h>

#include "fifoChannels.h"

#define REG_SNDEXCNT (*(vu16*)0x4004700)

extern void resetRudolph();

void VblankHandler(void) {
}

void ReturntoDSiMenu() {
	if (isDSiMode()) {
		i2cWriteRegister(0x4A, 0x70, 0x01);		// Bootflag = Warmboot/SkipHealthSafety
		i2cWriteRegister(0x4A, 0x11, 0x01);		// Reset to DSi Menu
	} else {
		u8 readCommand = readPowerManagement(0x10);
		readCommand |= BIT(0);
		writePowerManagement(0x10, readCommand);
	}
}

int main(void) {
	// Reset the clock if needed
	rtcReset();

	irqInit();
	fifoInit();

	installSystemFIFO();

	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable(IRQ_VBLANK);

	int isRegularDS = 1; 
	if (REG_SNDEXCNT != 0) isRegularDS = 0; // If sound frequency setting is found, then the console is not a DS Phat/Lite
	fifoSendValue32(FIFO_RETURN, isRegularDS);

	while (true) {
		swiWaitForVBlank();
		if(fifoCheckValue32(FIFO_CONTROL)) {
			u32 dumpOption = fifoGetValue32(FIFO_CONTROL);
			if(dumpOption == B2FW2_DUMP_FW) {
				u32 ret = 0;
				u32 mailAddr = fifoGetValue32(FIFO_BUFFER_ADDR);
				u32 mailSize = fifoGetValue32(FIFO_BUFFER_SIZE);
				readFirmware(0, (void *)mailAddr, mailSize);
				ret = 524288;
				fifoSendValue32(FIFO_RETURN, ret);
			} 
			else if(dumpOption == B2FW2_RETURN_DSI) ReturntoDSiMenu();
			else if(dumpOption == B2FW2_RETURN_NDS) resetRudolph();
		}
	}
	return 0;
}
