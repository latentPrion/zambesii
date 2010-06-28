#ifndef ___KSYMBOLS_H
	#define ___KSYMBOLS_H

	#include <__kstdlib/__ktypes.h>

/* These are the symbols defined in the linker script which delineate the
 * kernel's ELF sections.
 **/
extern "C" uarch_t		__kstart, __kend, __kphysStart, __kphysEnd;
extern "C" uarch_t		__kvirtStart, __kvirtEnd;
extern "C" uarch_t		__korientationStart, __korientationEnd;
extern "C" uarch_t		__ksetupStart, __ksetupEnd;
extern "C" uarch_t		__kctorStart, __kctorEnd, __kdtorStart, __kdtorEnd;
extern "C" uarch_t		__korientationStart, __korientationEnd;
extern "C" uarch_t		__kinitstart, __kinitEnd, __kfiniStart, __kfiniEnd;
extern "C" uarch_t		__ktextStart, __ktextEnd;
extern "C" uarch_t		__kdataStart, __kdataEnd;
extern "C" uarch_t		__kbssStart, __kbssEnd;

#endif

