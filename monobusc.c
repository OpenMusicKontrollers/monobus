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

#if defined(HAVE_NETPBM_SUBDIR)
#	include <netpbm/pbm.h>
#else
#	include <pbm.h>
#endif

#include <osc.lv2/writer.h>
#include <osc.lv2/stream.h>

#include <varchunk.h>
#include <monobus.h>

typedef struct _app_t app_t;

struct _app_t {
	uint8_t prio;
	const char *url;
	const char *path;

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

static void
_version(void)
{
	fprintf(stderr,
		"--------------------------------------------------------------------\n"
		"This is free software: you can redistribute it and/or modify\n"
		"it under the terms of the Artistic License 2.0 as published by\n"
		"The Perl Foundation.\n"
		"\n"
		"This source is distributed in the hope that it will be useful,\n"
		"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
		"Artistic License 2.0 for more details.\n"
		"\n"
		"You should have received a copy of the Artistic License 2.0\n"
		"along the source as a COPYING file. If not, obtain it from\n"
		"http://www.perlfoundation.org/artistic_license_2_0.\n\n");
}

static void
_usage(char **argv, app_t *app)
{
	fprintf(stderr,
		"--------------------------------------------------------------------\n"
		"USAGE\n"
		"   %s [OPTIONS] FILE.PBM|-\n"
		"\n"
		"OPTIONS\n"
		"   [-v]                     print version information\n"
		"   [-h]                     print usage information\n"
		"   [-d]                     enable verbose logging\n"
		"   [-P] PRIO                set priority of message(%"PRIu8")\n"
		"   [-U] URI                 OSC URI (%s)\n"
		"   [-I] FILE                Bitmap in PBM format (%s)\n"
		, argv[0], app->prio, app->url, app->path);
}

int
main(int argc, char **argv)
{
	(void)lv2_osc_stream_pollin; //FIXME
	(void)lv2_osc_hooks; //FIXME
	static app_t app;
	int logp = LOG_INFO;

	app.prio = 0;
	app.url = "osc.udp://localhost:7777";
	app.path = "-";

	int width = 0;
	int height = 0;
	int format = 0;
	FILE *fin = stdin;

	fprintf(stderr,
		"%s "MONOBUS_VERSION"\n"
		"Copyright (c) 2019 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
		"Released under Artistic License 2.0 by Open Music Kontrollers\n",
		argv[0]);

	int c;
	while( (c = getopt(argc, argv, "vhdU:I:") ) != -1)
	{
		switch(c)
		{
			case 'v':
			{
				_version();
			}	return 0;
			case 'h':
			{
				_usage(argv, &app);
			}	return 0;
			case 'd':
			{
				logp = LOG_DEBUG;
			}	break;
			case 'U':
			{
				app.url = optarg;
			} break;
			case 'I':
			{
				app.path = optarg;
			} break;

			case '?':
			{
				if( (optopt == 'U') || (optopt == 'I') )
				{
					fprintf(stderr, "Option `-%c' requires an argument.\n", optopt);
				}
				else if(isprint(optopt))
				{
					fprintf(stderr, "Unknown option `-%c'.\n", optopt);
				}
				else
				{
					fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
				}
			}	return -1;
			default:
			{
				// nothing
			}	return -1;
		}
	}

	openlog(NULL, LOG_PERROR, LOG_DAEMON);
	setlogmask(LOG_UPTO(logp));

	if(strcmp(app.path, "-"))
	{
		fin = fopen(app.path, "rb");
	}

	if(!fin)
	{
		syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
		return -1;
	}

	pbm_init(&argc, argv);
	pbm_readpbminit(fin, &width, &height, &format);

	if( (width != WIDTH) || (height != HEIGHT) )
	{
		syslog(LOG_ERR, "[%s] 'width or height invalid'", __func__);
		return -1;
	}

	pbm_readpbmrow_packed(fin, app.bitmap, width*height, format);

	if(fin != stdin)
	{
		fclose(fin);
	}

	if(_osc_init(&app) == -1)
	{
		return -1;
	}

	size_t sz = 0;
	uint8_t *buf = varchunk_write_request_max(app.rb.tx, 1024, &sz);
	if(!buf)
	{
		syslog(LOG_ERR, "varchunk_write_request_max");
		goto failure;
	}

	LV2_OSC_Writer writer;
	size_t written;

	lv2_osc_writer_initialize(&writer, buf, sz);

	const int32_t len = LENGTH;
	char path [32];
	snprintf(path, sizeof(path), "/monobus/%"PRIu8, app.prio);
	if(!lv2_osc_writer_message_vararg(&writer, path, "b", len, app.bitmap))
	{
		syslog(LOG_ERR, "lv2_osc_writer_message_vararg");
		goto failure;
	}

	if(!lv2_osc_writer_finalize(&writer, &written))
	{
		syslog(LOG_ERR, "lv2_osc_writer_finalize");
		goto failure;
	}

	varchunk_write_advance(app.rb.tx, written);

	while( (lv2_osc_stream_run(&app.stream) & LV2_OSC_SEND) != LV2_OSC_SEND)
	{
		usleep(1000);
	}

	_osc_deinit(&app);
	return 0;

failure:
		_osc_deinit(&app);
		return -1;
}
