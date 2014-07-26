#ifndef _ZUDI_READER_PROTOCOL_H
	#define _ZUDI_READER_PROTOCOL_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <kernel/common/processId.h>
	#include <kernel/common/floodplainn/fvfs.h> // FVFS_PATH_MAXLEN

namespace zuiServer
{
	enum indexE { INDEX_KERNEL=0, INDEX_RAMDISK, INDEX_EXTERNAL };
	enum newDeviceActionE {
		NDACTION_NOTHING=0, NDACTION_DETECT_DRIVER,
		NDACTION_LOAD_DRIVER, NDACTION_SPAWN_DRIVER,
		NDACTION_INSTANTIATE };

	struct indexMsgS
	{
	public:
		indexMsgS(
			ubit8 command, utf8Char *path,
			indexE index, newDeviceActionE action=NDACTION_NOTHING)
		:
		command(command), index(index), action(action)
		{
			if (path != NULL)
			{
				strncpy8(
					this->path, path, FVFS_PATH_MAXLEN);

				this->path[FVFS_PATH_MAXLEN - 1] = '\0';
			};
		}

		#define ZUISERVER_ADDINDEX_REQ			(0)
		#define ZUISERVER_REMOVEINDEX_REQ			(1)
		#define ZUISERVER_SET_NEWDEVICE_ACTION_REQ		(2)
		#define ZUISERVER_GET_NEWDEVICE_ACTION_REQ		(3)
		#define ZUISERVER_DETECTDRIVER_REQ			(4)
		#define ZUISERVER_LOADDRIVER_REQ			(5)
		#define ZUISERVER_LOAD_REQUIREMENTS_REQ		(6)
		#define ZUISERVER_NEWDEVICE_IND			(7)

		ubit8			command;
		indexE			index;
		newDeviceActionE	action;
		processId_t		processId;
		utf8Char		path[FVFS_PATH_MAXLEN];
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

