#ifndef _FLOODPLAINN_DEVICE_TREE_H
	#define _FLOODPLAINN_DEVICE_TREE_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <zudiIndex.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kstdlib/__kclib/string.h>
	#include <kernel/common/numaTypes.h>

/**	deviceC:
 * Base type for a device in general. The type of driver used to instantiate
 * the device is independent of this class; both the driver instance and the
 * driver are simply pointed to by the device.
 *
 * Each device has a name and a unique ID.
 **/

#define DEVICE_SHORTNAME_MAXLEN			DRIVER_SHORTNAME_MAXLEN
#define DEVICE_LONGNAME_MAXLEN			DRIVER_LONGNAME_MAXLEN
#define DEVICE_VENDORNAME_MAXLEN		DRIVER_VENDORNAME_MAXLEN
#define DEVICE_VENDORCONTACT_MAXLEN		DRIVER_VENDORCONTACT_MAXLEN
#define DEVICE_DRIVERNAME_MAXLEN		(128)

#define DEVICE_CLASS_MAXLEN			(48)

#define DRIVER_SHORTNAME_MAXLEN			(16)
#define DRIVER_LONGNAME_MAXLEN			(48)
#define DRIVER_VENDORNAME_MAXLEN		(24)
#define DRIVER_VENDORCONTACT_MAXLEN		(96)

namespace fplainn
{
	/**	deviceC:
	 * Data type used to represent a single device in the kernel's device
	 * tree.
	 **********************************************************************/
	class driverC;
	class driverInstanceC;

	class deviceC
	{
	public:
		deviceC(ubit16 id, utf8Char *shortName)
		:
		id(id), driver(NULL), driverInstance(NULL),
		nEnumerationAttribs(0), nInstanceAttribs(0),
		enumeration(NULL), instance(NULL)
		{
			if (shortName != NULL)
			{
				strncpy8(
					this->shortName, shortName,
					DEVICE_SHORTNAME_MAXLEN);
			};
		}

		error_t initialize(void) { return ERROR_SUCCESS; }
		~deviceC(void) {};

	public:
		driverC *getDriver(void) { return driver; }
		driverInstanceC *getDriverInstance(void)
			{ return driverInstance; }

		error_t addClass(utf8Char *name);
		error_t getEnumerationAttribute(
			utf8Char *name, udi_instance_attr_list_t **attrib);

		error_t setEnumerationAttribute(udi_instance_attr_list_t *attrib);

		void dumpEnumerationAttributes(void);

	public:
		ubit16		id;
		utf8Char	shortName[DEVICE_SHORTNAME_MAXLEN],
				longName[DEVICE_LONGNAME_MAXLEN],
				vendorName[DEVICE_VENDORNAME_MAXLEN],
				vendorContactInfo[DEVICE_VENDORCONTACT_MAXLEN];

		numaBankId_t		bankId;
		driverC			*driver;
		utf8Char		driverFullName[DEVICE_DRIVERNAME_MAXLEN];
		driverInstanceC		*driverInstance;
		ubit8			nEnumerationAttribs, nInstanceAttribs;
		udi_instance_attr_list_t
					**enumeration, **instance;
		utf8Char		(*classes)[DEVICE_CLASS_MAXLEN];
		sbit8			driverDetected, isKernelDriver;
	};

	class driverC
	{
	public:
		struct metalanguageS;
		struct moduleC;
		struct regionS;
		struct channelS;

		driverC(void)
		:
		nModules(0), nRegions(0), nMetalanguages(0),
		allMetalanguagesSatisfied(0), childEnumerationAttribSize(0),
		modules(NULL), regions(NULL), metalanguages(NULL)
		{}

		error_t initialize(utf8Char *fullName)
		{
			return ERROR_SUCCESS;
		}

		~driverC(void){}

	public:
		error_t preallocateModules(uarch_t nModules);
		error_t preallocateRegions(uarch_t nRegions);
		error_t preallocateRequirements(uarch_t nRequirements);
		error_t preallocateMetalanguages(uarch_t nMetalanguages);
		error_t preallocateChildBops(uarch_t nChildBops);
		error_t preallocateParentBops(uarch_t nParentBops);
		error_t preallocateInternalBops(uarch_t nInternalBops);

		struct moduleC
		{
		public:
			moduleC(void)
			:
			index(0), nAttachedRegions(0),
			regionIndexes(NULL)
			{
				filename[0] = '\0';
			}

			moduleC(utf8Char *filename, ubit16 index)
			:
			index(index), nAttachedRegions(0),
			regionIndexes(NULL)
			{
				strcpy8(this->filename, filename);
			}

		public:
			error_t addAttachedRegion(ubit16 index);

		public:
			ubit16		index, nAttachedRegions;
			// Array of region indexes belonging to this module.
			ubit16		*regionIndexes;
			utf8Char	filename[ZUDI_FILENAME_MAXLEN];
		};

		struct regionS
		{
		public:
			// 'moduleIndex' is this region's module's index.
			ubit16		index, moduleIndex,
					nAttachedChannels;
			// Array of channel indexes belonging to this region.
			ubit16		*channelIndexes;
			uarch_t		dataSize;
			ubit32		flags;
		};

		struct metalanguageS
		{
			metalanguageS(void)
			:
			isSatisfied(0)
			{
				name[0] = '\0';
			}

			ubit16		index;
			utf8Char	name[16];
			sarch_t		isSatisfied;
		};

	public:
		error_t addMetalanguage(ubit16 index, utf8Char *name);
		error_t addModule(ubit16 index, utf8Char *name);
		error_t addRegion(ubit16 moduleIndex, ubit16 regionIndex);

		moduleC *getModule(ubit16 index)
		{
			for (uarch_t i=0; i<nModules; i++)
			{
				if (modules[i].index == index)
					{ return &modules[i]; };
			};

			return NULL;
		};

		regionS *getRegion(ubit16 index)
		{
			for (uarch_t i=0; i<nRegions; i++)
			{
				if (regions[i].index == index)
					{ return &regions[i]; };
			};

			return NULL;
		}

		// Kernel doesn't need to know about control block information.

	public:
		utf8Char	fullName[DEVICE_DRIVERNAME_MAXLEN];
		ubit16		nModules, nRegions, nMetalanguages,
				nControlBlocks;
		sbit8		allMetalanguagesSatisfied;

		uarch_t		childEnumerationAttribSize;
		// Modules for this driver, and their indexes.
		moduleC		*modules;
		// Regions in this driver and their indexes/module indexes, etc.
		regionS		*regions;
		// Metalanguage indexes, dependency satisfaction, etc.
		metalanguageS	*metalanguages;
	};
}

#endif

