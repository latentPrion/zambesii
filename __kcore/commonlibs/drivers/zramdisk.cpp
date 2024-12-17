
#define UDI_VERSION	0x101
#include <udi.h>


static const udi_ubit8_t	blankFlags=0;
static const udi_ops_init_t	zramdisk_ops_init_info[] =
{
	{ 1, 2, 1, 0, (void (**)())0xFEEDED, &blankFlags },
	{ 2, 2, 2, 0, (void (**)())0xFEEDED, &blankFlags },
	{ 0, 0, 0, 0, NULL, 0 }
};

static udi_usage_ind_op_t		zramdisk_usage_ind;
static udi_devmgmt_req_op_t		zramdisk_devmgmt_req;
static udi_enumerate_req_op_t		zramdisk_enumerate_req;
static udi_final_cleanup_req_op_t	zramdisk_final_cleanup_req;

static const udi_mgmt_ops_t	zramdisk_mgmt_ops =
{
	&zramdisk_usage_ind,
	&zramdisk_enumerate_req,
	&zramdisk_devmgmt_req,
	&zramdisk_final_cleanup_req
};

static const udi_ubit8_t	zramdisk_mgmt_ops_flags = 0;
static const udi_primary_init_t	zramdisk_primary_init_info =
{
	&zramdisk_mgmt_ops,
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

#include <__kclasses/debugPipe.h>
static void zramdisk_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	(void)cb; (void)resource_level;
	printf(NOTICE"Is that driver code?\?!\n");
}

static void zramdisk_enumerate_req(
	udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level
	)
{
	(void)cb; (void)enumeration_level;
}

static void zramdisk_devmgmt_req(
	udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID
	)
{
	(void)cb; (void)mgmt_op; (void)parent_ID;
}

static void zramdisk_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	(void)cb;
}
