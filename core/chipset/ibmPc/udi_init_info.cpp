
#define UDI_VERSION 0x101
#include <udi.h>
#undef UDI_VERSION
#include <__kstdlib/__ktypes.h>
#include <kernel/common/floodplainn/floodplainn.h>

/**	EXPLANATION:
 * In this file, every UDI driver and library provided by this chipset shall
 * be listed. Simply create an entry for it with its shortname, a pointer to
 * its udi_init_info or mei_init_info, and that is all that is required.
 *
 * The kernel is equipped with the intelligence to do all the rest of the work.
 **/

/**	XXX:
 * Place your udi_init_info declaration here.
 **/
extern udi_init_t	zramdisk_init_info;
extern udi_mei_init_t	udi_gio_meta_info;

const driverInitEntryS		driverInitInfo[] =
{
	{ CC"zramdisk", &zramdisk_init_info },
	// Shall always be terminated with the NULL entry.
	{ NULL, NULL }
};

const metaInitEntryS		metaInitInfo[] =
{
	{ CC"udi_gio", &udi_gio_meta_info },
	{ NULL, NULL }
};

