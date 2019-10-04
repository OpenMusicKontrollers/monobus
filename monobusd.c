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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <ncurses.h>
#include <locale.h>

#ifdef HAVE_LIBFTDI1
#	include <libftdi1/ftdi.h>
#else
#	include <ftdi.h>
#endif // HAVE_LIBFTDI1

#include <osc.lv2/stream.h>

#include <varchunk.h>
#include <monobus.h>

#define FTDI_VID   0x0403
#define FT232_PID  0x6001
#define NSECS      1000000000
#define JAN_1970   2208988800ULL

typedef struct _sched_t sched_t;
typedef struct _app_t app_t;

struct _sched_t {
	sched_t *next;
	struct timespec to;
	size_t len;
	uint8_t buf [];
};

struct _app_t {
	uint16_t vid;
	uint16_t pid;
	const char *sid;
	const char *des;
	uint32_t fps;
	const char *url;
	bool simulate;

	LV2_OSC_Stream stream;
	pthread_t thread;

	struct ftdi_context ftdi;

	sched_t *list;

	struct {
		varchunk_t *rx;
		varchunk_t *tx;
	} rb;

	state_t state;
};

static atomic_bool reconnect = ATOMIC_VAR_INIT(false);
static atomic_bool done = ATOMIC_VAR_INIT(false);

static void
_sig(int num __attribute__((unused)))
{
	atomic_store(&reconnect, false);
	atomic_store(&done, true);
}

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
_handle_osc_packet(app_t *app, uint64_t timetag, const uint8_t *buf, size_t len);

static void
_handle_osc_message(app_t *app __attribute__((unused)), LV2_OSC_Reader *reader,
	size_t len)
{
	state_t *state = &app->state;

	lv2_osc_reader_match(reader, len, tree_root, state);
}

static void
_handle_osc_bundle(app_t *app, LV2_OSC_Reader *reader, size_t len)
{
	OSC_READER_BUNDLE_FOREACH(reader, itm, len)
	{
		_handle_osc_packet(app, itm->timetag, itm->body, itm->size);
	}
}

static sched_t *
_sched_append(sched_t *list, sched_t *elmnt)
{
	if(!list)
	{
		elmnt->next = NULL;
		return elmnt;
	}

	sched_t *prev = NULL;
	for(sched_t *ptr = list; ptr; prev = ptr, ptr = ptr->next)
	{
		if(  (ptr->to.tv_sec > elmnt->to.tv_sec)
			&& (ptr->to.tv_nsec > elmnt->to.tv_nsec) )
		{
			break;
		}
	}

	if(!prev)
	{
		elmnt->next = list;
		return elmnt;
	}

	elmnt->next = prev->next;
	prev->next = elmnt;

	return list;
}

static void
_handle_osc_packet(app_t *app, uint64_t timetag, const uint8_t *buf, size_t len)
{
	LV2_OSC_Reader reader;
	lv2_osc_reader_initialize(&reader, buf, len);

	if(lv2_osc_reader_is_bundle(&reader))
	{
		_handle_osc_bundle(app, &reader, len);
	}
	else if(lv2_osc_reader_is_message(&reader))
	{
		if(timetag == LV2_OSC_IMMEDIATE)
		{
			_handle_osc_message(app, &reader, len);
		}
		else
		{
			sched_t *elmnt = malloc(sizeof(sched_t) + len);
			if(elmnt)
			{
				elmnt->next = NULL;
				elmnt->to.tv_sec = (timetag >> 32) - JAN_1970;
				elmnt->to.tv_nsec = (timetag && 32) * 0x1p-32 * 1e9;
				elmnt->len = len;
				memcpy(elmnt->buf, buf, len);

				app->list = _sched_append(app->list, elmnt);
			}
			else
			{
				syslog(LOG_ERR, "[%s] malloc failed", __func__);
			}
		}
	}
}

typedef enum _command_type_t {
	COMMAND_STATUS = 0x80,
	COMMAND_LED_SETUP  = 0xb0,
	COMMAND_LED_OUTSET = 0xc0,
	COMMAND_LED_OUTDAT = 0xd0,
	COMMAND_LED_OUTPUT = 0xe0
} command_type_t;

typedef struct _payload_led_setup_t payload_led_setup_t;
typedef struct _payload_led_outset_t payload_led_outset_t;
typedef struct _payload_led_outdat_t payload_led_outdat_t;

struct _payload_led_setup_t {
	uint8_t data [17];
} __attribute__((packed));

struct _payload_led_outset_t {
	uint8_t data [8];
} __attribute__((packed));

struct _payload_led_outdat_t {
	uint8_t unused;
	uint8_t len;
	uint8_t bitmap [LENGTH];
} __attribute__((packed));

static const payload_led_setup_t led_setup = {
	.data = {
		0x00,
		0xff,
		0x2f,
		0x10,
		0x20,
		0x40,
		0x60,
		0x90,
		0xc0,
		0xf0,
		0x03,
		0x13,
		0x33,
		0x53,
		0x83,
		0xb3,
		0xe3
	}
};

static const payload_led_outset_t led_outset = {
	.data = {
		0x01,
		0x00,
		0x00,
		0x00,
		0x00,
		0x00,
		LENGTH,
		0x07
	}
};

static payload_led_outdat_t led_outdat = {
	.unused = 0xff,
	.len = LENGTH
};

#define FRAMING 0x7e
#define ESCAPE 0x7d

static uint8_t
_crc8(uint8_t seed, const uint8_t *data, size_t len)
{
	uint8_t crc = ~seed;

	for(size_t i = 0; i < len; i++)
	{
		crc ^= data[i];
	}

	return ~crc;
}

static ssize_t
_message(uint8_t *dst, size_t dst_len, uint8_t command, uint8_t id,
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
		const uint8_t crc = _crc8(0xff, &dst[1], len);

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

static int
_ftdi_xmit(app_t *app, const uint8_t *buf, ssize_t sz)
{
	if(app->simulate)
	{
		return 0;
	}

	if(ftdi_write_data(&app->ftdi, buf, sz) != sz)
	{
		goto failure;
	}

	usleep(100000); // give slave 100ms time to reply (half-duplex)

	return 0;

failure:
	syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
	return 1;
}

static int
_ftdi_init(app_t *app)
{
	if(app->simulate)
	{
		return 0;
	}

	app->ftdi.module_detach_mode = AUTO_DETACH_SIO_MODULE;

	if(ftdi_init(&app->ftdi) != 0)
	{
		goto failure;
	}

	if(ftdi_set_interface(&app->ftdi, INTERFACE_ANY) != 0)
	{
		goto failure_deinit;
	}

	if(app->des || app->sid)
	{
		if(ftdi_usb_open_desc(&app->ftdi, app->vid, app->pid,
			app->des, app->sid) != 0)
		{
			goto failure_deinit;
		}
	}
	else
	{
		if(ftdi_usb_open(&app->ftdi, app->vid, app->pid) != 0)
		{
			goto failure_deinit;
		}
	}

	if(ftdi_usb_reset(&app->ftdi) != 0)
	{
		goto failure_close;
	}

	if(ftdi_set_baudrate(&app->ftdi, 19200) != 0)
	{
		goto failure_close;
	}

	if(ftdi_set_line_property(&app->ftdi, BITS_8, STOP_BIT_1, NONE) != 0)
	{
		goto failure_close;
	}

	if(ftdi_usb_purge_buffers(&app->ftdi) != 0)
	{
		goto failure_close;
	}

	if(ftdi_setflowctrl(&app->ftdi, SIO_DISABLE_FLOW_CTRL) != 0)
	{
		goto failure_close;
	}

	if(ftdi_setrts(&app->ftdi, 0) != 0)
	{
		goto failure_close;
	}

	return 0;

failure_close:
	ftdi_usb_close(&app->ftdi);

failure_deinit:
	ftdi_deinit(&app->ftdi);

failure:
	syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
	return -1;
}

static void
_ftdi_deinit(app_t *app)
{
	if(app->simulate)
	{
		return;
	}

	if(ftdi_usb_close(&app->ftdi) != 0)
	{
		syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
	}

	ftdi_deinit(&app->ftdi);
}

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
_dump_bitmap(app_t *app)
{
	state_t *state = &app->state;

	if(!app->simulate)
	{
		return;
	}

	clear();

#define MUL 2
	WINDOW *win = newwin(WIDTH + 2, HEIGHT*MUL + 2 + 1, 0, 0);

	if(has_colors())
	{
		wattron(win, COLOR_PAIR(1));
	}

	box(win, 0, 0);

	if(has_colors())
	{
		wattroff(win, COLOR_PAIR(1));
	}

	for(unsigned y = 0; y < HEIGHT; y++)
	{
		const unsigned row_offset = y*STRIDE;

		for(unsigned x = 0; x < WIDTH; x++)
		{
			const unsigned col_offset = STRIDE - (x / 8) - 1;
			const uint8_t byte = state->bitmap[row_offset + col_offset];
			const uint8_t mask = 1 << (x % 8);

			if(byte & mask)
			{
				if(has_colors())
				{
					wattron(win, COLOR_PAIR(2) | A_BOLD);
				}

				mvwprintw(win, x + 1, y*MUL + 1, " ●");

				if(has_colors())
				{
					wattroff(win, COLOR_PAIR(2) | A_BOLD);
				}
			}
			else if(has_colors())
			{
				wattron(win, COLOR_PAIR(3) | A_BOLD);

				mvwprintw(win, x + 1, y*MUL + 1, " ●");

				wattroff(win, COLOR_PAIR(3) | A_BOLD);
			}
		}
	}

	wrefresh(win);
	delwin(win);
}

static void *
_beat(void *data)
{
	uint8_t dst [512];
	ssize_t sz;

	app_t *app = data;
	state_t *state = &app->state;
	const uint8_t id = 0x2;
	const uint64_t step_ns = NSECS / app->fps;

	struct timespec to;
	clock_gettime(CLOCK_REALTIME, &to);

	// write MONOBUS data
	sz = _message(dst, sizeof(dst), COMMAND_STATUS, id, NULL, 0);
	if(_ftdi_xmit(app, dst, sz) != 0)
	{
		atomic_store(&done, true); // end xmit loop
	}

	// write MONOBUS data
	sz = _message(dst, sizeof(dst), COMMAND_LED_SETUP, id,
		(const uint8_t *)&led_setup, sizeof(led_setup));
	if(_ftdi_xmit(app, dst, sz) != 0)
	{
		atomic_store(&done, true); // end xmit loop
	}

	while(!atomic_load(&done))
	{
		// sleep until next beat timestamp
		if(clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &to, NULL) == -1)
		{
			continue;
		}

		// read OSC messages from ringbuffer
		const uint8_t *buf;
		size_t len;
		while( (buf = varchunk_read_request(app->rb.rx, &len)) )
		{
			_handle_osc_packet(app, LV2_OSC_IMMEDIATE, buf, len);

			varchunk_read_advance(app->rb.rx);
		}

		// read OSC messages from list
		for(sched_t *elmnt = app->list; elmnt; elmnt = app->list)
		{
			double diff = to.tv_sec - elmnt->to.tv_sec;
			diff += (to.tv_nsec - elmnt->to.tv_nsec) * 1e-9;

			if(diff < 0.0)
			{
				break;
			}

			_handle_osc_packet(app, LV2_OSC_IMMEDIATE, elmnt->buf, elmnt->len);

			app->list = elmnt->next;
			free(elmnt);
		}

		// write MONOBUS data
		sz = _message(dst, sizeof(dst), COMMAND_LED_OUTSET, id,
			(const uint8_t *)&led_outset, sizeof(led_outset));
		if(_ftdi_xmit(app, dst, sz) != 0)
		{
			atomic_store(&done, true); // end xmit loop
		}

		_dump_bitmap(app);

		// copy bitmap to outdat
		memcpy(led_outdat.bitmap, state->bitmap, LENGTH);

		// write MONOBUS data
		sz = _message(dst, sizeof(dst), COMMAND_LED_OUTDAT, id,
			(const uint8_t *)&led_outdat, sizeof(led_outdat));
		if(_ftdi_xmit(app, dst, sz) != 0)
		{
			atomic_store(&done, true); // end xmit loop
		}

		// write MONOBUS data
		sz = _message(dst, sizeof(dst), COMMAND_LED_OUTPUT, id, NULL, 0);
		if(_ftdi_xmit(app, dst, sz) != 0)
		{
			atomic_store(&done, true); // end xmit loop
		}

		// calculate next beat timestamp
		to.tv_nsec += step_ns;
		while(to.tv_nsec >= NSECS)
		{
			to.tv_sec += 1;
			to.tv_nsec -= NSECS;
		}
	}

	{

		// write MONOBUS data
		sz = _message(dst, sizeof(dst), COMMAND_LED_OUTSET, id,
			(const uint8_t *)&led_outset, sizeof(led_outset));
		if(_ftdi_xmit(app, dst, sz) != 0)
		{
			atomic_store(&done, true); // end xmit loop
		}

		// clear bitmap in outdat
		memset(led_outdat.bitmap, 0x0, LENGTH);

		// write MONOBUS data
		sz = _message(dst, sizeof(dst), COMMAND_LED_OUTDAT, id,
			(const uint8_t *)&led_outdat, sizeof(led_outdat));
		if(_ftdi_xmit(app, dst, sz) != 0)
		{
			atomic_store(&done, true); // end xmit loop
		}

		// write MONOBUS data
		sz = _message(dst, sizeof(dst), COMMAND_LED_OUTPUT, id, NULL, 0);
		if(_ftdi_xmit(app, dst, sz) != 0)
		{
			atomic_store(&done, true); // end xmit loop
		}
	}

	return NULL;
}

static int
_thread_init(app_t *app)
{
	if(pthread_create(&app->thread, NULL, _beat, app) != 0)
	{
		syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
		return -1;
	}

	return 0;
}

static void
_thread_deinit(app_t *app)
{
	pthread_join(app->thread, NULL);
}

static void
_sched_deinit(app_t *app)
{
	for(sched_t *elmnt = app->list; elmnt; elmnt = app->list)
	{
		app->list = elmnt->next;
		free(elmnt);
	}
}

static int
_loop(app_t *app)
{
	if(_osc_init(app) == -1)
	{
		return -1;
	}

	if(_ftdi_init(app) == -1)
	{
		_osc_deinit(app);
		return -1;
	}

	if(_thread_init(app) == -1)
	{
		_ftdi_deinit(app);
		_osc_deinit(app);
		return -1;
	}

	atomic_store(&done, false);

	while(!atomic_load(&done))
	{
		const LV2_OSC_Enum status = lv2_osc_stream_pollin(&app->stream, 1000);

		if(status & LV2_OSC_ERR)
		{
			syslog(LOG_ERR, "[%s] '%s'", __func__, strerror(errno));
		}
	}

	_thread_deinit(app);
	_sched_deinit(app);
	_ftdi_deinit(app);
	_osc_deinit(app);

	return 0;
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
		"   %s [OPTIONS]\n"
		"\n"
		"OPTIONS\n"
		"   [-v]                     print version information\n"
		"   [-h]                     print usage information\n"
		"   [-d]                     enable verbose logging\n"
		"   [-A]                     enable auto-reconnect (disabled)\n"
		"   [-T]                     run test simulation (disabled)\n"
		"   [-V] VID                 USB vendor ID (0x%04"PRIx16")\n"
		"   [-P] PID                 USB product ID (0x%04"PRIx16")\n"
		"   [-D] DESCRIPTION         USB product name (%s)\n"
		"   [-S] SERIAL              USB serial ID (%s)\n"
		"   [-F] FPS                 Frame rate (%"PRIu32")\n"
		"   [-U] URI                 OSC URI (%s)\n\n"
		, argv[0], app->vid, app->pid, app->des, app->sid, app->fps, app->url);
}

int
main(int argc, char **argv)
{
	(void)lv2_osc_hooks; //FIXME
	static app_t app;
	int logp = LOG_INFO;

	app.vid = FTDI_VID;
	app.pid = FT232_PID;
	app.des = NULL;
	app.sid = NULL;
	app.fps = 2;
	app.url = "osc.udp://:7777";

	fprintf(stderr,
		"%s "MONOBUS_VERSION"\n"
		"Copyright (c) 2019 Hanspeter Portner (dev@open-music-kontrollers.ch)\n"
		"Released under Artistic License 2.0 by Open Music Kontrollers\n",
		argv[0]);

	int c;
	while( (c = getopt(argc, argv, "vhdATV:P:D:S:F:U:") ) != -1)
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
			case 'A':
			{
				atomic_store(&reconnect, true);
			}	break;
			case 'T':
			{
				app.simulate = true;
			}	break;

			case 'V':
			{
				app.vid = strtol(optarg, NULL, 16);
			} break;
			case 'P':
			{
				app.pid = strtol(optarg, NULL, 16);
			} break;
			case 'D':
			{
				app.des = optarg;
			} break;
			case 'S':
			{
				app.sid = optarg;
			} break;
			case 'F':
			{
				app.fps = strtol(optarg, NULL, 10);;
			} break;
			case 'U':
			{
				app.url = optarg;
			} break;

			case '?':
			{
				if(  (optopt == 'V') || (optopt == 'P') || (optopt == 'D')
					|| (optopt == 'S') || (optopt == 'F') || (optopt == 'U') )
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

	signal(SIGINT, _sig);
	signal(SIGTERM, _sig);
	signal(SIGQUIT, _sig);
	signal(SIGKILL, _sig);

	openlog(NULL, LOG_PERROR, LOG_DAEMON);
	setlogmask(LOG_UPTO(logp));

	if(app.simulate)
	{
		setlocale(LC_ALL, "");
		initscr();

		noecho(); // don't echo eny keypresses
		curs_set(false); // hide cursor

		if(has_colors())
		{
			start_color();
			init_pair(1, COLOR_WHITE, COLOR_BLACK);
			init_pair(2, COLOR_YELLOW, COLOR_BLACK);
			init_pair(3, COLOR_BLACK, COLOR_BLACK);
		}
	}

	int ret = _loop(&app);

	while(atomic_load(&reconnect))
	{
		syslog(LOG_NOTICE, "[%s] preparing to reconnect", __func__);
		sleep(1);

		ret = _loop(&app);
	}

	if(app.simulate)
	{
		endwin();
	}

	return ret;
}
