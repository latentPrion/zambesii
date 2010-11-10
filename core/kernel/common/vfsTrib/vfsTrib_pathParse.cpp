
#include <kernel/common/vfsTrib/vfsTrib.h>


#define PATH_TYPE_UNKNOWN		0x0
#define PATH_TYPE_TREE			0x1
#define PATH_TYPE_UNIX_ROOT		0x2
#define PATH_TYPE_DRIVE_ALIAS		0x3
#define PATH_TYPE_RELATIVE		0x4

static vfsDirDescC *parsePath(utf16Char *path)
{
	ubit8		pathType=PATH_TYPE_UNKNOWN;

	/**	EXPLANATION:
	 * Idea is to take a path (either utf16Char[] or utf8Char[]) and
	 * progressively split it up into chunks: find each path separator ("/")
	 * and replace it with NULL.
	 *
	 * For each located segment, scan for invalid sequences. Then pass the
	 * located segment to a dirDesc parser.
	 *
	 * So: take arg 'path', and parse for the indicator of the root:
	 * whether absolute or relative.
	 **/
	if (path[0] == static_cast<utf16Char>( ':' )) {
		pathType = PATH_TYPE_TREE;
	};
	if (path[1] == static_cast<utf16Char>( ':' ) && !isTree) {
		pathType = PATH_TYPE_DRIVE_ALIAS;
	};
	if (path[0] == static_cast<utf16Char>( '/' ) && !isDriveAlias) {
		pathType = PATH_TYPE_UNIX_ROOT;
	};

	/**	EXPLANATION:
	 * Now we have to decide what preprocessing to do before starting to
	 * split and traverse. If the path is a tree, we need to pass it on.
	 *
	 * If it's a unix root, we need to grab the default tree. If it's an
	 * alias, we need to grab the path for the alias and move on.
