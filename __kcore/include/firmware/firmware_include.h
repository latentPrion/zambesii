#ifndef FIRMWARE_INCLUDE

#include <config.h>

#ifdef CONFIG_FIRMWARE_IBM_PC_BIOS
	#define FIRMWARE_INCLUDE(__subpath)			<firmware/ibmPcBios/__subpath>
	#define FIRMWARE_SOURCE_INCLUDE(__subpath)		<firmware/ibmPcBios/__subpath>
#else
	#error "No firmware chosen in configuration script. Try \
	--firmware=MY_FIRMWARE."
#endif

#endif

