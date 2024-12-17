
#include <__kstdlib/__kcxxlib/memory>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string.h>
#include <kernel/common/process.h>
#include <kernel/common/panic.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/floodplainn/zudi.h>
#include <kernel/common/floodplainn/device.h>
#include <kernel/common/floodplainn/movableMemory.h>


error_t fplainn::Zudi::initialize(void)
{
	error_t		ret;

	ret = driverList.initialize();
	if (ret != ERROR_SUCCESS) { return ret; };

	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::findDriver(utf8Char *fullName, fplainn::Driver **retDrv)
{
	HeapArr<utf8Char>	tmpStr;
	fplainn::Driver		*tmpDrv;

	tmpStr = new utf8Char[DRIVER_FULLNAME_MAXLEN];

	if (tmpStr.get() == NULL) { return ERROR_MEMORY_NOMEM; };

	HeapList<fplainn::Driver>::Iterator		it =
		driverList.begin(PTRLIST_FLAGS_NO_AUTOLOCK);

	driverList.lock();
	for (; it != driverList.end(); ++it)
	{
		uarch_t		len;

		tmpDrv = *it;

		len = strlen8(tmpDrv->basePath);
		strcpy8(tmpStr.get(), tmpDrv->basePath);
		if (len > 0 && tmpStr[len - 1] != '/')
			{ strcat8(tmpStr.get(), CC"/"); };

		strcat8(tmpStr.get(), tmpDrv->shortName);

		if (strcmp8(tmpStr.get(), fullName) == 0)
		{
			driverList.unlock();
			*retDrv = tmpDrv;
			return ERROR_SUCCESS;
		};
	};

	driverList.unlock();
	return ERROR_NO_MATCH;
}

error_t fplainn::Zudi::instantiateDeviceReq(utf8Char *path, void *privateData)
{
	fplainn::Zudi::sKernelCallMsg			*request;
	fplainn::Device				*dev;
	error_t					ret;
	HeapObj<fplainn::DeviceInstance>	devInst;

	if (path == NULL) { return ERROR_INVALID_ARG; };
	if (strnlen8(path, FVFS_PATH_MAXLEN) >= FVFS_PATH_MAXLEN)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	ret = floodplainn.getDevice(path, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	// Ensure that spawnDriver() has been called beforehand.
	if (!dev->driverHasBeenDetected() || dev->driverInstance == NULL)
		{ return ERROR_UNINITIALIZED; };

	if (dev->instance == NULL)
	{
		// Allocate the device instance.
		devInst = new fplainn::DeviceInstance(dev);
		if (devInst == NULL) { return ERROR_MEMORY_NOMEM; };

		ret = devInst->initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		ret = dev->driverInstance->addHostedDevice(path);
		if (ret != ERROR_SUCCESS) { return ret; };
	};

	request = new fplainn::Zudi::sKernelCallMsg(
		dev->driverInstance->pid,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI___KCALL,
		sizeof(*request), 0, privateData);

	if (request == NULL)
	{
		dev->driverInstance->removeHostedDevice(path);
		return ERROR_MEMORY_NOMEM;
	};

	// Assign the device instance to dev->instance and release the mem.
	request->set(path, fplainn::Zudi::sKernelCallMsg::CMD_INSTANTIATE_DEVICE);
	if (dev->instance == NULL) { dev->instance = devInst.release(); };
	return MessageStream::enqueueOnThread(
		request->header.targetId, &request->header);
}

void fplainn::Zudi::instantiateDeviceAck(
	processId_t targetId, utf8Char *path, error_t err, void *privateData
	)
{
	Thread				*currThread;
	fplainn::Zudi::sKernelCallMsg	*response;
	fplainn::Device			*dev;

	currThread = (Thread *)cpuTrib.getCurrentCpuStream()->taskStream
		.getCurrentThread();

	// Only driver processes can call this.
	if (currThread->parent->getType() != ProcessStream::DRIVER)
		{ return; };

	// If the instantiation didn't work, delete the instance object.
	if (err != ERROR_SUCCESS)
	{
		if (floodplainn.getDevice(path, &dev) != ERROR_SUCCESS)
		{
			printf(ERROR FPLAINN"instantiateDevAck: getDevice "
				"failed on %s.\n\tUnable to unset "
				"device->instance pointer.\n",
				path);

			return;
		};

		dev->driverInstance->removeHostedDevice(path);
		// Set the dev->instance pointer to NULL.
		delete dev->instance;
		dev->instance = NULL;
	};

	response = new fplainn::Zudi::sKernelCallMsg(
		targetId,
		MSGSTREAM_SUBSYSTEM_ZUDI, MSGSTREAM_ZUDI___KCALL,
		sizeof(*response), 0, privateData);

	if (response == NULL) { return; };

	response->set(path, fplainn::Zudi::sKernelCallMsg::CMD_INSTANTIATE_DEVICE);
	response->header.error = err;

	MessageStream::enqueueOnThread(
		response->header.targetId, &response->header);
}

const sDriverInitEntry *fplainn::Zudi::findDriverInitInfo(utf8Char *shortName)
{
	const sDriverInitEntry	*tmp;

	for (tmp=driverInitInfo; tmp->udi_init_info != NULL; tmp++) {
		if (!strcmp8(tmp->shortName, shortName)) { return tmp; };
	};

	return NULL;
}

const sMetaInitEntry *fplainn::Zudi::findMetaInitInfo(utf8Char *shortName)
{
	const sMetaInitEntry		*tmp;

	for (tmp=metaInitInfo; tmp->udi_meta_info != NULL; tmp++) {
		if (!strcmp8(tmp->shortName, shortName)) { return tmp; };
	};

	return NULL;
}
