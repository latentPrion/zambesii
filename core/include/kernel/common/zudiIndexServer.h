#ifndef _ZUDI_READER_PROTOCOL_H
	#define _ZUDI_READER_PROTOCOL_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <kernel/common/processId.h>

namespace zudiIndexServer
{
	enum indexE { INDEX_KERNEL=0, INDEX_RAMDISK, INDEX_EXTERNAL };
	enum newDeviceActionE {
		NDACTION_NOTHING=0, NDACTION_DETECT_DRIVER,
		NDACTION_LOAD_DRIVER, NDACTION_SPAWN_DRIVER,
		NDACTION_INSTANTIATE };

	#define ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN			(256)

	struct zudiIndexMsgS
	{
	public:
		zudiIndexMsgS(
			ubit8 command, utf8Char *path,
			indexE index, newDeviceActionE action=NDACTION_NOTHING)
		:
		command(command), index(index), action(action)
		{
			if (path != NULL)
			{
				strncpy8(
					this->path, path,
					ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN
						- 1);

				this->path[
					ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN
						- 1] = '\0';
			};
		}

		#define ZUDIIDX_SERVER_ADDINDEX_REQ			(0)
		#define ZUDIIDX_SERVER_REMOVEINDEX_REQ			(1)
		#define ZUDIIDX_SERVER_SET_NEWDEVICE_ACTION_REQ		(2)
		#define ZUDIIDX_SERVER_GET_NEWDEVICE_ACTION_REQ		(3)
		#define ZUDIIDX_SERVER_DETECTDRIVER_REQ			(4)
		#define ZUDIIDX_SERVER_LOADDRIVER_REQ			(5)
		#define ZUDIIDX_SERVER_LOAD_REQUIREMENTS_REQ		(6)
		#define ZUDIIDX_SERVER_NEWDEVICE_IND			(7)

		ubit8			command;
		indexE			index;
		newDeviceActionE	action;
		processId_t		processId;
		utf8Char		path[
			ZUDIIDX_SERVER_MSG_DEVICEPATH_MAXLEN];
	};

	/**	EXPLANATION:
	 * API for communicating with the UDI Index server.
	 *	connect():
	 		Establishes a connection to the server.
	 *	detectDriverReq():
	 *		Takes the path of a device, and searches the index for
	 *		a driver that matches it. Takes an argument for the
	 *		maximum index to use when searching. The kernel will
	 *		always search the outermost index first.
	 *	loadDriverReq():
	 *		Takes the path of a device which already has had a
	 * 		driver detected for it, and loads the driver's
	 *		information into the kernel.
	 *	setOptionsReq():
	 *		Sets the default action to be taken when a new device
	 *		is reported, and the default index to be searched when
	 *		trying to detect a driver for a device.
	 *	getOptionsReq():
	 *		Gets the current default options.
	 *	newDeviceInd():
	 *		Notifies the server of the existence of a new device.
	 **/
	error_t detectDriverReq(
		utf8Char *devicePath, indexE index,
		void *privateData, ubit32 flags);

	error_t loadDriverReq(utf8Char *devicePath, void *privateData);
	void loadDriverRequirementsReq(void *privateData);

	void newDeviceInd(
		utf8Char *devicePath, indexE index, void *privateData);

	void setNewDeviceActionReq(newDeviceActionE action, void *privateData);
	void getNewDeviceActionReq(void *privateData);
}

#endif

