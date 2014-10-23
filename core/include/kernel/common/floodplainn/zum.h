#ifndef _FLOODPLAINN_ZAMBESII_UDI_MA_PUBLIC_API_H
	#define _FLOODPLAINN_ZAMBESII_UDI_MA_PUBLIC_API_H

	#define UDI_VERSION 0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/fvfs.h> // FVFS_PATH_MAXLEN

#define ZUM			"ZUM: "

class Thread;

namespace fplainn
{
	class Zum;
	class Endpoint;
}

class fplainn::Zum
{
public:
	Zum(void)
	:
	serverTid(PROCID_INVALID), server(NULL)
	{}

	error_t initialize(void);
	~Zum(void) {};

public:
	/* Causes the ZUM server to connect("udi_mgmt") to the device, and then
	 * send the sequence of initialization calls to it.
	 **/
	void startDeviceReq(
		utf8Char *devicePath, udi_ubit8_t resource_level,
		void *privateData);

	/* Causes the ZUM server to disconnect from the device, closing the
	 * management channel, and tearing down the device instance.
	 **/
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

	void usageInd(
		utf8Char *devicePath, udi_ubit8_t resource_level,
		void *privateData);

	union eventIndU {
		void setInternalParams(udi_cb_t *cb)
			{ internal_bound.bind_cb = cb; }

		void setParentParams(
			udi_cb_t *cb,
			udi_ubit8_t parentId, udi_buf_path_t *parentPath)
		{
			parent_bound.bind_cb = cb;
			parent_bound.parent_ID = parentId;
			parent_bound.path_handles = parentPath;
		}

		void setAbortParams(udi_cb_t *cb)
			{ orig_cb = cb; }

		struct {
			udi_cb_t *bind_cb;
		} internal_bound;
		struct {
			udi_cb_t *bind_cb;
			udi_ubit8_t parent_ID;
			udi_buf_path_t *path_handles;
		} parent_bound;
		udi_cb_t *orig_cb;
	};

	void internalChannelEventInd(
		utf8Char *devicePath, fplainn::Endpoint *endp,
		udi_ubit8_t event, void *privateData);

	void parentChannelEventInd(
		utf8Char *devicePath, fplainn::Endpoint *endp,
		udi_ubit8_t event, udi_ubit8_t parent_ID, void *privateData);

	void channelOpAbortedInd(
		utf8Char *devicePath, fplainn::Endpoint *endp,
		udi_cb_t *orig_cb, void *privateData);

	void channelEventInd(
		utf8Char *devicePath, fplainn::Endpoint *endp,
		udi_ubit8_t event, eventIndU *params, void *privateData);

	void setServerTid(processId_t tid)
		{ serverTid = tid; }

public:
	struct sZAsyncMsg
	{
		sZAsyncMsg(utf8Char *path, ubit16 opsIndex)
		:
		opsIndex(opsIndex)
		{
			if (path != NULL)
			{
				strncpy8(this->path, path, FVFS_PATH_MAXLEN);
				this->path[FVFS_PATH_MAXLEN - 1] = '\0';
			};
		}

		enum opE {
			OP_CHANNEL_EVENT_IND=0,
			OP_USAGE_IND, OP_ENUMERATE_REQ, OP_DEVMGMT_REQ,
			OP_FINAL_CLEANUP_REQ,
			OP_START_REQ };

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

			struct
			{
				udi_channel_event_cb_t	cb;

				fplainn::Endpoint	*endpoint;

				udi_status_t		status;
			} channel_event;
		} params;
	};

	struct sZumMsg
	{
		sZumMsg(
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
	 		uarch_t size, uarch_t flags, void *privateData)
	 	:
	 	header(
			targetPid, subsystem, function,
			size, flags, privateData),
		info(NULL, 0)
		{}

		MessageStream::sHeader		header;
		sZAsyncMsg			info;
	};

private:
	processId_t		serverTid;
	Thread			*server;

	static void main(void *);

	static sZAsyncMsg *getNewSZAsyncMsg(
		utf8Char *func, utf8Char *devPath, sZAsyncMsg::opE op);
};


/**	Inline methods:
 ******************************************************************************/

inline void fplainn::Zum::internalChannelEventInd(
	utf8Char *devicePath, fplainn::Endpoint *endp, udi_ubit8_t event,
	void *privateData
	)
{
	eventIndU		p;

	p.setInternalParams(NULL);
	channelEventInd(devicePath, endp, event, &p, privateData);
}

inline void fplainn::Zum::parentChannelEventInd(
	utf8Char *devicePath, fplainn::Endpoint *endp, udi_ubit8_t event,
	udi_ubit8_t parent_ID, void *privateData
	)
{
	eventIndU		p;

	// The buffer path info will be set by the ZUM server.
	p.setParentParams(NULL, parent_ID, NULL);
	channelEventInd(devicePath, endp, event, &p, privateData);
}

inline void fplainn::Zum::channelOpAbortedInd(
	utf8Char *devicePath, fplainn::Endpoint *endp, udi_cb_t *orig_cb,
	void *privateData
	)
{
	eventIndU		p;

	p.setAbortParams(orig_cb);
	channelEventInd(
		devicePath, endp, UDI_CHANNEL_OP_ABORTED, &p,
		privateData);
}

#endif

