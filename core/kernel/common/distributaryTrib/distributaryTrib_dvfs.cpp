
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/distributaryTrib/dvfs.h>


dvfs::distributaryInodeC::distributaryInodeC(
	const distributaryDescriptorS *descriptor, typeE type
	)
:
type(type), currentlyRunning(0),
majorVersion(descriptor->majorVersion), minorVersion(descriptor->minorVersion),
patchVersion(descriptor->patchVersion),
flags(descriptor->flags), entryAddress(descriptor->entryAddress)
{
	strncpy8(
		name, descriptor->name,
		DVFS_TAG_NAME_MAXLEN);

	strncpy8(
		vendor, descriptor->vendor,
		DVFS_DINODE_VENDOR_MAXLEN);

	strncpy8(
		description, descriptor->description,
		DVFS_DINODE_DESCRIPTION_MAXLEN);

	fullName[0] = '\0';
}

error_t dvfs::currenttC::getPath(
	utf8Char *fullName, tagC **tag
	)
{
	uarch_t			i=0;

	if (fullName == __KNULL || tag == __KNULL)
		{ return ERROR_INVALID_ARG; };

	// Discard the root prefix.
	if (fullName[0] == '/') { i = 1; };

	if (fullName[0] == '@')
	{
		if ((fullName[1] != 'd' && fullName[1] != 'D')
			|| fullName[2] != '/')
		{
			return ERROR_INVALID_RESOURCE_NAME;
		};

		// Discard the "@d/" prefix.
		i = 3;
	};

	tagC			*currTag, *dTag;
	status_t		nextSplitCharIndex;
	utf8Char		buff[DVFS_TAG_NAME_MAXLEN];

	currTag = getRoot();
	for (;; i += nextSplitCharIndex+1)
	{
		if (fullName[i] == '\0')
		{
			if (currTag != getRoot())
			{
				*tag = currTag;
				return ERROR_SUCCESS;
			}

			return ERROR_INVALID_RESOURCE_NAME;
		};

		nextSplitCharIndex = vfs::getIndexOfNext(
			&fullName[i], '/', DVFS_TAG_NAME_MAXLEN);

		if (nextSplitCharIndex == ERROR_INVALID_RESOURCE_NAME) {
			return nextSplitCharIndex;
		};

		// Skip over things like "foo///bar" and "foo/././bar".
		if (nextSplitCharIndex == 0
			|| (nextSplitCharIndex == 1 && fullName[i] == '.'))
			{ continue; };

		// If there are no further slashes:
		if (nextSplitCharIndex == ERROR_NOT_FOUND)
		{
			strncpy8(buff, &fullName[i], DVFS_TAG_NAME_MAXLEN);
			buff[DVFS_TAG_NAME_MAXLEN - 1] = '\0';
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
			dTag = currTag->getCInode()->getLeafTag(buff);
			if (dTag != __KNULL)
			{
				*tag = dTag;
				return ERROR_SUCCESS;
			};
		};

		currTag = currTag->getCInode()->getDirTag(buff);
		if (currTag == __KNULL) {
			return ERROR_NOT_FOUND;
		};

		// If this was the last path component, return the category tag.
		if (nextSplitCharIndex == ERROR_NOT_FOUND)
		{
			*tag = currTag;
			return ERROR_SUCCESS;
		};
	};
}

