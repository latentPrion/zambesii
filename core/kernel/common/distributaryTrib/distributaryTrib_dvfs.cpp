
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

static inline dvfs::tagC *getDefaultDtrib(dvfs::tagC *cTag)
{
	return cTag->getCInode()->getLeafTag(CC"default");
}

error_t dvfs::currenttC::getPath(
	utf8Char *fullName, tagC **tag
	)
{
	uarch_t			i=0;
	tagC			*currTag, *dTag;
	uarch_t			segmentLen;

	if (fullName == __KNULL || tag == __KNULL)
		{ return ERROR_INVALID_ARG; };

	// Discard preceding slashes and VFS prefix.
	for (; *fullName == '/'; fullName++) {};
	if (strncmp8(fullName, CC"@d/", 3) == 0) { i += 3; };

	currTag = getRoot();
	for (;; i += segmentLen + 1)
	{
		utf8Char		*nextSlash;
		utf8Char		buff[DVFS_TAG_NAME_MAXLEN];

		if (fullName[i] == '\0')
		{
			if (currTag != getRoot())
			{
				// Handle trailiing slashes.
				if (currTag->getType() == vfs::DIR)
				{
					currTag = getDefaultDtrib(currTag);
					if (currTag == __KNULL) {
						return ERROR_INVALID_ARG_VAL;
					};
				};

				*tag = currTag;
				return ERROR_SUCCESS;
			}

			return ERROR_NOT_FOUND;
		};

		nextSlash = strnchr8(&fullName[i], DVFS_TAG_NAME_MAXLEN, '/');
		if (nextSlash == __KNULL)
		{
			// If no further slashes:
			segmentLen = strnlen8(
				&fullName[i], DVFS_TAG_NAME_MAXLEN);
		}
		else { segmentLen = nextSlash - &fullName[i]; };

		if (segmentLen == DVFS_TAG_NAME_MAXLEN)
			{ return ERROR_INVALID_RESOURCE_NAME; };

		// Skip over things like "foo///bar" and "foo/././bar".
		if (segmentLen == 0 || (segmentLen == 1 && fullName[i] == '.'))
			{ continue; };

		// Copy the segment into the buffer.
		strncpy8(buff, &fullName[i], segmentLen);
		buff[segmentLen] = '\0';

		// Only check leaves if it's the last component(no more slashes)
		if (nextSlash == __KNULL)
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

		/* If this was the last path component, try to get the "default"
		 * distributary for this category. If none exists, return
		 * INVALID_ARG_VAL.
		 **/
		if (nextSlash == __KNULL)
		{
			dTag = getDefaultDtrib(currTag);
			if (dTag == __KNULL) { return ERROR_INVALID_ARG_VAL; };

			*tag = dTag;
			return ERROR_SUCCESS;
		};
	};
}

