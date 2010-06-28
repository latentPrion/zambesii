#ifndef LANG_INCLUDE

	#include <config.h>

#ifdef CONFIG_LANG_EN_UK
	#define LANG_SOURCE_INCLUDE(__subpath)	<lang/en-uk/__subpath>
#else
	#warning "No language chosen in configure. Try --lang=MY_LANG. \
		Defaulting to en-uk."

	#define CONFIG_LANG_EN_UK
	#define LANG_SOURCE_INCLUDE(__subpath)	<lang/en-uk/__subpath>
#endif

#endif

