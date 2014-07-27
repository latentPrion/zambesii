
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/distributaryTrib/distributaryTrib.h>
#include <kernel/common/distributaryTrib/dvfs.h>


dvfs::distributaryINode::distributaryINode(
	const sDistributaryDescriptor *descriptor, typeE type
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
}

static inline dvfs::Tag *getDefaultDtrib(dvfs::Tag *cTag)
{
	return cTag->getCInode()->getLeafTag(CC"default");
}

error_t dvfs::Currentt::getPath(
	utf8Char *fullName, Tag **tag
	)
{
	uarch_t			i=0;
	Tag			*currTag, *dTag;
	uarch_t			segmentLen;

	if (fullName == NULL || tag == NULL)
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
					if (currTag == NULL) {
						return ERROR_INVALID_ARG_VAL;
					};
				};

				*tag = currTag;
				return ERROR_SUCCESS;
			}

			return ERROR_NOT_FOUND;
		};

		nextSlash = strnchr8(&fullName[i], DVFS_TAG_NAME_MAXLEN, '/');
		if (nextSlash == NULL)
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
		if (nextSlash == NULL)
		{
			// If it was a leaf node, return it.
			dTag = currTag->getCInode()->getLeafTag(buff);
			if (dTag != NULL)
			{
				*tag = dTag;
				return ERROR_SUCCESS;
			};
		};

		currTag = currTag->getCInode()->getDirTag(buff);
		if (currTag == NULL) {
			return ERROR_NOT_FOUND;
		};

		/* If this was the last path component, try to get the "default"
		 * distributary for this category. If none exists, return
		 * INVALID_ARG_VAL.
		 **/
		if (nextSlash == NULL)
		{
			dTag = getDefaultDtrib(currTag);
			if (dTag == NULL) { return ERROR_INVALID_ARG_VAL; };

			*tag = dTag;
			return ERROR_SUCCESS;
		};
	};
}

