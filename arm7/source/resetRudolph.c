/*
	Back2FW2

	Copyright (C) 2010-2015 Taiju Yamada
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: MIT
*/

#include <nds.h>

extern __attribute__((noreturn)) void _menu7_Gen_s();
extern __attribute__((noreturn)) void reboot();

void resetRudolph(void) {
#define ARM7_PROG (0x03810000 - 0xA00)
    void (*_menu7_Gen)();
    REG_SOUNDCNT = 0;
    u32	*adr;
    u32	*buf;
    u32	i;

    //IPCZ->cmd=0;
    //*(vu32*)0x2fffc24=dstt_sdhc;
    //writePowerManagement(0, readPowerManagement(0) | myPM_LED_BLINK);
    REG_IME = 0;
    REG_IE = 0;
    REG_IF = REG_IF;

    REG_IPC_SYNC = 0;
    DMA0_CR = 0;
    DMA1_CR = 0;
    DMA2_CR = 0;
    DMA3_CR = 0;

    //copy final loader to private RAM
    adr = (u32*)ARM7_PROG;
    buf = (u32*)_menu7_Gen_s;
    for(i = 0; i < 0x200/4; i++) {
        *adr = *buf;
        adr++;
        buf++;
    }

    while((*(vu32*)0x02fFFDFC) != 0x02fFFDF8 && (*(vu32*)0x02fFFDFC) != 0x0cfFFDF8);	// Timing adjustment with ARM9
    _menu7_Gen = (void(*)())ARM7_PROG;
    _menu7_Gen();
    while(1);
}