/*
	Back2FW2

	Copyright (C) 2010-2015 Taiju Yamada
	Copyright (C) 2023 lifehackerhansol

	SPDX-License-Identifier: MIT
*/

#pragma once

#define FIFO_BUFFER_ADDR	FIFO_USER_01
#define FIFO_BUFFER_SIZE	FIFO_USER_02
#define FIFO_CONTROL		FIFO_USER_03
#define FIFO_RETURN			FIFO_USER_04

enum B2FW2_FIFO_CONTROL_VALUES {
	B2FW2_DUMP_FW,
	B2FW2_RETURN_NDS,
	B2FW2_RETURN_DSI
};
