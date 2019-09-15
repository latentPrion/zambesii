#ifndef _VIRTUAL_FILESYSTEM_TRIBUTARY_H
	#define _VIRTUAL_FILESYSTEM_TRIBUTARY_H

	#include <arch/arch.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/cachePool.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/vfsTrib/vfsTypes.h>
	#include <kernel/common/vfsTrib/hierarchicalStorage.h>
	#include <kernel/common/distributaryTrib/dvfs.h>
	#include <kernel/common/floodplainn/fvfs.h>

#define VFSTRIB			"VFS Trib: "

#define VFSPATH_TYPE_DIR		0x1
#define VFSPATH_TYPE_FILE		0x2

#define VFSPATH_INVALID			15
#define VFSPATH_INVALID_CHAR		16

class DistributaryTrib;

class VfsTrib
:
public Tributary
{
friend class DistributaryTrib;
public:
	VfsTrib(void) {}
	error_t initialize(void)
	{
		error_t		ret;

		ret = dvfsCurrentt.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		ret = hvfsCurrentt.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };

		zkcmCore.chipsetEventNotification(
			__KPOWER_EVENT_VFS_TRIB_AVAIL, 0);

		return ERROR_SUCCESS;
	}

	~VfsTrib(void) {}

public:
	dvfs::Currentt *getDvfs(void) { return &dvfsCurrentt; }
	hvfs::Currentt *getHvfs(void) { return &hvfsCurrentt; }
	fvfs::Currentt *getFvfs(void) { return &fvfsCurrentt; }

	void dumpCurrentts(void);

private:
	hvfs::Currentt		hvfsCurrentt;
	dvfs::Currentt		dvfsCurrentt;
	fvfs::Currentt		fvfsCurrentt;
};

extern VfsTrib		vfsTrib;

#endif

