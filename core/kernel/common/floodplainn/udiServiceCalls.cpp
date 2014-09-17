
#define UDI_VERSION	0x101
#include <udi.h>
#include <stdarg.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>


void __udi_assert(const char *expr, const char *file, int line)
{
	printf(FATAL"udi_assert failure: line %d of %s; expr '%s'.\n",
		line, file, expr);

	assert_fatal(0);
}

error_t fplainn::Zudi::getInternalBopVectorIndexes(
	ubit16 regionIndex, ubit16 *opsIndex0, ubit16 *opsIndex1
	)
{
	Thread				*self;
	fplainn::DriverInstance		*drvInst;
	fplainn::Driver::sInternalBop	*iBop;

	self = (Thread *)cpuTrib.getCurrentCpuStream()
		->taskStream.getCurrentThread();

	drvInst = self->parent->getDriverInstance();

	iBop = drvInst->driver->getInternalBop(regionIndex);
	if (iBop == NULL) { return ERROR_NOT_FOUND; };

	*opsIndex0 = iBop->opsIndex0;
	*opsIndex1 = iBop->opsIndex1;
	return ERROR_SUCCESS;
}

error_t fplainn::Zudi::spawnInternalBindChannel(
	utf8Char *devPath, ubit16 regionIndex,
	udi_ops_vector_t *opsVector0, udi_ops_vector_t *opsVector1)
{
	fplainn::DeviceInstance::sRegion	*region0, *region1;
	udi_init_context_t			*rdata0=NULL, *rdata1=NULL;
	processId_t				region0Tid=PROCID_INVALID,
						dummy;
	fplainn::Device				*dev;
	error_t					ret;

printf(NOTICE"spawnIBopChan(%s, %d, 0x%p, 0x%p).\n",
	devPath, regionIndex, opsVector0, opsVector1);
	/* region1 is always the caller because internal bind channels are
	 * spawned by the secondary region thread.
	 **/
	region1 = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread()
		->getRegion();

	ret = floodplainn.getDevice(devPath, &dev);
	if (ret != ERROR_SUCCESS) { return ret; };

	if (dev->instance->getRegionInfo(regionIndex, &dummy, &rdata1)
		!= ERROR_SUCCESS)
		{ return ERROR_INVALID_ARG_VAL; };

	// If we can't get primary region info, something is terribly wrong.
	assert_fatal(
		dev->instance->getRegionInfo(0, &region0Tid, &rdata0)
		== ERROR_SUCCESS);

	region0 = region1->thread->parent->getThread(region0Tid)->getRegion();

	return spawnChannel(
		dev, region0, opsVector0, rdata0,
		dev, region1, opsVector1, rdata1);
}

error_t fplainn::Zudi::spawnChildBindChannel(
	utf8Char *parentDevPath, utf8Char *childDevPath, utf8Char *metaName,
	udi_ops_vector_t *opsVector
	)
{
	fplainn::Device				*parentDev, *childDev;
	fplainn::Driver::sMetalanguage		*parentMeta, *childMeta;
	fplainn::Driver::sChildBop		*parentCBop;
	fplainn::Driver::sParentBop		*childPBop;
	fplainn::DeviceInstance::sRegion	*parentRegion, *childRegion;
	processId_t				parentTid, childTid;
	udi_init_context_t			*parentRdata, *childRdata;
	fplainn::DriverInstance::sChildBop	*parentInstCBop;

	if (floodplainn.getDevice(parentDevPath, &parentDev) != ERROR_SUCCESS
		|| floodplainn.getDevice(childDevPath, &childDev) != ERROR_SUCCESS)
		{ return ERROR_INVALID_RESOURCE_NAME; };

	/* Both the parent device and the connecting child device must expose
	 * "meta" indexes for the specified metalanguage.
	 **/
	childMeta = childDev->driverInstance->driver->getMetalanguage(metaName);
	parentMeta = parentDev->driverInstance->driver->getMetalanguage(
		metaName);

	if (childMeta == NULL || parentMeta == NULL)
	{
		printf(WARNING FPLAINN"spawnCBindChan: meta %s not declared "
			"by one or both devices.\n",
			metaName);

		return ERROR_NON_CONFORMANT;
	};

	/* Furthermore, the parent must have a child bop for the meta, while the
	 * child must have a parent bop for the meta. If any of these conditions
	 * are not met, the connection cannot be established.
	 **/
	parentCBop = parentDev->driverInstance->driver->getChildBop(
		parentMeta->index);

	childPBop = childDev->driverInstance->driver->getParentBop(
		childMeta->index);

	if (parentCBop == NULL || childPBop == NULL)
	{
		printf(WARNING FPLAINN"spawnCBindChan: meta %s not exposed "
			"as parent/child bop by one or both devices.\n",
			metaName);

		return ERROR_UNSUPPORTED;
	};

	// Next, get the region thread IDs.
	if (parentDev->instance->getRegionInfo(
		parentCBop->regionIndex, &parentTid, &parentRdata)
		!= ERROR_SUCCESS
		|| childDev->instance->getRegionInfo(
			childPBop->regionIndex, &childTid, &childRdata)
			!= ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Either parent or child's "
			"Bop region ID is invalid.\n");

		return ERROR_INVALID_RESOURCE_ID;
	};

	// Now get the thread objects using their thread IDs.
	parentRegion = processTrib.getThread(parentTid)->getRegion();
	childRegion = processTrib.getThread(childTid)->getRegion();
	if (parentRegion == NULL || childRegion == NULL)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Either parent or child's "
			"region TID doesn't map to valid thread.\n");

		return ERROR_INVALID_RESOURCE_ID;
	};

	// Finally, we need the parent's ops vector and context info.
	parentInstCBop = parentDev->driverInstance->getChildBop(
		parentMeta->index);

	if (parentInstCBop == NULL)
	{
		printf(ERROR FPLAINN"spawnCBindChan: Parent driver instance "
			"hasn't set its child bop vectors.\n");

		return ERROR_UNINITIALIZED;
	};

	// Finally, call spawnChannel.
	return spawnChannel(
		parentDev, parentRegion, parentInstCBop->opsVector, parentRdata,
		childDev, childRegion, opsVector, childRdata);
}

error_t fplainn::Zudi::spawnChannel(
	fplainn::Device *dev0, fplainn::DeviceInstance::sRegion *region0,
	udi_ops_vector_t *opsVector0, udi_init_context_t *channelContext0,
	fplainn::Device *dev1, fplainn::DeviceInstance::sRegion *region1,
	udi_ops_vector_t *opsVector1, udi_init_context_t *channelContext1
	)
{
	HeapObj<fplainn::DeviceInstance::sChannel>	chan;
	error_t						ret0, ret1;

	/**	EXPLANATION:
	 * Back-end function that actually allocates the channel object and
	 * adds it to both device-instances' channel lists.
	 *
	 * The region thread, ops-vector and channel-context need not be
	 * set at this point, but they will be required by the kernel before
	 * the channel can be used.
	 **/
	if (dev0 == NULL || dev1 == NULL) { return ERROR_INVALID_ARG; };

	chan = new fplainn::DeviceInstance::sChannel(
		region0, opsVector0, channelContext0,
		region1, opsVector1, channelContext1);

	if (chan == NULL) { return ERROR_MEMORY_NOMEM; };

	ret0 = dev0->instance->addChannel(chan.get());
	if (dev1 != dev0) { ret1 = dev1->instance->addChannel(chan.get()); }
	else { ret1 = ret0; };

	if (ret0 != ERROR_SUCCESS || ret1 != ERROR_SUCCESS)
	{
		printf(ERROR FPLAINN"spawnChannel: Failed to add chan to "
			"one or both deviceInstance objects.\n");

		return ERROR_UNKNOWN;
	};

	chan.release();
	return ERROR_SUCCESS;
}

void udi_mei_call(
	udi_cb_t *,
	udi_mei_init_t *,
	udi_index_t ,
	udi_index_t vec_idx,
	...
	)
{
	va_list		args;

	va_start(args, vec_idx);
	va_end(args);
}
