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

#define VFSTRIB			"VFS Trib: "

#define VFSPATH_TYPE_DIR		0x1
#define VFSPATH_TYPE_FILE		0x2

#define VFSPATH_INVALID			15
#define VFSPATH_INVALID_CHAR		16

class vfsTribC
:
public tributaryC
{
friend class vfsDirInodeC;
public:
	vfsTribC(void) {}
	error_t initialize(void) { return hvfsCurrentt.initialize(); }
	~vfsTribC(void) {}

	void dumpCurrentts(void);

private:
	hvfs::currenttC		hvfsCurrentt;
	dvfs::currenttC		dvfsCurrentt;
};

extern vfsTribC		vfsTrib;

#endif

