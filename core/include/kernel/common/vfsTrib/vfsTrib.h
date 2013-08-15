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

class distributaryTribC;

class vfsTribC
:
public tributaryC
{
friend class distributaryTribC;
public:
	vfsTribC(void) {}
	error_t initialize(void)
	{
		error_t		ret;

		ret = dvfsCurrentt.initialize();
		if (ret != ERROR_SUCCESS) { return ret; };
		return hvfsCurrentt.initialize();
	}

	~vfsTribC(void) {}

public:
	dvfs::currenttC *getDvfs(void) { return &dvfsCurrentt; }
	hvfs::currenttC *getHvfs(void) { return &hvfsCurrentt; }
	fvfs::currenttC *getFvfs(void) { return &fvfsCurrentt; }

	void dumpCurrentts(void);

private:
	hvfs::currenttC		hvfsCurrentt;
	dvfs::currenttC		dvfsCurrentt;
	fvfs::currenttC		fvfsCurrentt;
};

extern vfsTribC		vfsTrib;

#endif

