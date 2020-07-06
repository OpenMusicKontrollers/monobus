## monobus

### OSC to Lawo MonoBus bridge

Control your Lawo MonoBus devices via an OSC server that talks to FTDI USB adapters.

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

* <https://git.open-music-kontrollers.ch/gfx/monobus>

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
	ninja test
	sudo ninja install

### Usage

#### Discover your type of FTDI device via e.g. dmesg when plugging it in

	[user@machine ~] dmesg
	[  132.718684] usb 1-1.4: New USB device found, idVendor=0403, idProduct=6001, bcdDevice= 6.00
	[  132.718689] usb 1-1.4: New USB device strings: Mfr=1, Product=2, SerialNumber=3
	[  132.718691] usb 1-1.4: Product: FT232R USB UART
	[  132.718693] usb 1-1.4: Manufacturer: FTDI
	[  132.718695] usb 1-1.4: SerialNumber: ABCXYZ

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

#### Run monobus client with a 112x16 pixel pbm image at offset (2, 3)

	monobusc \
		-P 11  \                      # priority level 11
		-X 2 \                        # put image data at x-offset 2
		-Y 3 \                        # put image data at x-offset 3
		-W 112 \                      # image data width
		-H 16 \                       # image data height
		-U osc.udp://localhost:7777 \ # OSC server URI
		-I bitmap.pbm                 # Bitmap in PBM format

#### Run monobus client to clear image at priority level 11

	monobusc \
		-P 11  \                      # priority level 11
		-C \                          # clear image data
		-U osc.udp://localhost:7777   # OSC server URI

#### Control monobusd with your favorite OSC client

##### **/monobus/PRIO ,iiiib BITMAP-DATA**

To set the bitmap, send your OSC messages to given OSC path with
**PRIO** 0-31 **i**nteger x-offset, **i**nteger y-offset, **i**nteger width,
**i**nteger height and **b**lob argument being your bitmap data in PBM payload
format.

	# set bitmap of size 112x16 at offset position (8,12) for priority level 11
	osc.udp://localhost:7777 /monobus/11 ,iiiib 8 12 112 16 {...}

##### **/monobus/PRIO ,

To clear the bitmap, send your empty OSC messages to given OSC path with
**PRIO** 0-31.

	# clear whole bitmap for priority level 11
	osc.udp://localhost:7777 /monobus/11 ,

### License

Copyright (c) 2019-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)

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
