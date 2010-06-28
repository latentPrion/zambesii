#ifndef _PANIC_H
	#define _PANIC_H

	#include <lang/lang.h>
	#include <__kstdlib/__ktypes.h>

// Handle a standard kernel error.
void panic(error_t err);
// Handle a generic error with a specific message.
void panic(const LANG_TYPE msg);

#endif

