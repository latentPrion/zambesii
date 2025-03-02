#ifndef FIRMWARE_INCLUDE

#include <config.h>

#ifdef CONFIG_FIRMWARE_IBM_PC_BIOS
	#define FIRMWARE_INCLUDE(__subpath)			<firmware/ibmPcBios/__subpath>
	#define FIRMWARE_SOURCE_INCLUDE(__subpath)		<firmware/ibmPcBios/__subpath>
#elif defined(CONFIG_FIRMWARE_UEFI)
	#define FIRMWARE_INCLUDE(__subpath)			<firmware/uefi/__subpath>
	#define FIRMWARE_SOURCE_INCLUDE(__subpath)		<firmware/uefi/__subpath>
#else
	#error "No firmware chosen in configuration script. Try \
	--firmware=MY_FIRMWARE."
#endif

#endif

