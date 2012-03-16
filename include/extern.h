#ifndef _CPP_EXTERN_H
	#define _CPP_EXTERN_H

#ifdef __cplusplus
	#define CPPEXTERN_START			extern "C" {
	#define CPPEXTERN_END			}
	#define CPPEXTERN_PROTO			extern "C"
#else
	#define CPPEXTERN_START
	#define CPPEXTERN_END
	#define CPPEXTERN_PROTO			extern
#endif
	
#endif

