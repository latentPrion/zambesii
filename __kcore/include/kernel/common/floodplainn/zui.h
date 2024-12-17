#ifndef _ZAMBESII_UDI_INDEX_SERVER_PUBLIC_API_H
	#define _ZAMBESII_UDI_INDEX_SERVER_PUBLIC_API_H

	#include <kernel/common/processId.h>
	#include <kernel/common/floodplainn/fvfs.h> // FVFS_PATH_MAXLEN
	#include <kernel/common/messageStream.h>

/**	EXPLANATION:
 * The Zambesii UDI Index server public API.
 *
 * Zambesii UDI Index server is responsible for maintaining metadata about
 * drivers that have been loaded by the kernel, and for matching devices with
 * their appropriate drivers.
 *
 * It maintains several driver indexes, which serve as lookups for driver and
 * metalanguage metadata.
 ******************************************************************************/

#define ZUI			"ZUI: "

class Thread;
namespace fplainn { class Zui; }

class fplainn::Zui
{
public:
	enum indexE { INDEX_KERNEL=0, INDEX_RAMDISK, INDEX_EXTERNAL };
	enum newDeviceActionE {
		NDACTION_NOTHING=0, NDACTION_DETECT_DRIVER,
		NDACTION_LOAD_DRIVER, NDACTION_SPAWN_DRIVER,
		NDACTION_INSTANTIATE };

	Zui(void) {}

	error_t initialize(void);
	~Zui(void) {}

public:
	/**	EXPLANATION:
	 * API for communicating with the UDI Index server.
	 *	detectDriverReq():
	 *		Takes the path of a device, and searches the index for
	 *		a driver that matches it. Takes an argument for the
	 *		maximum index to use when searching. The kernel will
	 *		always search the outermost index first.
	 *	loadDriverReq():
	 *		Takes the path of a device which already has had a
	 * 		driver detected for it, and loads the driver's
	 *		information into the kernel.
	 *	setNewDeviceActionReq():
	 *		Sets the default action to be taken when a new device
	 *		is reported, and the default index to be searched when
	 *		trying to detect a driver for a device.
	 *	getNewDeviceActionReq():
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

	Thread *getServerThread(void)
		{ return server; }

	processId_t getServerTid(void)
		{ return serverTid; }

	void setServerTid(processId_t tid)
		{ serverTid = tid; }

public:
	struct sIndexZAsyncMsg
	{
	public:
		sIndexZAsyncMsg(
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

		#define MSGSTREAM_ZUI_ADDINDEX_REQ			(0)
		#define MSGSTREAM_ZUI_REMOVEINDEX_REQ			(1)
		#define MSGSTREAM_ZUI_SET_NEWDEVICE_ACTION_REQ		(2)
		#define MSGSTREAM_ZUI_GET_NEWDEVICE_ACTION_REQ		(3)
		#define MSGSTREAM_ZUI_DETECTDRIVER_REQ			(4)
		#define MSGSTREAM_ZUI_LOADDRIVER_REQ			(5)
		#define MSGSTREAM_ZUI_LOAD_REQUIREMENTS_REQ		(6)
		#define MSGSTREAM_ZUI_NEWDEVICE_IND			(7)

		ubit8			command;
		indexE			index;
		newDeviceActionE	action;
		processId_t		processId;
		utf8Char		path[FVFS_PATH_MAXLEN];
	};

	struct sIndexMsg
	{
		sIndexMsg(
			processId_t targetPid,
			ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(
			targetPid, subsystem, function,
			size, flags, privateData),
		info(0, NULL, fplainn::Zui::INDEX_KERNEL)
		{}

		void set(
			ubit8 command, utf8Char *path,
			fplainn::Zui::indexE index,
			fplainn::Zui::newDeviceActionE action=
				fplainn::Zui::NDACTION_NOTHING)
		{
			this->info.command = command;
			this->info.index = index;
			this->info.action = action;
			if (path != NULL)
			{
				strncpy8(
					this->info.path, path,
					FVFS_PATH_MAXLEN);

				this->info.path[FVFS_PATH_MAXLEN - 1] =
					'\0';
			};
		}

		MessageStream::sHeader		header;
		fplainn::Zui::sIndexZAsyncMsg	info;
	};

	static void main(void *);

private:
	processId_t			serverTid;
	Thread				*server;
};

#endif
