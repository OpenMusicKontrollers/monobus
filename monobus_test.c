/*
 * Copyright (c) 2019-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <assert.h>
#include <string.h>

#include <monobus.h>

static void
_test_parse()
{
	state_t state;
	LV2_OSC_Reader reader;

	// test clearing bitmap
	{
		const uint8_t msg [] = {
			'/', 'm', 'o', 'n',
			'o', 'b', 'u', 's',
			'/', '0', 0x0, 0x0,
			',', 0x0, 0x0, 0x0
		};

		memset(&reader, 0xff, sizeof(reader));
		lv2_osc_reader_initialize(&reader, msg, sizeof(msg));

		memset(&state, 0x0, sizeof(state));
		lv2_osc_reader_match(&reader, sizeof(msg), tree_root, &state);

		for(unsigned y = 0; y < HEIGHT_NET; y++)
		{
			for(unsigned x = 0; x < WIDTH_NET; x++)
			{
				assert(state.pixels[y][x].mask == 0x0);
				assert(state.pixels[y][x].bits== 0x0);
			}
		}
	}

	// test setting bitmap
	{
		const uint8_t msg [] = {
			'/', 'm', 'o', 'n',
			'o', 'b', 'u', 's',
			'/', '1', 0x0, 0x0,
			',', 'b', 0x0, 0x0,

			0x00, 0x00, 0x00, LENGTH_SER, // blob size

			0x00, 0x01, 0x02, 0x03, // blob data (aka bitmap)
			0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b,
			0x0c, 0x0d, 0x0e, 0x0f,

			0x10, 0x11, 0x12, 0x13,
			0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b,
			0x1c, 0x1d, 0x1e, 0x1f,

			0x20, 0x21, 0x22, 0x23,
			0x24, 0x25, 0x26, 0x27,
			0x28, 0x29, 0x2a, 0x2b,
			0x2c, 0x2d, 0x2e, 0x2f,

			0x30, 0x31, 0x32, 0x33,
			0x34, 0x35, 0x36, 0x37,
			0x38, 0x39, 0x3a, 0x3b,
			0x3c, 0x3d, 0x3e, 0x3f,

			0x40, 0x41, 0x42, 0x43,
			0x44, 0x45, 0x46, 0x47,
			0x48, 0x49, 0x4a, 0x4b,
			0x4c, 0x4d, 0x4e, 0x4f,

			0x50, 0x51, 0x52, 0x53,
			0x54, 0x55, 0x56, 0x57,
			0x58, 0x59, 0x5a, 0x5b,
			0x5c, 0x5d, 0x5e, 0x5f,

			0x60, 0x61, 0x62, 0x63,
			0x64, 0x65, 0x66, 0x67,
			0x68, 0x69, 0x6a, 0x6b,
			0x6c, 0x6d, 0x6e, 0x6f,

			0x70, 0x71, 0x72, 0x73,
			0x74, 0x75, 0x76, 0x77,
			0x78, 0x79, 0x7a, 0x7b,
			0x7c, 0x7d, 0x7e, 0x7f,

			0x80, 0x81, 0x82, 0x83,
			0x84, 0x85, 0x86, 0x87,
			0x88, 0x89, 0x8a, 0x8b,
			0x8c, 0x8d, 0x8e, 0x8f,

			0x90, 0x91, 0x92, 0x93,
			0x94, 0x95, 0x96, 0x97,
			0x98, 0x99, 0x9a, 0x9b,
			0x9c, 0x9d, 0x9e, 0x9f,

			0xa0, 0xa1, 0xa2, 0xa3,
			0xa4, 0xa5, 0xa6, 0xa7,
			0xa8, 0xa9, 0xaa, 0xab,
			0xac, 0xad, 0xae, 0xaf,

			0xb0, 0xb1, 0xb2, 0xb3,
			0xb4, 0xb5, 0xb6, 0xb7,
			0xb8, 0xb9, 0xba, 0xbb,
			0xbc, 0xbd, 0xbe, 0xbf,

			0xc0, 0xc1, 0xc2, 0xc3,
			0xc4, 0xc5, 0xc6, 0xc7,
			0xc8, 0xc9, 0xca, 0xcb,
			0xcc, 0xcd, 0xce, 0xcf,

			0xd0, 0xd1, 0xd2, 0xd3,
			0xd4, 0xd5, 0xd6, 0xd7,
			0xd8, 0xd9, 0xda, 0xdb,
			0xdc, 0xdd, 0xde, 0xdf
		};

		memset(&reader, 0x00, sizeof(reader));
		lv2_osc_reader_initialize(&reader, msg, sizeof(msg));

		memset(&state, 0x0, sizeof(state));
		lv2_osc_reader_match(&reader, sizeof(msg), tree_root, &state);

		for(unsigned y = 0; y < HEIGHT_NET; y++)
		{
			for(unsigned x = 0; x < WIDTH_NET; x++)
			{
				assert(state.pixels[y][x].mask == 0x2);

				assert( (state.pixels[y][x].bits == 0x2)
					|| (state.pixels[y][x].bits == 0x0) ); //FIXME
			}
		}
	}
}

static void
_test_crc8()
{
	const uint8_t seed = 0xff;

	// test single-byte crc8
	{
		const uint8_t msg [] = {
			0x1
		};

		const uint8_t crc8 = ~(~seed ^ msg[0]);
		assert(crc8 == monobus_crc8(seed, msg, sizeof(msg)));
	}

	// test two-byte crc8
	{
		const uint8_t msg [] = {
			0x1,
			0x2
		};

		const uint8_t crc8 = ~(~seed ^ msg[0] ^ msg[1]);
		assert(crc8 == monobus_crc8(seed, msg, sizeof(msg)));
	}

	// test three-byte crc8
	{
		const uint8_t msg [] = {
			0x1,
			0x2,
			0x3
		};

		const uint8_t crc8 = ~(~seed ^ msg[0] ^ msg[1] ^ msg[2]);
		assert(crc8 == monobus_crc8(seed, msg, sizeof(msg)));
	}

	// test signgle-call vs iterative call of four-byte crc8
	{
		const uint8_t msg [] = {
			0x1,
			0x2,
			0x3,
			0x4
		};

		const uint8_t crc8_0 = ~(~seed ^ msg[0] ^ msg[1] ^ msg[2] ^ msg[3]);
		const uint8_t crc8_1 = monobus_crc8(seed, msg, sizeof(msg));

		uint8_t crc8_2 = seed;
		for(unsigned i = 0; i < sizeof(msg); i++)
		{
			crc8_2 = monobus_crc8(crc8_2, &msg[i], 1);
		}

		assert(crc8_0 == crc8_1);
		assert(crc8_1 == crc8_2);
	}
}

static void
_test_stride()
{
	assert(monobus_stride_for_width(0) == 0);
	assert(monobus_stride_for_width(1) == 1);
	assert(monobus_stride_for_width(2) == 1);
	assert(monobus_stride_for_width(3) == 1);
	assert(monobus_stride_for_width(4) == 1);
	assert(monobus_stride_for_width(5) == 1);
	assert(monobus_stride_for_width(6) == 1);
	assert(monobus_stride_for_width(7) == 1);
	assert(monobus_stride_for_width(8) == 1);

	assert(monobus_stride_for_width(9) == 2);
	assert(monobus_stride_for_width(10) == 2);
	assert(monobus_stride_for_width(11) == 2);
	assert(monobus_stride_for_width(12) == 2);
	assert(monobus_stride_for_width(13) == 2);
	assert(monobus_stride_for_width(14) == 2);
	assert(monobus_stride_for_width(15) == 2
			);
	assert(monobus_stride_for_width(16) == 2);
	assert(monobus_stride_for_width(17) == 3);
}

int
main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
	(void)lv2_osc_hooks; //FIXME
	_test_parse();
	_test_crc8();
	_test_stride();

	return 0;
}
