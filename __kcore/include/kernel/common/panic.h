#ifndef _PANIC_H
	#define _PANIC_H

	#include <__kstdlib/__ktypes.h>

// Handle a standard kernel error.
void panic(error_t err, utf8Char *str=NULL) __attribute__((noreturn));
// Handle a generic error with a specific message.
void panic(utf8Char *str, error_t err=ERROR_SUCCESS) __attribute__((noreturn));
// Panic without attempting to print any message at all.
void panic(void) __attribute__((noreturn));
#endif

