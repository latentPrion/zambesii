#ifndef _FLOODPLAINN_DEVICE_TREE_H
	#define _FLOODPLAINN_DEVICE_TREE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
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

#define DEVICE_CLASS_MAXLEN			(48)

#define DEVICE_ATTR_NAME_MAXLEN			(32)
#define DEVICE_ATTR_VALUE_MAXLEN		(64)

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
		id(id), driver(__KNULL), instance(__KNULL), attributes(__KNULL)
		{
			if (shortName != __KNULL)
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
		driverInstanceC *getDriverInstance(void) { return instance; }

		error_t addClass(utf8Char *name);

		struct instanceAttributeS
		{
			instanceAttributeS(void)
			:
			valueLength(0), type(NONE)
			{
				value[0] = name[0] = '\0';
			}

			enum typeE {
				NONE=0, STRING, ARRAY8, UBIT32, BOOLEAN, FILE };

			utf8Char	name[DEVICE_ATTR_NAME_MAXLEN],
					value[DEVICE_ATTR_VALUE_MAXLEN];
			ubit8		valueLength;
			typeE		type;
		};

	public:
		ubit16		id;
		utf8Char	shortName[DEVICE_SHORTNAME_MAXLEN],
				longName[DEVICE_LONGNAME_MAXLEN],
				vendorName[DEVICE_VENDORNAME_MAXLEN],
				vendorContactInfo[DEVICE_VENDORCONTACT_MAXLEN];

		numaBankId_t		bankId;
		driverC			*driver;
		driverInstanceC		*instance;
		instanceAttributeS	*attributes;
		utf8Char		(*classes)[DEVICE_CLASS_MAXLEN];
	};

	class driverC
	{
	public:
		struct metalanguageS;
		struct moduleS;
		struct regionS;
		struct channelS;

		driverC(void)
		:
		nModules(0), nRegions(0), nChannels(0), nMetalanguages(0),
		allMetalanguagesSatisfied(0), childEnumerationAttribSize(0),
		moduleInfo(__KNULL), regionInfo(__KNULL), channelInfo(__KNULL),
		metalanguageInfo(__KNULL)
		{}

		error_t initialize(void){ return ERROR_SUCCESS; }
		~driverC(void){}

	public:
		error_t addMetalanguage(ubit16 index, utf8Char *name);
		error_t addModule(ubit16 index);
		error_t addRegion(ubit16 moduleIndex, ubit16 regionIndex);
		error_t addChannel(ubit16 regionIndex, ubit16 index);

		moduleS *getModule(ubit16 index);
		regionS *getRegion(ubit16 index);
		channelS *getChannel(ubit16 index);

	public:
		struct moduleS
		{
			ubit16		index, nAttachedRegions;
			// Array of region indexes belonging to this module.
			ubit16		*regionIndexes;
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

		struct channelS
		{
			// 'regionIndex' is this channel's region's index.
			ubit16		index, regionIndex,
					metalanguageIndex;
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

		// Kernel doesn't need to know about control block information.

	public:
		ubit16		nModules, nRegions, nChannels, nMetalanguages,
				nControlBlocks;
		sbit8		allMetalanguagesSatisfied;

		uarch_t		childEnumerationAttribSize;
		// Modules for this driver, and their indexes.
		moduleS		*moduleInfo;
		// Regions in this driver and their indexes/module indexes, etc.
		regionS		*regionInfo;
		// Driver's channels and their indexes, channel indexes, etc.
		channelS	*channelInfo;
		// Metalanguage indexes, dependency satisfaction, etc.
		metalanguageS	*metalanguageInfo;
	};

	
}

#endif

