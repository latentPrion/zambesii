
#define UDI_VERSION	0x101
#include <udi.h>


static const udi_ubit8_t	blankFlags=0;
static const udi_ops_init_t	zramdisk_ops_init_info[] =
{
	{ 1, 2, 1, 0, (void (**)())0xFEEDED, &blankFlags },
	{ 2, 2, 2, 0, (void (**)())0xFEEDED, &blankFlags },
	{ 0, 0, 0, 0, NULL, 0 }
};

static const udi_ubit8_t	zramdisk_mgmt_ops_flags = 0;
static const udi_primary_init_t	zramdisk_primary_init_info =
{
	NULL,
	&zramdisk_mgmt_ops_flags,
	0, 0, 0, 0,
	0
};

extern "C" const udi_init_t	zramdisk_init_info =
{
	&zramdisk_primary_init_info,
	NULL,
	zramdisk_ops_init_info,
	NULL, NULL, NULL
};

