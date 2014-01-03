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
	#include <kernel/common/zudiIndexServer.h>

/**	deviceC:
 * Base type for a device in general. The type of driver used to instantiate
 * the device is independent of this class; both the driver instance and the
 * driver are simply pointed to by the device.
 *
 * Each device has a name and a unique ID.
 **/
#define DRIVER_LONGNAME_MAXLEN			ZUDI_MESSAGE_MAXLEN
#define DRIVER_VENDORNAME_MAXLEN		ZUDI_MESSAGE_MAXLEN
#define DRIVER_VENDORCONTACT_MAXLEN		ZUDI_MESSAGE_MAXLEN
#define DRIVER_FULLNAME_MAXLEN			\
	(ZUDI_DRIVER_BASEPATH_MAXLEN + ZUDI_DRIVER_SHORTNAME_MAXLEN)

#define DEVICE_SHORTNAME_MAXLEN			DRIVER_SHORTNAME_MAXLEN
#define DEVICE_LONGNAME_MAXLEN			DRIVER_LONGNAME_MAXLEN
#define DEVICE_VENDORNAME_MAXLEN		DRIVER_VENDORNAME_MAXLEN
#define DEVICE_VENDORCONTACT_MAXLEN		DRIVER_VENDORCONTACT_MAXLEN
#define DEVICE_DRIVER_FULLNAME_MAXLEN		DRIVER_FULLNAME_MAXLEN
#define DEVICE_CLASS_MAXLEN			(48)

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
		enumeration(NULL), instance(NULL), classes(NULL),
		driverDetected(0), isKernelDriver(0)
		{
			this->shortName[0] = this->longName[0]
				= this->driverFullName[0] = '\0';

			if (shortName != NULL)
			{
				strncpy8(
					this->shortName, shortName,
					ZUDI_DRIVER_SHORTNAME_MAXLEN);
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
			utf8Char *name, udi_instance_attr_list_t *attrib);

		error_t setEnumerationAttribute(udi_instance_attr_list_t *attrib);

		error_t preallocateRequirements(uarch_t nRequirements);

		void dumpEnumerationAttributes(void);

	public:
		ubit16		id;
		utf8Char	shortName[ZUDI_DRIVER_SHORTNAME_MAXLEN],
				longName[ZUDI_MESSAGE_MAXLEN];

		/* Vendor name and contact info should be retrieved from the
		 * driver object, instead of unnecessarily being duplicated.
		 **/

		numaBankId_t		bankId;
		driverC			*driver;
		driverInstanceC		*driverInstance;
		// The index which enumerated this device's driver.
		zudiIndexServer::indexE	driverIndex;
		utf8Char		**requirements;
		ubit16			nRequirements;
		utf8Char		driverFullName[DRIVER_FULLNAME_MAXLEN];
		ubit8			nEnumerationAttribs, nInstanceAttribs;
		udi_instance_attr_list_t
					**enumeration, **instance;
		utf8Char		(*classes)[DEVICE_CLASS_MAXLEN];
		sbit8			driverDetected, isKernelDriver;
	};

	/**	driverC:
	 * driverC represents a UDI driver in the kernel's metadata. It stores
	 * enough information about a driver to enable the kernel to instantiate
	 * a process for devices which are driven by that driver.
	 **********************************************************************/
	class driverC
	{
	public:
		struct moduleS;
		struct regionS;
		struct requirementS;
		struct metalanguageS;
		struct childBopS;
		struct parentBopS;
		struct internalBopS;

		driverC(void)
		:
		nModules(0), nRegions(0), nRequirements(0), nMetalanguages(0),
		nChildBops(0), nParentBops(0), nInternalBops(0),
		childEnumerationAttrSize(0),
		modules(NULL), regions(NULL), requirements(NULL),
		metalanguages(NULL), childBops(NULL), parentBops(NULL),
		internalBops(NULL)
		{
			basePath[0] = shortName[0] = longName[0]
				= supplier[0] = supplierContact[0] = '\0';
		}

		error_t initialize(utf8Char *basePath, utf8Char *shortName)
		{
			strcpy8(this->basePath, basePath);
			strcpy8(this->shortName, shortName);
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

		void dump(void);

		struct moduleS
		{
			moduleS(ubit16 index, utf8Char *filename)
			:
			index(index), nAttachedRegions(0), regionIndexes(NULL)
			{
				if (filename != NULL)
					{ strcpy8(this->filename, filename); }
				else { filename[0] = '\0'; };
			}

			error_t addAttachedRegion(ubit16 regionIndex);

			void dump(void);

			ubit16		index, nAttachedRegions;
			// Array of region indexes belonging to this module.
			ubit16		*regionIndexes;
			utf8Char	filename[ZUDI_FILENAME_MAXLEN];

		private:
			friend class fplainn::driverC;
			moduleS(void)
			:
			index(0), nAttachedRegions(0), regionIndexes(NULL)
			{
				filename[0] = '\0';
			}
		};

		struct regionS
		{
			regionS(ubit16 index, ubit16 moduleIndex, ubit32 flags)
			:
			index(index), moduleIndex(moduleIndex),
			nAttachedChannels(0), channelIndexes(NULL),
			dataSize(0), flags(flags)
			{}

			void dump(void);

			// 'moduleIndex' is this region's module's index.
			ubit16		index, moduleIndex,
					nAttachedChannels;
			// Array of channel indexes belonging to this region.
			ubit16		*channelIndexes;
			uarch_t		dataSize;
			ubit32		flags;

		private:
			friend class fplainn::driverC;
			regionS(void)
			:
			index(0), moduleIndex(0),
			nAttachedChannels(0), channelIndexes(NULL),
			dataSize(0), flags(0)
			{}
		};

		struct requirementS
		{
			requirementS(utf8Char *name, ubit32 version)
			:
			version(version)
			{
				strcpy8(this->name, name);
			}

			void dump(void);

			utf8Char	name[ZUDI_DRIVER_REQUIREMENT_MAXLEN];
			ubit32		version;

		private:
			friend class fplainn::driverC;
			requirementS(void)
			:
			version(0)
			{
				name[0] = '\0';
			}
		};

		struct metalanguageS
		{
			metalanguageS(ubit16 index, utf8Char *name)
			:
			index(index)
			{
				strcpy8(this->name, name);
			}

			void dump(void);

			ubit16		index;
			utf8Char	name[ZUDI_DRIVER_METALANGUAGE_MAXLEN];

		private:
			friend class fplainn::driverC;
			metalanguageS(void)
			:
			index(0)
			{
				name[0] = '\0';
			}
		};

		struct childBopS
		{
			childBopS(
				ubit16 metaIndex, ubit16 regionIndex,
				ubit16 opsIndex)
			:
			metaIndex(metaIndex), regionIndex(regionIndex),
			opsIndex(opsIndex)
			{}

			void dump(void);

			ubit16		metaIndex, regionIndex, opsIndex;

		private:
			friend class fplainn::driverC;
			childBopS(void)
			:
			metaIndex(0), regionIndex(0), opsIndex(0)
			{}
		};

		struct parentBopS
		{
			parentBopS(
				ubit16 metaIndex, ubit16 regionIndex,
				ubit16 opsIndex, ubit16 bindCbIndex)
			:
			metaIndex(metaIndex), regionIndex(regionIndex),
			opsIndex(opsIndex), bindCbIndex(bindCbIndex)
			{}

			void dump(void);

			ubit16		metaIndex, regionIndex, opsIndex,
					bindCbIndex;

		private:
			friend class fplainn::driverC;
			parentBopS(void)
			:
			metaIndex(0), regionIndex(0), opsIndex(0),
			bindCbIndex(0)
			{}
		};

		struct internalBopS
		{
			internalBopS(
				ubit16 metaIndex, ubit16 regionIndex,
				ubit16 opsIndex0, ubit16 opsIndex1,
				ubit16 bindCbIndex)
			:
			metaIndex(metaIndex), regionIndex(regionIndex),
			opsIndex0(opsIndex0), opsIndex1(opsIndex1),
			bindCbIndex(bindCbIndex)
			{}

			void dump(void);

			ubit16		metaIndex, regionIndex,
					opsIndex0, opsIndex1,
					bindCbIndex;

		private:
			friend class fplainn::driverC;
			internalBopS(void)
			:
			metaIndex(0), regionIndex(0),
			opsIndex0(0), opsIndex1(0), bindCbIndex(0)
			{}
		};

	public:
		moduleS *getModule(ubit16 index)
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
		utf8Char	basePath[ZUDI_DRIVER_BASEPATH_MAXLEN],
				shortName[ZUDI_DRIVER_SHORTNAME_MAXLEN],
				longName[ZUDI_MESSAGE_MAXLEN],
				supplier[ZUDI_MESSAGE_MAXLEN],
				supplierContact[ZUDI_MESSAGE_MAXLEN];
		ubit16		nModules, nRegions, nRequirements,
				nMetalanguages, nChildBops, nParentBops,
				nInternalBops;

		uarch_t		childEnumerationAttrSize;
		// Modules for this driver, and their indexes.
		moduleS		*modules;
		// Regions in this driver and their indexes/module indexes, etc.
		regionS		*regions;
		// All metalanguage required libraries for this driver.
		requirementS	*requirements;
		// Metalanguage indexes, dependency satisfaction, etc.
		metalanguageS	*metalanguages;
		childBopS	*childBops;
		parentBopS	*parentBops;
		internalBopS	*internalBops;
	};
}

#endif

