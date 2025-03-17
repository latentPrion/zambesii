#ifndef _FLOODPLAINN_ZAMBESII_UDI_MA_PUBLIC_API_H
	#define _FLOODPLAINN_ZAMBESII_UDI_MA_PUBLIC_API_H

	#define UDI_VERSION 0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kcxxlib/new>
	#include <kernel/common/processId.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/movableMemory.h>
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

	/* Causes the ZUM server to execute a full cycle of udi_enumerate_req
	 * calls into a device. The kernel will build the device's children
	 * using the information gathered. This function will return a buffer
	 * of strings.
	 *
	 * The buffer will contain UTF-8 device paths, separated by ascii NULL
	 * bytes. The terminating sequence is two contiguous NULLs.
	 *
	 * There is no way for the kernel to predict how many child devices the
	 * target device will enumerate when this function is called, so we
	 * simply recommend passing in a reasonably overcompensated buffer for
	 * your output.
	 **/
	#define ZUM_ENUMCHILDREN_FLAGS_TEST_RUN			(1<<0)
	#define ZUM_ENUMCHILDREN_FLAGS_UNCACHED_SCAN		(1<<1)
	// We should eventually add support for filter_list to be passed to this.
	void enumerateChildrenReq(
		utf8Char *devicePath, udi_enumerate_cb_t *ecb,
		uarch_t flags, void *privateData);

	void postManagementCbReq(
		utf8Char *devicePath, udi_enumerate_cb_t *ecb,
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

	void enumerateReq(
		utf8Char *devicePath, udi_ubit8_t enumLevel,
		udi_enumerate_cb_t *cb, void *privateData);

	error_t unmarshalEnumerateAckAttrsAndFilters(
		udi_enumerate_cb_t *enumCb,
		udi_instance_attr_list_t *attrList,
		udi_filter_element_t *filterList);

	// Forward declaration of implementation class
public:
	class EnumerateReqMovableMemMarshaller
	{
	public:
		EnumerateReqMovableMemMarshaller(uarch_t nAttrs, uarch_t nFilters);

		static uarch_t calcMemRequirementsFor(
			uarch_t nAttrs, uarch_t nFilters);

		// Individual marshal/unmarshal methods
		void marshalAttrList(
			const udi_instance_attr_list_t *const srcAttrList,
			uarch_t nAttrs);
		void marshalFilterList(
			const udi_filter_element_t *const srcFilterList,
			uarch_t nAttrs, uarch_t nFilters);
		void unmarshalAttrList(
			udi_instance_attr_list_t *destAttrList,
			uarch_t nAttrs) const;
		void unmarshalFilterList(
			udi_filter_element_t *destFilterList,
			uarch_t nAttrs, uarch_t nFilters) const;

		// Wrapper methods that handle both attr and filter lists
		void marshal(
			const udi_instance_attr_list_t *const srcAttrList, uarch_t nAttrs,
			const udi_filter_element_t *const srcFilterList, uarch_t nFilters);
		void unmarshal(
			udi_instance_attr_list_t *destAttrList, uarch_t nAttrs,
			udi_filter_element_t *destFilterList, uarch_t nFilters) const;

		// Pointer calculation methods
		fplainn::sMovableMemory *calcAttrMovableMemHeaderPtr(void) const;
		udi_instance_attr_list_t *calcAttrListPtr(uarch_t nAttrs) const;
		fplainn::sMovableMemory *calcFilterMovableMemHeaderPtr(uarch_t nAttrs) const;
		udi_filter_element_t *calcFilterListPtr(
			uarch_t nAttrs, uarch_t nFilters) const;
	};

	void usageInd(
		utf8Char *devicePath, udi_ubit8_t resource_level,
		void *privateData);

	union eventIndU
	{
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
	/**
	 * @brief Message structure used for communication with the ZUM server
	 *
	 * This structure is used to send requests to the ZUM backend server.
	 * It contains the necessary information for the server to
	 * process requests related to device management operations.
	 * These messages are sent TO the ZUM server from clients
	 * that need to interact with devices.
	 */
	struct sZumServerMsg
	{
		sZumServerMsg(
			utf8Char *path, ubit16 opsIndex,
			uarch_t extraMemNBytes=0)
		:
		extraMemNBytes(extraMemNBytes),
		opsIndex(opsIndex)
		{
			if (path != NULL)
			{
				strncpy8(this->path, path, FVFS_PATH_MAXLEN);
				this->path[FVFS_PATH_MAXLEN - 1] = '\0';
			};
		}

		// Copy constructor
		sZumServerMsg(const sZumServerMsg& other)
		{
			// memcpy whole obj for simplicity and maintainability
			memcpy(this, &other, sizeof(sZumServerMsg));
		}

		enum opE {
			OP_CHANNEL_EVENT_IND=0,
			OP_USAGE_IND, OP_ENUMERATE_REQ, OP_DEVMGMT_REQ,
			OP_FINAL_CLEANUP_REQ,
			OP_START_REQ, OP_ENUMERATE_CHILDREN_REQ,
			OP_POST_MANAGEMENT_CB_REQ };

		void *operator new(size_t n, uarch_t extra)
			{ return ::operator new(n + extra); }

		void operator delete(void *m, uarch_t)
			{ ::operator delete(m); }

		void operator delete(void *m)
			{ operator delete(m, 0); }

		uarch_t			extraMemNBytes;
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

			struct
			{
				struct sChannelEventTracker
				{
					/* nChannels Starts as -1.
					 * The caller can detect
					 * whether or not the kernel tried to
					 * do the internal-bind init sequence
					 * by checking this value, since it
					 * will be >= 0 if the kernel started
					 * the internal-bind init sequence.
					 **/
					sarch_t		nChannels,
							nSuccesses, nFailures;
				} ibind, pbind;
			} start;

			struct
			{
				// We need the cb here only for the filter_list.
				udi_enumerate_cb_t	cb;

				udi_ubit8_t
						final_enumeration_result;
				uarch_t			flags;
				uarch_t			nDeviceIds;
				ubit32			*deviceIdsHandle;
			} enumerateChildren;

			struct
			{
				/* cb is needed here for the filter_list and
				 * attr_list. The attr_list is an input here
				 * because this request is meant to be used
				 * for directed enumeration.
				 **/
				udi_enumerate_cb_t	cb;

				udi_ubit8_t		enumeration_result;
				udi_index_t		ops_idx;
			} postManagementCb;
		} params;
	};

	/**
	 * @brief Message structure used for communication between ZUM server and devices
	 *
	 * This structure is used by the ZUM server to communicate with devices being managed.
	 * It encapsulates a MessageStream header along with a sZumServerMsg for the actual
	 * request data. These messages are sent FROM the ZUM server TO the devices being
	 * managed, and are used to carry out device management operations.
	 */
	struct sZumDeviceMgmtMsg
	{
		sZumDeviceMgmtMsg(
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(
			targetPid, subsystem, function,
			size, flags, privateData),
		info(NULL, 0)
		{
			// Empty constructor body
		}

		MessageStream::sHeader		header;
		sZumServerMsg			info;
	};

private:
	processId_t		serverTid;
	Thread			*server;

	static void main(void *);

	static sZumServerMsg *createServerRequestMsg(
		utf8Char *func, utf8Char *devPath, sZumServerMsg::opE op,
		uarch_t extraMemNBytes=0);
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

	/* The buffer path *is* the parent_ID in Zambesii. But that will be
	 * taken care of by libzbzcore.
	 */
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

/* Movable Objects marshaling class for EnumerateReq
 ******************************************************************************/

inline fplainn::Zum::EnumerateReqMovableMemMarshaller::EnumerateReqMovableMemMarshaller(
	uarch_t nAttrs, uarch_t nFilters)
{
	/** FIXME:
	 * I think this is buggy. AFAICT we allocate one movable memory
	 * chunk for both the attr_list and the filter_list. In here we
	 * appear to be initializing two headers, one for each list.
	 * Double-check and fix if necessary.
	 **/
	if (nAttrs > 0)
	{
		::new (calcAttrMovableMemHeaderPtr())
		fplainn::sMovableMemory(
			sizeof(udi_instance_attr_list_t) * nAttrs);
	};

	if (nFilters > 0)
	{
		::new (calcFilterMovableMemHeaderPtr(nAttrs))
		fplainn::sMovableMemory(
			sizeof(udi_filter_element_t) * nFilters);
	}
}

inline fplainn::sMovableMemory *
fplainn::Zum::EnumerateReqMovableMemMarshaller::calcAttrMovableMemHeaderPtr(
	void) const
{
	// Even if nAttrs==0, return "this".
	return const_cast<fplainn::sMovableMemory *>(
		reinterpret_cast<const fplainn::sMovableMemory *>(this));
}

inline udi_instance_attr_list_t *
fplainn::Zum::EnumerateReqMovableMemMarshaller::calcAttrListPtr(
	uarch_t nAttrs) const
{
	if (nAttrs > 0)
	{
		return const_cast<udi_instance_attr_list_t *>(
			reinterpret_cast<const udi_instance_attr_list_t *>(
				calcAttrMovableMemHeaderPtr() + 1));
	};

	return const_cast<udi_instance_attr_list_t *>(
		reinterpret_cast<const udi_instance_attr_list_t *>(this));
}

inline fplainn::sMovableMemory *
fplainn::Zum::EnumerateReqMovableMemMarshaller::calcFilterMovableMemHeaderPtr(
	uarch_t nAttrs) const
{
	return const_cast<fplainn::sMovableMemory *>(
		reinterpret_cast<const fplainn::sMovableMemory *>(
			calcAttrListPtr(nAttrs) + nAttrs));
}

inline udi_filter_element_t *
fplainn::Zum::EnumerateReqMovableMemMarshaller::calcFilterListPtr(
	uarch_t nAttrs, uarch_t nFilters) const
{
	if (nFilters > 0)
	{
		return const_cast<udi_filter_element_t *>(
			reinterpret_cast<const udi_filter_element_t *>(
				calcFilterMovableMemHeaderPtr(nAttrs) + 1));
	};

	return const_cast<udi_filter_element_t *>(
		reinterpret_cast<const udi_filter_element_t *>(
			calcFilterMovableMemHeaderPtr(nAttrs)));
}

inline uarch_t fplainn::Zum::EnumerateReqMovableMemMarshaller::calcMemRequirementsFor(
	uarch_t nAttrs, uarch_t nFilters)
{
	uarch_t ret=0;

	if (nAttrs > 0)
	{
		ret += sizeof(fplainn::sMovableMemory);
		ret += sizeof(udi_instance_attr_list_t) * nAttrs;
	};

	if (nFilters > 0)
	{
		ret += sizeof(fplainn::sMovableMemory);
		ret += sizeof(udi_filter_element_t) * nFilters;
	};

	return ret;
}

#endif

