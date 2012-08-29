
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/timerTrib/timeTypes.h>


/**	DISCOURSE:
 * Dir inodes link the directory tree up, but file inodes just contain metadata.
 * They also contain a link to the correct driver instance for the file's
 * FS.
 *
 * Thoughts on open and access modes for files: Caching of file data is done
 * at the disk driver level, and not at the FS level.
 *
 * Two processes have a file, F open, process A for reading and process B for
 * writing. A reads region foo from the file, and B writes to that region after.
 * When B writes, the disk caching for that region must be invalidated, and
 * on the next read, a new set of caching will be established.
 *
 * For files, there are only read caches: the kernel does not do write-back
 * caching on files. All file writes are write-through from the kernel's POV.
 *
 * In that sense, region caching is handled in the FS driver, and not by the
 * kernel. The filesystem alone would know what the mappings between sectors and
 * logical offsets in a file are. So the kernel doesn't actually cache anything
 * on its own for the VFS...? What about disk sector caches? Should they be
 * handled by the kernel, or by disk drivers? UDI's model is more likely to
 * favour handing that job over to the kernel.
 *
 * Next, an FS reads from the disk layer. It gets a cached block from the
 * kernel. The FS takes that and continues asking for sector ranges it needs
 * until it can return the "seek and read" range the kernel asked for. Then
 * it stops and returns to the kernel.
 *
 * 
 **/

vfsFileInodeC::vfsFileInodeC(
	ubit32 _inodeHigh, ubit32 _inodeLow, uarch_t _fileSize
	)
:
inodeLow(_inodeLow), inodeHigh(_inodeHigh)
{
	refCount = 0;
	fileSize = _fileSize;

	createdDate = 0;
	modifiedDate = 0;
	accessedDate = 0;

	memset(&createdTime, 0, sizeof(createdTime));
	memset(&modifiedTime, 0, sizeof(modifiedTime));
	memset(&modifiedTime, 0, sizeof(accessedTime));
}

error_t vfsFileInodeC::initialize(void)
{
	return ERROR_SUCCESS;
}

vfsFileInodeC::~vfsFileInodeC(void)
{
}

