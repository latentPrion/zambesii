#ifndef CHIPSET_INCLUDE

#include <config.h>

#ifdef CONFIG_CHIPSET_IBM_PC
	#define CHIPSET_INCLUDE(__subpath)		<chipset/ibmPc/__subpath>
	#define CHIPSET_SOURCE_INCLUDE(__subpath)	<chipset/ibmPc/__subpath>
#elif defined(CONFIG_CHIPSET_GENERIC)
	#define CHIPSET_INCLUDE(__subpath)		<chipset/generic/__subpath>
	#define CHIPSET_INCLUDE(__subpath)		<chipset/generic/__subpath>

#else
	#warning "No chipset selected in configure script. Defaulting to \
	CONFIG_CHIPSET_GENERIC. Try --chipset=MY_CHIPSET."

	#define CONFIG_CHIPSET_GENERIC
	#define CHIPSET_INCLUDE(__subpath)		<chipset/generic/__subpath>
	#define CHIPSET_INCLUDE(__subpath)		<chipset/generic/__subpath>
#endif

#endif

