#ifndef _LANG_H
	#define _LANG_H

	#include <__kstdlib/__ktypes.h>

/**	EXPLANATION:
 * These are all expected to all be printf() compatible strings in UTF-8. That
 * is, they should be able to be used with __kprintf() without any
 * problems. In fact, they are all very much standardized, and each index has
 * a specific name (given by the preprocessor token), and is a printf format
 * string.
 *
 * The kernel is expected to give uniform notice, warning, and error messages
 * no matter what the language. So the same printf() args are expected for
 * every set of strings.
 *
 * For example, if there is a string in numaTribStr[] which, in English
 * is "Notice: creating new NUMA node with ID %i at %p\n", then in Spanish,
 * the person translating the kernel is *required* to have the '%i' and '%p'
 * placeholders since for any build of the kernel, no matter what the language,
 * the kernel is going to be passing two arguments.
 *
 * It would be really stupid if we had to do #ifdef ENGLISH #elif SPANISH for
 * every message.
 **/

// Standard, universal indexes into the 'lang[]' array of strings.
#define LANG_LANG_NAME		0
#define LANG_LANG_CODE		1
extern utf8Char		*lang[];

extern utf8Char		*mmStr[];

#endif

