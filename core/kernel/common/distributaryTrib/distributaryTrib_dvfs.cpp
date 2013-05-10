
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/distributaryTrib/dvfs.h>


dvfs::distributaryInodeC::distributaryInodeC(
	const distributaryDescriptorS *descriptor
	)
:
type(IN_KERNEL), currentlyRunning(0),
majorVersion(descriptor->majorVersion), minorVersion(descriptor->minorVersion),
patchVersion(descriptor->patchVersion),
entryAddress(descriptor->entryAddress), flags(descriptor->flags)
{
	strncpy8(
		name, descriptor->name,
		DVFS_TAG_NAME_MAX_NCHARS);

	strncpy8(
		vendor, descriptor->vendor,
		DVFS_DINODE_VENDOR_MAX_NCHARS);

	strncpy8(
		description, descriptor->description,
		DVFS_DINODE_DESCRIPTION_MAX_NCHARS);
}

