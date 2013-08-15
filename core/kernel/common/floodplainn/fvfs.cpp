
#include <kernel/common/floodplainn/fvfs.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


error_t fvfs::currenttC::getPath(utf8Char *path, fvfs::tagC **ret)
{
	/**	EXPLANATION:
	 * Returns the deviceC object for a particular device path. The path
	 * may be prefixed with "/", like a unix absolute path, may carry no
	 * prefix at all, or may be prefixed with "@f/".
	 *
	 * '.' and '..' do not have special meaning in device paths, and do not
	 * refer to "current node" or "parent node". A path containing a '.' or
	 * '..' segment will be treated as if those segments must be matched
	 * against an FVFS node.
	 *
	 * Consecutive slashes (e.g.: "///0///1/2//3") are skipped.
	 **/
	fvfs::tagC		*currTag;
	uarch_t			i=0;
	uarch_t			segmentLen;

	if (path == __KNULL || ret == __KNULL) { return ERROR_INVALID_ARG; };

	*ret = __KNULL;

	for (; *path == '/'; path++) {};
	if (strncmp8(path, CC"@f/", 3) == 0) { i = 3; };
	// zero-length string results in NOT_FOUND.
	if (path[i] == '\0') { return ERROR_NOT_FOUND; };

	currTag = vfsTrib.getFvfs()->getRoot();
	for (;; i += segmentLen + 1)
	{
		utf8Char	tmpBuff[FVFS_TAG_NAME_MAXLEN+1];
		utf8Char	*nextSlash;

		/* If the path ends in "/", such as "@f/foo/bar/", and a node
		 * was found up to "bar" such that the loop went into the next
		 * iteration after "bar" and ended up starting this iteration
		 * directly at the "NULL" char following the trailing slash,
		 * return the node that was found for "bar".
		 *
		 * TL;DR: Allow trailing slashes in device paths.
		 **/
		if (path[i] == '\0' && currTag != vfsTrib.getFvfs()->getRoot())
		{
			*ret = currTag;
			return ERROR_SUCCESS;
		};

		// Find next slash, if any.
		nextSlash = strnchr8(&path[i], FVFS_TAG_NAME_MAXLEN, '/');
		if (nextSlash != __KNULL)
		{
			// If there was a slash, copy until the slash.
			segmentLen = nextSlash - &path[i];
			// Skip contiguous slashes.
			if (segmentLen == 0) { continue; };

			strncpy8(tmpBuff, &path[i], segmentLen);
			tmpBuff[segmentLen] = '\0';
		}
		else
		{
			// If no slash, check strlen, then copy until EOString.
			segmentLen = strnlen8(&path[i], FVFS_TAG_NAME_MAXLEN);
			if (segmentLen == FVFS_TAG_NAME_MAXLEN) {
				return ERROR_INVALID_RESOURCE_NAME;
			};

			// Can strcpy() safely because we know it's fine.
			strcpy8(tmpBuff, &path[i]);
		};

		// tmpBuff now holds the next segment.
		currTag = currTag->getChild(tmpBuff);
		if (currTag == __KNULL) { return ERROR_NOT_FOUND; };

		// If there were no more slashes, this is the last segment.
		if (nextSlash == __KNULL)
		{
			*ret = currTag;
			return ERROR_SUCCESS;
		};
	};

	return ERROR_NOT_FOUND;	
}

