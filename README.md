## osc2ftdimonobus

### OSC to FTDI MONOBUS bridge

Control your LAWO MONOBUS devices via an OSC server that talks to FTDI USB adapters..

#### Build status

[![build status](https://gitlab.com/OpenMusicKontrollers/osc2ftdimonobus/badges/master/build.svg)](https://gitlab.com/OpenMusicKontrollers/osc2ftdimonobus/commits/master)

### Binaries

For GNU/Linux (64-bit, 32-bit, armv7, aarch64).

<!--
#### Stable release

* [osc2ftdimonobus-0.2.0.zip](https://dl.open-music-kontrollers.ch/osc2ftdimonobus/stable/osc2ftdimonobus-0.2.0.zip) ([sig](https://dl.open-music-kontrollers.ch/osc2ftdimonobus/stable/osc2ftdimonobus-0.2.0.zip.sig))
-->

#### Unstable (nightly) release

* [osc2ftdimonobus-latest-unstable.zip](https://dl.open-music-kontrollers.ch/osc2ftdimonobus/unstable/osc2ftdimonobus-latest-unstable.zip) ([sig](https://dl.open-music-kontrollers.ch/osc2ftdimonobus/unstable/osc2ftdimonobus-latest-unstable.zip.sig))

### Sources

<!--
#### Stable release

* [osc2ftdimonobus-0.2.0.tar.xz](https://git.open-music-kontrollers.ch/lad/osc2ftdimonobus/snapshot/osc2ftdimonobus-0.2.0.tar.xz)
-->

#### Git repository

* <https://git.open-music-kontrollers.ch/lad/osc2ftdimonobus>

### Bugs and feature requests

* [Gitlab](https://gitlab.com/OpenMusicKontrollers/osc2ftdimonobus)
* [Github](https://github.com/OpenMusicKontrollers/osc2ftdimonobus)

### Dependencies

* [LV2](http://lv2plug.in/) (LV2 Plugin Standard)
* [libftdi](https://www.intra2net.com/en/developer/libftdi/index.php) (Library to talk to FTDI chips)

### Build / install

	git clone https://git.open-music-kontrollers.ch/lad/osc2ftdimonobus
	cd osc2ftdimonobus
	meson build
	cd build
	ninja
	sudo ninja install

### Usage

#### Run osc2ftdimonobus with the information gathered above

	osc2ftdimonobus \
		-V 0x0403 \                   # USB idVendor
		-P 0x6001 \                   # USB idProduct
		-D 'FTDI-232' \               # USB product description
		-S ABCXYZ \                   # USB product serial number
		-F 2 \                        # update rate in frames per second
		-U osc.udp://:6666            # OSC server URI

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
