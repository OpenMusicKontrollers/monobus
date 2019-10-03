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

#ifndef _OSC2FTDIMONOBUS_H
#define _OSC2FTDIMONOBUS_H

#include <stdbool.h>
#include <stdint.h>
#include <syslog.h>

#include <osc.lv2/reader.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WIDTH 16
#define HEIGHT 112
#define STRIDE (WIDTH / 8)
#define LENGTH (STRIDE * HEIGHT)

typedef struct _state_t state_t;

struct _state_t {
	uint8_t bitmap [LENGTH];
};

extern const LV2_OSC_Tree tree_root [];

#ifdef __cplusplus
}
#endif

#endif //_OSC2FTDIMONOBUS_H
