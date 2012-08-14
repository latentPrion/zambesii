#ifndef _SIZE_T_H
	#define _SIZE_T_H

// GNU Compiler Collection.
#ifdef __GNUC__
	// For GCC 4.5+, stdint is different, and the old one won't work.
	#if ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 5) \
		&& __GNUC_PATCHLEVEL__ <= 3)
		typedef unsigned int size_t;
		typedef signed int ssize_t;
	#else
		typedef unsigned long int size_t;
		typedef signed long int ssize_t;
	#endif
#else
	#error "No size_t.h found for your compiler. Please provide one."
#endif

#endif

