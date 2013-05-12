
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
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

void *dvfs::currenttC::getPath(
	utf8Char *fullName, vfs::inodeTypeE *type, error_t *ret
	)
{
	uarch_t			i=0;

	if (fullName == __KNULL || type == __KNULL)
	{
		if (ret != __KNULL) { *ret = ERROR_INVALID_ARG; };
		return __KNULL;
	};

	// Discard the root prefix.
	if (fullName[0] == '/') { i = 1; };

	if (fullName[0] == '@')
	{
		if ((fullName[1] != 'd' && fullName[1] != 'D')
			|| fullName[2] != '/')
		{
			if (ret != __KNULL)
				{ *ret = ERROR_INVALID_RESOURCE_NAME; };

			return __KNULL;
		};

		// Discard the "@d/" prefix.
		i = 3;
	};

	dvfs::categoryTagC	*currTag;
	dvfs::distributaryTagC	*dTag;
	status_t		nextSplitCharIndex;
	utf8Char		buff[DVFS_TAG_NAME_MAX_NCHARS];

	currTag = getRoot();
	for (;; i += nextSplitCharIndex+1)
	{
		if (fullName[i] == '\0')
		{
			if (currTag != getRoot())
			{
				if (ret != __KNULL) { *ret = ERROR_SUCCESS; };
				*type = vfs::DIR;
				return currTag;
			}

			if (ret != __KNULL)
				{ *ret = ERROR_INVALID_RESOURCE_NAME; };

			return __KNULL;
		};

		nextSplitCharIndex = vfs::getIndexOfNext(
			&fullName[i], '/', DVFS_TAG_NAME_MAX_NCHARS);

		if (nextSplitCharIndex == ERROR_INVALID_RESOURCE_NAME)
		{
			if (ret != __KNULL) { *ret = nextSplitCharIndex; };
			return __KNULL;
		};

		// Skip over things like "foo///bar" and "foo/././bar".
		if (nextSplitCharIndex == 0
			|| (nextSplitCharIndex == 1 && fullName[i] == '.'))
			{ continue; };

		// If there are no further slashes:
		if (nextSplitCharIndex == ERROR_NOT_FOUND)
		{
			strncpy8(buff, &fullName[i], DVFS_TAG_NAME_MAX_NCHARS);
			buff[DVFS_TAG_NAME_MAX_NCHARS - 1] = '\0';
		}
		else
		{
			strncpy8(buff, &fullName[i], nextSplitCharIndex);
			buff[nextSplitCharIndex] = '\0';

		};

		// Only check leaves if it's the last component(no more slashes)
		if (nextSplitCharIndex == ERROR_NOT_FOUND)
		{
			// If it was a leaf node, return it.
			dTag = currTag->getInode()->getLeafTag(buff);
			if (dTag != __KNULL)
			{
				if (ret != __KNULL) { *ret = ERROR_SUCCESS; }
				*type = vfs::LEAF;
				return dTag;
			};
		};

		currTag = currTag->getInode()->getDirTag(buff);
		if (currTag == __KNULL)
		{
			if (ret != __KNULL)
				{ *ret = ERROR_NOT_FOUND; };

			return __KNULL;
		};

		// If this was the last path component, return the category tag.
		if (nextSplitCharIndex == ERROR_NOT_FOUND)
		{
			if (ret != __KNULL) { *ret = ERROR_SUCCESS; };
			*type = vfs::DIR;
			return currTag;
		};
	};
}

