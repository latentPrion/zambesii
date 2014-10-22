#ifndef _FLOODPLAINN_ZAMBESII_UDI_ENV_PUBLIC_API_H
	#define _FLOODPLAINN_ZAMBESII_UDI_ENV_PUBLIC_API_H

	#define UDI_VERSION 0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/floodplainn/zui.h>
	#include <kernel/common/floodplainn/fvfs.h> // FVFS_PATH_MAXLEN
	#include <kernel/common/floodplainn/channel.h>

#define ZUP			"ZUP: "

class Thread;

namespace fplainn
{
	class Zum;
}

class fplainn::Zum
{
public:
	Zum();

	error_t initialize(void);
	~Zum(void);

public:
	/* Causes the ZUM server to connect("udi_mgmt") to the device, and then
	 * send the sequence of initialization calls to it.
	 **/
	void startDeviceReq(utf8Char *devicePath, void *privateData);
	void finalCleanupReq(utf8Char *devicePath, void *privateData);

	void suspendDeviceReq(utf8Char *devicePath, void *privateData);
	void wakeDeviceReq(utf8Char *devicePath, void *privateData);
	void shutdownDeviceReq(utf8Char *devicePath, void *privateData);

	void parentSuspendedInd(utf8Char *devicePath, void *privateData);
	void unbindFromParentReq(utf8Char *devicePath, void *privateData);

	void deviceManagementReq(
		utf8Char *devicePath, udi_ubit8_t op, ubit16 parentId,
		void *privateData);

	// TODO: later.
	void enumerateReq(
		utf8Char *devicePath, udi_ubit8_t enumLevel, void *privateData);


private:
	struct sZAsyncMsg
	{
		sZAsyncMsg(utf8Char *path, ubit16 opsIndex)
		:
		opsIndex(opsIndex)
		{
			strncpy8(this->path, path, FVFS_PATH_MAXLEN);
			this->path[FVFS_PATH_MAXLEN - 1] = '\0';
		}

		ubit16			opsIndex;
		utf8Char		path[FVFS_PATH_MAXLEN];
		union
		{
			struct
			{
				udi_mgmt_cb_t		cb;

				udi_ubit8_t		mgmt_op;
				udi_ubit8_t		parent_ID;

				udi_ubit8_t		flags;
				udi_status_t		status;
			} devmgmt;

			struct
			{
				udi_enumerate_cb_t	cb;

				udi_ubit8_t		enumeration_level;

				udi_ubit8_t		enumeration_result;
				udi_index_t		ops_idx;
			} enumerate;

			struct
			{
				udi_usage_cb_t		cb;
				udi_ubit8_t		resource_level;
			} usage;

			struct
			{
				udi_mgmt_cb_t		cb;
			} final_cleanup;
		} params;
	};

	processId_t		serverTid;
	Thread			*thread;

	static void main(void *);
};

#endif

