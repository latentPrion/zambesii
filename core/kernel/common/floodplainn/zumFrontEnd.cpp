
#include <kernel/common/floodplainn/zum.h>

/**	EXPLANATION:
 * Like the ZUI server, the ZUM server will also take commands over a ZAsync
 * Stream connection.
 *
 * This thread essentially implements all the logic required to allow the kernel
 * to call into a device on its udi_mgmt channel. It doesn't care about any
 * other channels.
 **/

void fplainn::Zum::deviceManagementReq(
	utf8Char *devicePath, udi_ubit8_t op, ubit16 parentId,
	void *privateData
	)
{

}
