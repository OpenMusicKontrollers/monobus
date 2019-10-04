/*
 * Copyright (c) 2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _MONOBUS_H
#define _MONOBUS_H

#include <stdbool.h>
#include <stdint.h>
#include <syslog.h>

#include <osc.lv2/reader.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIDTH  16
#define HEIGHT 112
#define STRIDE (WIDTH / 8)
#define LENGTH (STRIDE * HEIGHT)

typedef enum _command_type_t {
	COMMAND_STATUS     = 0x80,
	COMMAND_LED_SETUP  = 0xb0,
	COMMAND_LED_OUTSET = 0xc0,
	COMMAND_LED_OUTDAT = 0xd0,
	COMMAND_LED_OUTPUT = 0xe0
} command_type_t;

typedef struct _payload_led_setup_t payload_led_setup_t;
typedef struct _payload_led_outset_t payload_led_outset_t;
typedef struct _payload_led_outdat_t payload_led_outdat_t;
typedef struct _state_t state_t;

struct _payload_led_setup_t {
	uint8_t unknown_00;      // FIXME what is this byte for ?
	uint8_t unknown_01;      // FIXME what is this byte for ?
	uint8_t unknown_02;      // FIXME what is this byte for ?
	uint8_t unknown_03;      // FIXME what is this byte for ?
	uint8_t unknown_04;      // FIXME what is this byte for ?
	uint8_t unknown_05;      // FIXME what is this byte for ?
	uint8_t unknown_06;      // FIXME what is this byte for ?
	uint8_t unknown_07;      // FIXME what is this byte for ?
	uint8_t unknown_08;      // FIXME what is this byte for ?
	uint8_t unknown_09;      // FIXME what is this byte for ?
	uint8_t unknown_0a;      // FIXME what is this byte for ?
	uint8_t unknown_0b;      // FIXME what is this byte for ?
	uint8_t unknown_0c;      // FIXME what is this byte for ?
	uint8_t unknown_0d;      // FIXME what is this byte for ?
	uint8_t unknown_0e;      // FIXME what is this byte for ?
	uint8_t unknown_0f;      // FIXME what is this byte for ?
	uint8_t unknown_10;      // FIXME what is this byte for ?
} __attribute__((packed));

struct _payload_led_outset_t {
	uint8_t unknown_00;      // FIXME what is this byte for ?
	uint8_t unknown_01;      // FIXME what is this byte for ?
	uint8_t unknown_02;      // FIXME what is this byte for ?
	uint8_t unknown_03;      // FIXME what is this byte for ?
	uint8_t unknown_04;      // FIXME what is this byte for ?
	uint8_t unknown_05;      // FIXME what is this byte for ?
	uint8_t len;             // byte-length of bitmap
	uint8_t unknown_07;      // FIXME what is this byte for ?
} __attribute__((packed));

struct _payload_led_outdat_t {
	uint8_t unknown_00;      // FIXME what is this byte for ?
	uint8_t len;             // byte-length of bitmap
	uint8_t bitmap [LENGTH]; // bitmap in PBM format
} __attribute__((packed));

struct _state_t {
	uint8_t bitmap [LENGTH]; // bitmap in PBM format
};

extern const LV2_OSC_Tree tree_root [];

uint8_t
monobus_crc8(uint8_t seed, const uint8_t *data, size_t len);

ssize_t
monobus_message(uint8_t *dst, size_t dst_len, uint8_t command, uint8_t id,
	const uint8_t *src, size_t src_len);

#ifdef __cplusplus
}
#endif

#endif //_MONOBUS_H
