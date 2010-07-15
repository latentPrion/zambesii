#ifndef ___KORIENTATION_ENTRY_H
	#define ___KORIENTATION_ENTRY_H

	#include <multiboot.h>
	#include <__kstdlib/__ktypes.h>

extern "C" __attribute__((	section(".__korientationText") ))
	void __korientationEntry(ubit32 mbMagic, multibootDataS *mbData);

#endif

