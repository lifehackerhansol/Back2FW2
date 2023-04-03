/*
	Back2FW2

	Copyright (C) 2010-2015 Taiju Yamada
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: MIT
*/

#include <nds.h>

#include "encryption.h"

void noop(u32 *ptr){}

static u32 lookup(u32 *magic, u32 v){
	u32 a = (v >> 24) & 0xFF;
	u32 b = (v >> 16) & 0xFF;
	u32 c = (v >> 8) & 0xFF;
	u32 d = (v >> 0) & 0xFF;

	a = magic[a + 18 + 0];
	b = magic[b + 18 + 256];
	c = magic[c + 18 + 512];
	d = magic[d + 18 + 768];

	return d + (c ^ (b + a));
}
static void enc_encrypt(u32 *magic, u32 *arg1, u32 *arg2) {
	u32 a,b,c;
	a = *arg1;
	b = *arg2;
	for(int i = 0; i < 16; i++) {
		c = magic[i] ^ a;
		a = b ^ lookup(magic, c);
		b = c;
	}
	*arg2 = a ^ magic[16];
	*arg1 = b ^ magic[17];
}

static void enc_decrypt(u32 *magic, u32 *arg1, u32 *arg2) {
	u32 a, b, c;
	a = *arg1;
	b = *arg2;
	for(int i = 17; i > 1; i--) {
		c = magic[i] ^ a;
		a = b ^ lookup(magic, c);
		b = c;
	}
	*arg1 = b ^ magic[0];
	*arg2 = a ^ magic[1];
}

static void enc_update_hashtable(u32* magic, u8* arg1, u32 modulo) {
	for(int j = 0; j < 18; j++) {
		u32 r3 = 0;
		for (int i = 0; i < 4; i++) {
			r3 <<= 8;
			r3 |= arg1[(j * 4 + i) % modulo];
		}
		magic[j] ^= r3;
	}

	u32 tmp1 = 0;
	u32 tmp2 = 0;
	for(int i = 0; i < 18; i += 2) {
		enc_encrypt(magic, &tmp1, &tmp2);
		magic[i + 0] = tmp1;
		magic[i + 1] = tmp2;
	}
	for(int i = 0; i < 0x400; i += 2) {
		enc_encrypt(magic, &tmp1, &tmp2);
		magic[i + 18 + 0] = tmp1;
		magic[i + 18 + 1] = tmp2;
	}
}

u32 arg2[3];

u32 card_hash[0x412];


static void enc_init2(u32 *magic, u32* a, u32 modulo) {
	enc_encrypt(magic, a + 2, a + 1);
	enc_encrypt(magic, a + 1, a + 0);
	enc_update_hashtable(magic, (u8*)a, modulo);
}

static void enc_init1(u32 cardheader_gamecode, int level, u32 modulo) {
	memcpy(card_hash, __encr_data, 4*(1024 + 18));
	arg2[0] = cardheader_gamecode;
	arg2[1] = cardheader_gamecode >> 1;
	arg2[2] = cardheader_gamecode << 1;
	enc_init2(card_hash, arg2, modulo);
	if(level >= 2) enc_init2(card_hash, arg2, modulo);
}

bool initKeycode(u32 idCode, int level) {
	enc_init1(idCode, level, 12);
	return true;
}


void crypt64BitDown(u32 *ptr) {
    enc_decrypt(card_hash, ptr+1, ptr);
}