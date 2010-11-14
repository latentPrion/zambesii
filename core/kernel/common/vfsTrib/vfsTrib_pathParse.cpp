
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/vfsTrib/vfsTraverse.h>
#include <kernel/common/cpuTrib/cpuTrib.h>


status_t vfsTribC::getPath(utf16Char *path, ubit8 *type, void **ret)
{
	vfsDirC		*dir=0;
	sbit32		idx=0;

	/**	EXPLANATION:
	 * The idea is to find out what kind of path we're dealing with, then
	 * do any preprocessing and pass it on to the recursive relative path
	 * parser.
	 *
	 * For a tree-absolute path, we get the tree, then treat the rest of
	 * the path as if it was relative
	 **/
	if (vfsTraverse::isTree(path))
	{
		__kprintf(NOTICE"Detected tree path.\n");
		idx = vfsTraverse::getNextSegmentIndex(path);
		if (idx < 0)
		{
			*ret = vfsTrib.getTree(path);
			*type = VFSPATH_TYPE_DIR;
			if (*ret != __KNULL) {
				return ERROR_SUCCESS;
			};
		}

		// Temporarily replace the '/' with a '\0'.
		path[idx - 1] = '\0';
		dir = vfsTrib.getTree(path);
		path[idx - 1] = '/';
		if (dir == __KNULL) {
			return VFSPATH_INVALID;
		};
		goto parseRelative;
	};

	if (vfsTraverse::isUnixRoot(path))
	{
		// Get default tree, then do vfsTraverse::getNextSegmentIndex().
		__kprintf(NOTICE"Detected unix root path.\n");
		dir = getDefaultTree();
		if (dir == __KNULL) {
			return VFSPATH_INVALID;
		};

		idx = vfsTraverse::getNextSegmentIndex(path);
		if (idx < 0)
		{
			*type = VFSPATH_TYPE_DIR;
			*ret = dir;
			return ERROR_SUCCESS;
		};
		goto parseRelative;
	};

#if 0
	// Add support for Path Aliases later.
	if (vfsTraverse::isPathAlias(path)) {
		// Get path alias via recurse into getPath, then move on.
	};
#endif

#if 0
	// Add support for working-directory-relative paths later.
	// Fell through to here. Path is relative to process's current dir.
	if (getPath(
		cpuTrib.getCurrentCpuStream()
			->getCurrentTask()->parent->workingDir,
		type,
		&dir) != ERROR_SUCCESS || *type == VFSPATH_TYPE_FILE)
	{
		return VFSPATH_INVALID;
	};
#endif
parseRelative:
	// Now pass to the recursive relative path parser.
	return vfsTraverse::getRelativePath(dir, &path[idx], type, ret);
}

