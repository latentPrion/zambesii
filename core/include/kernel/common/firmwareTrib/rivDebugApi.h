#ifndef _RIVULET_DEBUG_API_H
	#define _RIVULET_DEBUG_API_H

	#include <__kstdlib/__ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NOTICE
	#undef NOTICE
	#undef ERROR
	#undef WARNING
	#undef FATAL
#endif

#define NOTICE			(utf8Char *)"N: "
#define WARNING			(utf8Char *)"W: "
#define ERROR			(utf8Char *)"E: "
#define FATAL			(utf8Char *)"F: "

void rivPrintf(utf8Char *str, ...);

#ifdef __cplusplus
}
#endif

#endif

