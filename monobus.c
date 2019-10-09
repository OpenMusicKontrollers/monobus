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

#include <monobus.h>

#define FRAMING 0x7e
#define ESCAPE 0x7d

uint8_t
monobus_crc8(uint8_t seed, const uint8_t *data, size_t len)
{
	uint8_t crc = ~seed;

	for(size_t i = 0; i < len; i++)
	{
		crc ^= data[i];
	}

	return ~crc;
}

ssize_t
monobus_message(uint8_t *dst, size_t dst_len, uint8_t command, uint8_t id,
	const uint8_t *src, size_t src_len)
{
	(void)dst_len; //FIXME
	uint8_t *ptr = dst;

	*ptr++ = FRAMING;

	*ptr++ = command | id;

	for(size_t i = 0; i < src_len; i++)
	{
		const uint8_t byt = src[i];

		switch(byt)
		{
			case FRAMING:
			{
				*ptr++ = ESCAPE;
				*ptr++ = byt ^ 0x20;
			} break;
			case ESCAPE:
			{
				*ptr++ = ESCAPE;
				*ptr++ = byt ^ 0x20;
			} break;
			default:
			{
				*ptr++ = byt;
			} break;
		}
	}

	{
		const size_t len = ptr - &dst[1];
		const uint8_t crc = monobus_crc8(0xff, &dst[1], len);

		switch(crc)
		{
			case FRAMING:
			{
				*ptr++ = ESCAPE;
				*ptr++ = crc ^ 0x20;
			} break;
			case ESCAPE:
			{
				*ptr++ = ESCAPE;
				*ptr++ = crc ^ 0x20;
			} break;
			default:
			{
				*ptr++ = crc;
			} break;
		}
	}

	*ptr++ = FRAMING;

	const size_t len = ptr - dst;

	return len;
}

static const LV2_OSC_Tree tree_priority [32+1]; //FIXME

static void
_priority (LV2_OSC_Reader *reader __attribute__((unused)),
	LV2_OSC_Arg *arg __attribute__((unused)), const LV2_OSC_Tree *tree, void *data)
{
	(void)lv2_osc_hooks; //FIXME
	state_t *state = data;
	const uint8_t prio = tree - tree_priority;
	(void)prio; //FIXME

	bool set = false;

	for( ;
			!lv2_osc_reader_arg_is_end(reader, arg);
			arg = lv2_osc_reader_arg_next(reader, arg) )
	{
		switch(arg->type[0])
		{
			case LV2_OSC_BLOB:
			{
				if(arg->size == LENGTH)
				{
					memcpy(state->bitmap, arg->b, LENGTH);
					set = true;
				}
			} break;

			default:
			{
				// ignore other types
			} break;
		}
	}

	if(!set)
	{
		memset(state->bitmap, 0x0, LENGTH);
	}
}

static const LV2_OSC_Tree tree_priority [32+1] = {
	{ .name =  "0", .branch = _priority },
	{ .name =  "1", .branch = _priority },
	{ .name =  "2", .branch = _priority },
	{ .name =  "3", .branch = _priority },
	{ .name =  "4", .branch = _priority },
	{ .name =  "5", .branch = _priority },
	{ .name =  "6", .branch = _priority },
	{ .name =  "7", .branch = _priority },
	{ .name =  "8", .branch = _priority },
	{ .name =  "9", .branch = _priority },
	{ .name = "10", .branch = _priority },
	{ .name = "11", .branch = _priority },
	{ .name = "12", .branch = _priority },
	{ .name = "13", .branch = _priority },
	{ .name = "14", .branch = _priority },
	{ .name = "15", .branch = _priority },
	{ .name = "16", .branch = _priority },
	{ .name = "17", .branch = _priority },
	{ .name = "18", .branch = _priority },
	{ .name = "19", .branch = _priority },
	{ .name = "20", .branch = _priority },
	{ .name = "21", .branch = _priority },
	{ .name = "22", .branch = _priority },
	{ .name = "23", .branch = _priority },
	{ .name = "24", .branch = _priority },
	{ .name = "25", .branch = _priority },
	{ .name = "26", .branch = _priority },
	{ .name = "27", .branch = _priority },
	{ .name = "28", .branch = _priority },
	{ .name = "29", .branch = _priority },
	{ .name = "30", .branch = _priority },
	{ .name = "31", .branch = _priority },
	{ .name = NULL }
};

const LV2_OSC_Tree tree_root [1+1] = {
	{ .name = "monobus", .trees = tree_priority },
	{ .name = NULL }
};
