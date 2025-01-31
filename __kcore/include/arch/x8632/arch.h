#ifndef _ARCH_H
	#define _ARCH_H

#define __x86_32__
#define __32_BIT__
#define __ARCH_LITTLE_ENDIAN__
#define __BITS_PER_BYTE__	8

#define __VADDR_NBITS__		32

#ifdef CONFIG_ARCH_x86_32_PAE
	#define __PADDR_NBITS__		64
#else
	#define __PADDR_NBITS__		32
#endif

#define ARCH_SHORT_STRING	"x86-32"
#define ARCH_LONG_STRING	"Intel IA-32 architecture compatible"
#define ARCH_PATH		x8632

#endif

