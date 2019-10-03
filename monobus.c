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

static void
_root(LV2_OSC_Reader *reader, LV2_OSC_Arg *arg,
	const LV2_OSC_Tree *tree __attribute__((unused)), void *data)
{
	(void)lv2_osc_hooks; //FIXME
	state_t *state = data;
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

const LV2_OSC_Tree tree_root [1+1] = {
	{ .name = "monobus", .branch = _root },
	{ .name = NULL }
};
