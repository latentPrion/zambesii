#!/bin/bash

#Set your cross toolchain here if you want. Also, change the chipset and
#firmware if building for a different chipset and firmware.

export \
	ZCPP='i586-elf-gcc -E' \
	ZCC='i586-elf-gcc' \
	ZCXX='i586-elf-g++' \
	ZAS='i586-elf-as' \
	ZLD='i586-elf-ld' \
	ZAR='i586-elf-ar' \
	\
	ZCPPFLAGS='-I/void/cygwin/osdev/zbz/include -I/void/cygwin/osdev/zbz/core/include' \
	ZCCFLAGS='' \
	ZCXXFLAGS='-Wall -Wextra -pedantic -Wno-long-long -fno-exceptions -fno-rtti' \
	ZASFLAGS=''\
	ZLDFLAGS='-nostdlib -nostartfiles -nodefaultlibs' \
	ZARFLAGS=''

export \
	LCPP='gcc -E' \
	LCC='gcc' \
	LCXX='g++' \
	LAS='as' \
	LLD='ld' \
	LAR='ar' \
	\
	LCPPFLAGS='' \
	LCCFLAGS='' \
	LCXXFLAGS='-Wall -Wextra -pedantic' \
	LASFLAGS=''\
	LLDFLAGS='-nostdlib -nostartfiles -nodefaultlibs' \
	LARFLAGS=''

export \
	ZARCH='x8632' \
	ZCHIPSET='ibmPc' \
	ZFIRMWARE='ibmPcBios' \
	ZLANG='en-uk'

