#ifndef _PANIC_H
	#define _PANIC_H

	#include <lang/lang.h>
	#include <__kstdlib/__ktypes.h>

// Handle a standard kernel error.
void panic(error_t err, utf8Char *str=__KNULL);
// Handle a generic error with a specific message.

#endif

