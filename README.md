## monobus

### OSC to Lawo MonoBus bridge

Control your Lawo MonoBus devices via an OSC server that talks to FTDI USB adapters..

#### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/monobus/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/monobus/commits/master)

### Binaries

For GNU/Linux (64-bit, 32-bit, armv7, aarch64).

<!--
#### Stable release

* [monobus-0.2.0.zip](https://dl.open-music-kontrollers.ch/monobus/stable/monobus-0.2.0.zip) ([sig](https://dl.open-music-kontrollers.ch/monobus/stable/monobus-0.2.0.zip.sig))
-->

#### Unstable (nightly) release

* [monobus-latest-unstable.zip](https://dl.open-music-kontrollers.ch/monobus/unstable/monobus-latest-unstable.zip) ([sig](https://dl.open-music-kontrollers.ch/monobus/unstable/monobus-latest-unstable.zip.sig))

### Sources

<!--
#### Stable release

* [monobus-0.2.0.tar.xz](https://git.open-music-kontrollers.ch/gfx/monobus/snapshot/monobus-0.2.0.tar.xz)
-->

#### Git repository

* <https://git.open-music-kontrollers.ch/mix/monobus>

### Bugs and feature requests

* [Gitlab](https://gitlab.com/OpenMusicKontrollers/monobus)
* [Github](https://github.com/OpenMusicKontrollers/monobus)

### Dependencies

* [LV2](http://lv2plug.in/) (LV2 Plugin Standard)
* [libftdi](https://www.intra2net.com/en/developer/libftdi/index.php) (Library to talk to FTDI chips)
* [netpbm](http://netpbm.sourceforge.net/) (Toolkit for manipulation of graphic images)
* [ncurses](https://www.gnu.org/software/ncurses/) (Free software emulation of curses)

### Build / install

	git clone https://git.open-music-kontrollers.ch/gfx/monobus
	cd monobus
	meson build
	cd build
	ninja
	sudo ninja install

### Usage

#### Run monobus daemon with the information gathered above

	monobusd \
		-V 0x0403 \                   # USB idVendor
		-P 0x6001 \                   # USB idProduct
		-D 'FT232R USB UART' \        # USB product description
		-S ABCXYZ \                   # USB product serial number
		-F 2 \                        # update rate in frames per second
		-U osc.udp://:7777            # OSC server URI

#### Run monobus daemon in testing, aka simulation mode with ncurses output

	monobusd \
		-T \                          # enable testing mode
		-F 2 \                        # update rate in frames per second
		-U osc.udp://:7777            # OSC server URI

#### Run monobus client with a 16x112 pixel pbm image

	monobusc \
		-U osc.udp://localhost:7777   # OSC server URI
		bitmap.pbm

### License

Copyright (c) 2019 Hanspeter Portner (dev@open-music-kontrollers.ch)

This is free software: you can redistribute it and/or modify
it under the terms of the Artistic License 2.0 as published by
The Perl Foundation.

This source is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
Artistic License 2.0 for more details.

You should have received a copy of the Artistic License 2.0
along the source as a COPYING file. If not, obtain it from
<http://www.perlfoundation.org/artistic_license_2_0>.
