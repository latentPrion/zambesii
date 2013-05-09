
#include <__kstdlib/__kclib/assert.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/distributaryTrib/dvfs.h>


error_t dvfs::dvfsCurrenttC::initialize(void)
{
	// In case we make changes and forget to check this.
	assert_fatal(
		sizeof(dvfs::distributaryTagC) == sizeof(dvfs::categoryTagC));

	tagCache = cachePool.createCache(sizeof(dvfs::distributaryTagC));
	if (tagCache == __KNULL)
	{
		__kprintf(ERROR DTRIBTRIB"Failed to create tag cache.\n");
		return ERROR_MEMORY_NOMEM;
	};

	return vfs::currenttC::initialize();
}

dvfs::distributaryInodeC::distributaryInodeC(
	typeE type, distributaryDescriptorS *descriptor
	)
:
distributaryDescriptorS(*descriptor), type(type), currentlyRunning(0)
{
	strncpy8(
		name, descriptor->name,
		DVFS_TAG_NAME_MAX_NCHARS);

	strncpy8(
		category, descriptor->category,
		DVFS_TAG_NAME_MAX_NCHARS);

	strncpy8(description, descriptor->description, 256);
	strncpy8(vendor, descriptor->vendor, 64);
}

