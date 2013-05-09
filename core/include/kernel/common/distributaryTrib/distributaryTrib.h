#ifndef _DISTRIBUTARY_TRIB_H
	#define _DISTRIBUTARY_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/currentt.h>
	#include <__kclasses/ptrList.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/distributaryTrib/dvfs.h>

	/**	EXPLANATION:
	 * The Distributary Tributary is the tributary responsible for managing
	 * the loading of kernel Distributaries. "Distributary" is the
	 * nomenclature used for a runtime loadable separate address space
	 * kernel component; many other kernels would call this a "service".
	 *
	 * A namespace of descriptor objects for each available distributary is
	 * constructed within the VFS Tributary. At some point in the future,
	 * dynamic loading of distributaries from disk, or other sources
	 * external to the kernel's memory image may be supported, but for now,
	 * such a feature is quite far from my thoughts.
	 **/

#define DTRIBTRIB		"Dtrib Trib: "

#define DTRIBTRIB_TAG_NAME_MAX_NCHARS		(48)

class slamCacheC;

class distributaryTribC
:
public tributaryC
{
public:
	distributaryTribC(void)
	:
	dtribTagCache(__KNULL)
	{}

	// Builds the Dtrib VFS tree of compiled-in kernel distributaries.
	error_t initialize(void) { return ERROR_SUCCESS; }
	~distributaryTribC(void) {};

public:

public:

private:
	// Called at boot to construct the distributary VFS tree.
	void buildTree(void);

private:
	slamCacheC			*dtribTagCache;
	static const dvfs::distributaryDescriptorS
		*const distributaryDescriptors[];
};

extern distributaryTribC	distributaryTrib;

#endif

