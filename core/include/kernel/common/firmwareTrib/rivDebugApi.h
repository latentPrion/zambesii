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

#define NOTICE			(utf8Char *)"Notice: "
#define WARNING			(utf8Char *)"Warning: "
#define ERROR			(utf8Char *)"Error: "
#define FATAL			(utf8Char *)"Fatal: "

void rivPrintf(utf8Char *str, ...);

#ifdef __cplusplus
}
#endif

#endif

