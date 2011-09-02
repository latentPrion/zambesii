#ifndef ___KSYMBOLS_H
	#define ___KSYMBOLS_H

	#include <__kstdlib/__ktypes.h>

/* These are the symbols defined in the linker script which delineate the
 * kernel's ELF sections.
 **/

// Don't let C code see the 'extern "C"' used for C++.
#ifdef __cplusplus
	#define EXTERN		extern "C"
#else
	#define EXTERN		extern
#endif

EXTERN uarch_t		__kstart, __kend, __kphysStart, __kphysEnd;
EXTERN uarch_t		__kvirtStart, __kvirtEnd;
EXTERN uarch_t		__korientationStart, __korientationEnd;
EXTERN uarch_t		__ksetupStart, __ksetupEnd;
EXTERN uarch_t		__kctorStart, __kctorEnd, __kdtorStart, __kdtorEnd;
EXTERN uarch_t		__korientationStart, __korientationEnd;
EXTERN uarch_t		__kinitstart, __kinitEnd, __kfiniStart, __kfiniEnd;
EXTERN uarch_t		__ktextStart, __ktextEnd;
EXTERN uarch_t		__kdataStart, __kdataEnd;
EXTERN uarch_t		__kbssStart, __kbssEnd;
EXTERN uarch_t		__kcpuPowerOnTextStart, __kcpuPowerOnTextEnd;
EXTERN uarch_t		__kcpuPowerOnDataStart, __kcpuPowerOnDataEnd;

#undef EXTERN

#endif

