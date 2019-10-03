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

#include <syslog.h>

#include <netpbm/pbm.h>

#include <osc.lv2/writer.h>
#include <osc.lv2/stream.h>

#include <varchunk.h>
#include <osc2ftdimonobus.h>

typedef struct _app_t app_t;

struct _app_t {
	const char *url;

	LV2_OSC_Stream stream;

	struct {
		varchunk_t *rx;
		varchunk_t *tx;
	} rb;

	uint8_t bitmap [LENGTH];
};

static void *
_write_req(void *data, size_t minimum, size_t *maximum)
{
	app_t *app = data;

	return varchunk_write_request_max(app->rb.rx, minimum, maximum);
}

static void
_write_adv(void *data, size_t written)
{
	app_t *app = data;

	varchunk_write_advance(app->rb.rx, written);
}

static const void *
_read_req(void *data, size_t *toread)
{
	app_t *app = data;

	return varchunk_read_request(app->rb.tx, toread);
}

static void
_read_adv(void *data)
{
	app_t *app = data;

	varchunk_read_advance(app->rb.tx);
}

static const LV2_OSC_Driver driver = {
	.write_req = _write_req,
	.write_adv = _write_adv,
	.read_req = _read_req,
	.read_adv = _read_adv
};

static void
_osc_deinit(app_t *app)
{
	lv2_osc_stream_deinit(&app->stream);

	if(app->rb.rx)
	{
		varchunk_free(app->rb.rx);
	}

	if(app->rb.tx)
	{
		varchunk_free(app->rb.tx);
	}
}

static int
_osc_init(app_t *app)
{
	app->rb.rx = varchunk_new(8192, true);
	if(!app->rb.rx)
	{
		goto failure;
	}

	app->rb.tx = varchunk_new(8192, true);
	if(!app->rb.tx)
	{
		goto failure;
	}

	if(lv2_osc_stream_init(&app->stream, app->url, &driver, app) != 0)
	{
		syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
		goto failure;
	}

	return 0;

failure:
	_osc_deinit(app);
	return -1;
}

int
main(int argc, char **argv)
{
	(void)lv2_osc_stream_pollin; //FIXME
	(void)lv2_osc_hooks; //FIXME

	static app_t app;
	int width = 0;
	int height = 0;
	int format = 0;
	FILE *fin = stdin;

	pbm_init(&argc, argv);

	if(argc > 1)
	{
		fin = fopen(argv[1], "rb");
	}

	if(!fin)
	{
		return -1;
	}

	pbm_readpbminit(fin, &width, &height, &format);

	if( (width != WIDTH) || (height != HEIGHT) )
	{
		return -1;
	}

	pbm_readpbmrow_packed(fin, app.bitmap, width*height, format);

	if(fin != stdin)
	{
		fclose(fin);
	}

	app.url = "osc.udp://localhost:7777";

	int logp = LOG_DEBUG;
	openlog(NULL, LOG_PERROR, LOG_DAEMON);
	setlogmask(LOG_UPTO(logp));

	if(_osc_init(&app) == -1)
	{
		return -1;
	}

	size_t sz = 0;
	uint8_t *buf = varchunk_write_request_max(app.rb.tx, 1024, &sz);
	if(buf)
	{
		LV2_OSC_Writer writer;
		size_t written;

		lv2_osc_writer_initialize(&writer, buf, sz);

		const int32_t len = LENGTH;
		if(lv2_osc_writer_message_vararg(&writer, "/monobus", "b", len, app.bitmap))
		{
			if(lv2_osc_writer_finalize(&writer, &written))
			{
				varchunk_write_advance(app.rb.tx, written);

				while( (lv2_osc_stream_run(&app.stream) & LV2_OSC_SEND) != LV2_OSC_SEND)
				{
					usleep(1000);
				}
			}
			else
			{
				syslog(LOG_ERR, "lv2_osc_writer_finalize");
			}
		}
		else
		{
			syslog(LOG_ERR, "lv2_osc_writer_message_vararg");
		}
	}
	else
	{
		syslog(LOG_ERR, "varchunk_write_request_max");
	}

	_osc_deinit(&app);

	return 0;
}
