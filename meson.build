project('monobus', 'c', default_options : [
	'buildtype=release',
	'warning_level=3',
	'werror=false',
	'b_lto=false',
	'c_std=c11'])

version = run_command('cat', 'VERSION').stdout().strip()

add_project_arguments('-DMONOBUS_VERSION="'+version+'"', language : 'c')
add_project_arguments('-D_GNU_SOURCE', language : 'c')

osc_inc = include_directories('osc.lv2')
varchunk_inc = include_directories('varchunk')
incs = [osc_inc, varchunk_inc]

static_link = meson.is_cross_build()

cc = meson.get_compiler('c')

netpbm_dep = cc.find_library('netpbm', static : static_link)
ncursesw_dep = dependency('ncursesw')
thread_dep = dependency('threads')
lv2_dep = dependency('lv2', version : '>=1.14.0')
ftdi_dep = dependency('libftdi1', version : '>=1.3', static : static_link,
	required : false)
if not ftdi_dep.found()
	ftdi_dep = dependency('libftdi', version : '>=0.20', static : static_link)
else
	add_project_arguments('-DHAVE_LIBFTDI1', language : 'c')
endif

if cc.has_header('netpbm/pbm.h')
	add_project_arguments('-DHAVE_NETPBM_SUBDIR', language : 'c')
endif

executable('monobusd',
	[ 'monobusd.c', 'monobus.c' ],
	include_directories : incs,
	dependencies : [thread_dep, lv2_dep, ftdi_dep, ncursesw_dep],
	install : true)

executable('monobusc',
	[ 'monobusc.c' ],
	include_directories : incs,
	dependencies : [lv2_dep, netpbm_dep],
	install : true)

monobusd_man = configure_file(
	input : 'monobusd.1',
	output : 'monobusd.1',
	copy : true)

monobusc_man = configure_file(
	input : 'monobusc.1',
	output : 'monobusc.1',
	copy : true)

install_man(monobusd_man)
install_man(monobusc_man)

monobus_test = executable('monobus_test',
	[ 'monobus_test.c', 'monobus.c' ],
	include_directories : incs,
	dependencies : [lv2_dep],
	install : false)

test('Test', monobus_test)
