#ifndef _DRIVER_H
	#define _DRIVER_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/driverTrib/udi/meta.h>
	#include <kernel/common/driverTrib/udi/cb.h>
	#include <kernel/common/fsTrib/vfsTypes.h>

/**	EXPLANATION:
 * The kernel loads basic information on a driver itself, and keeps that in its
 * address space to facilitate fast instantiation.
 *
 * A driverC object tells the kernel what metalanguage libs must be loaded
 * into the address space of every instance of this driver (and where in the
 * driver instance address space it must be loaded as well.). It also holds
 * information on control block groups for instances of the driver (to the
 * kernel, a control block group is nothing more than a new object cache).
 *
 * When the kernel instantiates a driver, it uses this loaded information to
 * quickly "know" how to relocate metalanguage libs, etc.
 *
 * In particular, when loading information on a to-be-instantiated driver, the
 * kernel will quickly try to determine the best way to position the driver's
 * surrounding metalanguage libraries. To do this, first we determine the size
 * of the driver image (all Zambezii UDI drivers are relocated statically to a
 * fixed base address when they are "installed", such that they don't begin at
 * base 0x0).
 *
 * Next, each metalanguage required is examined in turn, and a base virtual
 * address is determined for it such that it will not collide with any other
 * item in the driver's address space. The determined best load address is
 * stored in the driverC object's metaRequirements array index for that required
 * metalanguage library.
 *
 * Then when the driver is instantiated, the kernel just takes all of this pre-
 * determined information and copies each file into its rightful place in the
 * driver instance's address space.
 *
 * Along with information on how to quickly relocate a driver file/meta lib,
 * is information on where its entry points are within the driver. Zambezii
 * would prefer to require that drivers are all statically relocated to a
 * specific base address so that entry points remain as described in the ops
 * vectors.
 *
 * The 'regions' array is simply more information on how the load the driver:
 * What must be allocated, etc.
 **/
// Fast cache of loading information to enable fast driver instantiation.
class driverC
{
public:
	ubit32		refCount;
	utf8Char	*driverName;
	utf8Char	*vendor;

	/* Information needed to determine whether this driver should be used
	 * for a device. Generally used in the case that the driver has been
	 * loaded for another device, and the kernel is searching all loaded16
	 * drivers to see if it can use a currently loaded one instead of
	 * searching the disk.
	 **/
	utf8Char	*category;
	utf8Char	*devName;

	// Basic information needed to load and instantiate the driver.
	utf8Char	*fileName;
	vfsFileS	*file;

	ubit8		isFullyEnumerated;

	// A driver must have a map of all required metalibs.
	udiMetaDescS	**metaRequirements;
	udiCbDescS	*cbRequirements;

	udiRegionDescS		*regions;
	// Index of within the above array for the primary region.
	ubit16			primaryRegionIndex;
	udiOpsVectorDescS	*opsVectors;
};

/**	EXPLANATION:
 * An instance describes a particular driver process. An instance will tell of
 * how this particular driver has been loaded, and it points to any necessary
 * information for its particular device instance.
 *
 * For example, a filesystem driver instance would need to point to a
 * storage driver instance.
 *
 * Instance information is mainly per-device info, to which the device tree
 * points. You could say that the driver instance is pretty much like the device
 * tree node for a device. At the time of writing, I haven't yet decided whether
 * or not I should create fooDevC classes for device instances, or just use the
 * fooDrvInstC objects for the device tree.
 *
 * To an extent, it would make sense to have a separate set of fooDevC classes
 * for the reason that an enumerated device may not necessarily have a driver
 * instance bound to it. A device could have been discovered, and then ignored
 * for driver instantiation, due to a better device being found to serve its
 * purpose, etc. In such a case, when displaying the device tree in the
 * "Device Manager", it would be useful not to make driver instances implicitly
 * be the device tree.
 *
 * A UDI channel is not a property of a driver, but of one of its instances, so
 * the driverInstanceC base class should have an array of channel descriptors.
 **/
class driverInstanceC
{
public:
	processId_t	processId;
	processStreamC	*process;

	
};

#endif


#endif
