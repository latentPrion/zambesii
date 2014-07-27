#ifndef _FLOODPLAINN_DEVICE_TREE_H
	#define _FLOODPLAINN_DEVICE_TREE_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <zui.h>
	#include <chipset/chipset.h>
	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kstdlib/__kcxxlib/memory>
	#include <__kclasses/ptrlessList.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/zudiIndexServer.h>
	#include <kernel/common/floodplainn/fvfs.h>	// FVFS_TAG_NAME_MAXLEN

/**	Device:
 * Base type for a device in general. The type of driver used to instantiate
 * the device is independent of this class; both the driver instance and the
 * driver are simply pointed to by the device.
 *
 * Each device has a name and a unique ID.
 **/
#define DRIVER_BASEPATH_MAXLEN			ZUI_DRIVER_BASEPATH_MAXLEN
#define DRIVER_SHORTNAME_MAXLEN			ZUI_DRIVER_SHORTNAME_MAXLEN
#define DRIVER_LONGNAME_MAXLEN			ZUI_MESSAGE_MAXLEN
#define DRIVER_FULLNAME_MAXLEN			\
	(DRIVER_BASEPATH_MAXLEN + DRIVER_SHORTNAME_MAXLEN)
#define DRIVER_VENDORNAME_MAXLEN		ZUI_MESSAGE_MAXLEN
#define DRIVER_VENDORCONTACT_MAXLEN		ZUI_MESSAGE_MAXLEN
#define DRIVER_SUPPLIER_MAXLEN			(ZUI_MESSAGE_MAXLEN)
#define DRIVER_CONTACT_MAXLEN			(ZUI_MESSAGE_MAXLEN)
#define DRIVER_FILENAME_MAXLEN			(128)
#define DRIVER_CLASS_MAXLEN			FVFS_TAG_NAME_MAXLEN
#define DRIVER_METALANGUAGE_MAXLEN		(ZUI_DRIVER_METALANGUAGE_MAXLEN)
#define DRIVER_REQUIREMENT_MAXLEN		(ZUI_DRIVER_METALANGUAGE_MAXLEN)

#define DEVICE_SHORTNAME_MAXLEN			DRIVER_SHORTNAME_MAXLEN
#define DEVICE_LONGNAME_MAXLEN			DRIVER_LONGNAME_MAXLEN
#define DEVICE_FULLNAME_MAXLEN			(DRIVER_FULLNAME_MAXLEN)
#define DEVICE_VENDORNAME_MAXLEN		DRIVER_VENDORNAME_MAXLEN
#define DEVICE_VENDORCONTACT_MAXLEN		DRIVER_VENDORCONTACT_MAXLEN
#define DEVICE_DRIVER_FULLNAME_MAXLEN		DRIVER_FULLNAME_MAXLEN
#define DEVICE_CLASS_MAXLEN			DRIVER_CLASS_MAXLEN


struct sDriverInitEntry
{
	utf8Char	*shortName;
	udi_init_t	*udi_init_info;
};

struct sMetaInitEntry
{
	utf8Char	*shortName;
	udi_mei_init_t	*udi_meta_info;
};

struct sDriverClassMapEntry
{
	utf8Char	*metaName;
	uarch_t		classIndex;
} extern driverClassMap[];

extern utf8Char		*Driverlasses[];

namespace fplainn
{
	class Driver;
	class DeviceInstance;
	class DriverInstance;

	/**	Device:
	 * Data type used to represent a single device in the kernel's device
	 * tree.
	 **********************************************************************/
	class Device
	:
	public vfs::DirInode<fvfs::Tag>
	{
	public:
		Device(numaBankId_t bid)
		:
		bankId(bid), driverInstance(NULL), instance(NULL),
		nEnumerationAttrs(0), nInstanceAttrs(0), nClasses(0),
		nParentTags(0),
		enumerationAttrs(NULL), instanceAttrs(NULL), classes(NULL),
		driverDetected(0),
		driverIndex(zuiServer::INDEX_KERNEL),
		requestedIndex(zuiServer::INDEX_KERNEL),
		parentTags(NULL), parentTagCounter(0)
		{
			this->longName[0]
				= this->driverFullName[0] = '\0';
		}

		error_t initialize(void)
		{
			return vfs::DirInode<fvfs::Tag>::initialize();
		}

		~Device(void) {};

	public:
		/**	EXPLANATION:
		 * These are the publicly exposed wrappers around the underlying
		 * vfs::DirInode:: namespace methods. We hid the *DirTag()
		 * functions with overloads, and then "renamed" them to
		 * *Child() so we could have more intuitive naming, and more
		 * suitable function prototypes.
		 **/
		error_t createChild(
			utf8Char *name, fvfs::Tag *parent,
			Device *device, fvfs::Tag **tag)
		{
			error_t		ret;

			if (name == NULL || tag == NULL || device == NULL)
				{ return ERROR_INVALID_ARG; };

			*tag = createDirTag(
				name, vfs::DEVICE, parent, device, &ret);

			if (ret != ERROR_SUCCESS) { return ret; };
			return ERROR_SUCCESS;
		}

		fvfs::Tag *getChild(utf8Char *name) { return getDirTag(name); }
		sarch_t removeChild(utf8Char *name)
			{ return removeDirTag(name); }

	public:
		struct sParentTag
		{
			sParentTag(void)
			:
			id(0), tag(NULL)
			{}

			sParentTag(ubit16 id, fvfs::Tag *tag)
			:
			id(id), tag(tag)
			{}

			/* Each parent device will have a device-relative ID.
			 * This ID is used as the UDI parent ID number.
			 *
			 * We don't care about being prudent with our allocation
			 * of these IDs. We just hand them out from a counter,
			 * not checking for collisions, because it's a very
			 * unlikely thing that a device will have parents bound
			 * and unbound to it so much that it will overflow even
			 * a 16 bit counter;
			 *
			 * Talk much less about a 32 bit counter.
			 **/
			ubit16		id;
			fvfs::Tag	*tag;
		};

		error_t addClass(utf8Char *name);
		error_t addEnumerationAttribute(udi_instance_attr_list_t *attrib);
		error_t getEnumerationAttribute(
			utf8Char *name, udi_instance_attr_list_t *attrib);

		error_t addParentTag(fvfs::Tag *tag, ubit16 *newIdRetval);
		void removeParentTag(fvfs::Tag *tag);

		void dumpEnumerationAttributes(void);

	private:
		/* Not meant to be used by callers. Used only internally as
		 * wrapper functions. Deliberately made private.
		 **/
		fvfs::Tag *createDirTag(
			utf8Char *name, vfs::tagTypeE type,
			fvfs::Tag *parent, vfs::DirInode<fvfs::Tag> *dev,
			error_t *err)
		{
			return vfs::DirInode<fvfs::Tag>::createDirTag(
				name, type, parent, dev, err);
		}

		sarch_t removeDirTag(utf8Char *n)
			{ return vfs::DirInode<fvfs::Tag>::removeDirTag(n); }

		fvfs::Tag *getDirTag(utf8Char *name)
			{ return vfs::DirInode<fvfs::Tag>::getDirTag(name); }

		/* The leaf functions are not meant to be used at all by anyone;
		 * not even internals within this class, and they override the
		 * base-class functions of the same name, returning error
		 * unconditionally.
		 **/
		fvfs::Tag *createLeafTag(
			utf8Char *, vfs::tagTypeE, fvfs::Tag *,
			vfs::Inode *, error_t *err)
		{
			*err = ERROR_UNIMPLEMENTED;
			return NULL;
		}

		sarch_t removeLeafTag(utf8Char *) { return 0; }
		fvfs::Tag *getLeafTag(utf8Char *) { return NULL; }
		error_t findEnumerationAttribute(
			utf8Char *name, udi_instance_attr_list_t **attr);

	public:
		utf8Char	longName[DEVICE_LONGNAME_MAXLEN],
				driverFullName[DEVICE_FULLNAME_MAXLEN];

		/* Vendor name and contact info should be retrieved from the
		 * driver object, instead of unnecessarily being duplicated.
		 **/
		numaBankId_t		bankId;
		DriverInstance		*driverInstance;
		DeviceInstance		*instance;
		ubit8			nEnumerationAttrs, nInstanceAttrs,
					nClasses, nParentTags;
		HeapArr<HeapObj<udi_instance_attr_list_t> >
					enumerationAttrs, instanceAttrs;
		utf8Char		(*classes)[DEVICE_CLASS_MAXLEN];
		sbit8			driverDetected;
		// The index which enumerated this device's driver.
		zuiServer::indexE	driverIndex, requestedIndex;
		sParentTag		*parentTags;
		uarch_t			parentTagCounter;
	};

	/**	DeviceInstance:
	 * Device-instance specific attributes.
	 **********************************************************************/
	class DeviceInstance
	{
	public:
		DeviceInstance(Device *dev)
		:
		nChannels(0),
		device(dev),
		regions(NULL), channels(NULL),
		nRegionsInitialized(0), nRegionsFailed(0)
		{}

		error_t initialize(void);

	public:
		void setRegionInfo(
			ubit16 index,
			processId_t tid, udi_init_context_t *rdata);

		error_t getRegionInfo(
			processId_t tid, ubit16 *index,
			udi_init_context_t **rdata);

		error_t getRegionInfo(
			ubit16 index, processId_t *tid,
			udi_init_context_t **rdata);

	public:
		struct sRegion
		{
			ubit16			index;
			processId_t		tid;
			udi_init_context_t	*rdata;
		};

		struct sChannel
		{
			sChannel(
				Thread *t0, udi_ops_vector_t *opsVec0,
				udi_init_context_t *chanContext0,
				Thread *t1, udi_ops_vector_t *opsVec1,
				udi_init_context_t *chanContext1)
			{
				new (&endpoints[0]) sEndpoint(
					this, &endpoints[1],
					t0, opsVec0, chanContext0);

				new (&endpoints[1]) sEndpoint(
					this, &endpoints[0],
					t1, opsVec1, chanContext1);
			}

			struct sEndpoint
			{
				friend struct fplainn::DeviceInstance::sChannel;
				// Private, void constructor that does nothing.
				sEndpoint(void) {}

			public:
				sEndpoint(
					sChannel *parent, sEndpoint *otherEnd,
					Thread *thread,
					udi_ops_vector_t *opsVector,
					udi_init_context_t *chanContext)
				:
				parent(parent), otherEnd(otherEnd),
				thread(thread),
				opsVector(opsVector),
				channelContext(chanContext)
				{}

				sChannel		*parent;
				sEndpoint		*otherEnd;
				Thread			*thread;
				udi_ops_vector_t	*opsVector;
				udi_init_context_t	*channelContext;
			};

			struct sIncompleteChannel
			{
				sIncompleteChannel(void)
				:
				spawnIndex(-1), channel(NULL)
				{}

				List<sIncompleteChannel>::sHeader
					listHeader;

				udi_index_t	spawnIndex;
				sChannel	*channel;
			};

			/**	EXPLANATION:
			 * This is a list of all incomplete spawn operations
			 * that are taking place across this channel. The
			 * spawn_idx is recorded until two udi_channel_spawn()
			 * operations are paired up. At that point, a channel
			 * spawn is completed and the spawn_idx is removed from
			 * this list.
			 **/
			List<sIncompleteChannel> incompleteChannels;
			sEndpoint				endpoints[2];
		};

		error_t addChannel(sChannel *newChan);
		void removeChannel(sChannel *chan);

	public:
		ubit16			nChannels;
		Device			*device;
		sRegion			*regions;
		sChannel		**channels;
		udi_init_context_t	*mgmtChannelContext;

		// XXX: ONLY to be used by __klibzbzcore.
		uarch_t			nRegionsInitialized, nRegionsFailed;
	};

	class DriverInstance;

	/**	Driver:
	 * Driver represents a UDI driver in the kernel's metadata. It stores
	 * enough information about a driver to enable the kernel to instantiate
	 * a process for devices which are driven by that driver.
	 **********************************************************************/
	class Driver
	{
	public:
		struct sModule;
		struct sRegion;
		struct sRequirement;
		struct sMetalanguage;
		struct sChildBop;
		struct sParentBop;
		struct sInternalBop;

		Driver(zuiServer::indexE index)
		:
		index(index),
		nModules(0), nRegions(0), nRequirements(0), nMetalanguages(0),
		nChildBops(0), nParentBops(0), nInternalBops(0), nInstances(0),
		nClasses(0),
		instances(NULL), allRequirementsSatisfied(0),
		childEnumerationAttrSize(0),
		modules(NULL), regions(NULL), requirements(NULL),
		metalanguages(NULL), childBops(NULL), parentBops(NULL),
		internalBops(NULL), classes(NULL)
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

		~Driver(void){}

	public:
		error_t preallocateModules(uarch_t nModules);
		error_t preallocateRegions(uarch_t nRegions);
		error_t preallocateRequirements(uarch_t nRequirements);
		error_t preallocateMetalanguages(uarch_t nMetalanguages);
		error_t preallocateChildBops(uarch_t nChildBops);
		error_t preallocateParentBops(uarch_t nParentBops);
		error_t preallocateInternalBops(uarch_t nInternalBops);
		error_t detectClasses(void);

		error_t addInstance(numaBankId_t bid, processId_t pid);
		DriverInstance *getInstance(numaBankId_t bid);

		void dump(void);

		struct sModule
		{
			sModule(ubit16 index, utf8Char *filename)
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
			utf8Char	filename[DRIVER_SHORTNAME_MAXLEN];

		private:
			friend class fplainn::Driver;
			sModule(void)
			:
			index(0), nAttachedRegions(0), regionIndexes(NULL)
			{
				filename[0] = '\0';
			}
		};

		struct sRegion
		{
			sRegion(ubit16 index, ubit16 moduleIndex, ubit32 flags)
			:
			index(index), moduleIndex(moduleIndex),
			dataSize(0), flags(flags)
			{}

			void dump(void);

			// 'moduleIndex' is this region's module's index.
			ubit16		index, moduleIndex;
			// Array of channel indexes belonging to this region.
			uarch_t		dataSize;
			ubit32		flags;

		private:
			friend class fplainn::Driver;
			sRegion(void)
			:
			index(0), moduleIndex(0),
			dataSize(0), flags(0)
			{}
		};

		struct sRequirement
		{
			sRequirement(utf8Char *name, ubit32 version)
			:
			fullName(NULL), version(version)
			{
				strcpy8(this->name, name);
			}

			void dump(void);

			utf8Char	name[DRIVER_REQUIREMENT_MAXLEN],
					*fullName;
			ubit32		version;

		private:
			friend class fplainn::Driver;
			sRequirement(void)
			:
			fullName(NULL), version(0)
			{
				name[0] = '\0';
			}
		};

		struct sMetalanguage
		{
			sMetalanguage(
				ubit16 index, utf8Char *name,
				udi_mei_init_t *udi_meta_info)
			:
			index(index), udi_meta_info(udi_meta_info)
			{
				strcpy8(this->name, name);
			}

			void dump(void);

			ubit16		index;
			utf8Char	name[DRIVER_METALANGUAGE_MAXLEN];
			// XXX: Only used by the kernel for kernelspace drivers.
			const udi_mei_init_t	*udi_meta_info;

		private:
			friend class fplainn::Driver;
			sMetalanguage(void)
			:
			index(0), udi_meta_info(NULL)
			{
				name[0] = '\0';
			}
		};

		struct sChildBop
		{
			sChildBop(
				ubit16 metaIndex, ubit16 regionIndex,
				ubit16 opsIndex)
			:
			metaIndex(metaIndex), regionIndex(regionIndex),
			opsIndex(opsIndex)
			{}

			void dump(void);

			ubit16		metaIndex, regionIndex, opsIndex;

		private:
			friend class fplainn::Driver;
			sChildBop(void)
			:
			metaIndex(0), regionIndex(0), opsIndex(0)
			{}
		};

		struct sParentBop
		{
			sParentBop(
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
			friend class fplainn::Driver;
			sParentBop(void)
			:
			metaIndex(0), regionIndex(0), opsIndex(0),
			bindCbIndex(0)
			{}
		};

		struct sInternalBop
		{
			sInternalBop(
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
			friend class fplainn::Driver;
			sInternalBop(void)
			:
			metaIndex(0), regionIndex(0),
			opsIndex0(0), opsIndex1(0), bindCbIndex(0)
			{}
		};

	public:
		sMetalanguage *getMetalanguage(ubit16 index)
		{
			for (uarch_t i=0; i<nMetalanguages; i++)
			{
				if (metalanguages[i].index == index)
					{ return &metalanguages[i]; };
			};

			return NULL;
		}

		sMetalanguage *getMetalanguage(utf8Char *name)
		{
			for (uarch_t i=0; i<nMetalanguages; i++)
			{
				if (!strcmp8(metalanguages[i].name, name))
					{ return &metalanguages[i]; };
			};

			return NULL;
		}

		sModule *getModule(ubit16 index)
		{
			for (uarch_t i=0; i<nModules; i++)
			{
				if (modules[i].index == index)
					{ return &modules[i]; };
			};

			return NULL;
		};

		sRegion *getRegion(ubit16 index)
		{
			for (uarch_t i=0; i<nRegions; i++)
			{
				if (regions[i].index == index)
					{ return &regions[i]; };
			};

			return NULL;
		}

		sChildBop *getChildBop(ubit16 metaIndex)
		{
			for (uarch_t i=0; i<nChildBops; i++)
			{
				if (childBops[i].metaIndex == metaIndex)
					{ return &childBops[i]; };
			};

			return NULL;
		}

		sParentBop *getParentBop(ubit16 metaIndex)
		{
			for (uarch_t i=0; i<nParentBops; i++)
			{
				if (parentBops[i].metaIndex == metaIndex)
					{ return &parentBops[i]; };
			};

			return NULL;
		}

		sInternalBop *getInternalBop(ubit16 regionIndex)
		{
			for (uarch_t i=0; i<nInternalBops; i++)
			{
				if (internalBops[i].regionIndex == regionIndex)
					{ return &internalBops[i]; };
			};

			return NULL;
		}

		// XXX: Only used by kernel for kernelspace drivers.
		const udi_mei_init_t *getMetaInitInfo(const utf8Char *name)
		{
			for (uarch_t i=0; i<nMetalanguages; i++)
			{
				if (!strcmp8(metalanguages[i].name, name)) {
					return metalanguages[i].udi_meta_info;
				};
			};

			return NULL;
		}

		// Kernel doesn't need to know about control block information.

	public:
		zuiServer::indexE		index;

		utf8Char	basePath[DRIVER_BASEPATH_MAXLEN],
				shortName[DRIVER_SHORTNAME_MAXLEN],
				longName[DRIVER_LONGNAME_MAXLEN],
				supplier[DRIVER_SUPPLIER_MAXLEN],
				supplierContact[DRIVER_CONTACT_MAXLEN];
		ubit16		nModules, nRegions, nRequirements,
				nMetalanguages, nChildBops, nParentBops,
				nInternalBops, nInstances, nClasses;

		/**	NOTES:
		 * Zambesii's definition of "driver instances" is not conformant
		 * to the UDI definition. We use "device instance" to cover what
		 * UDI defines as a "driver instance".
		 *
		 * Driver instances in Zambesii are driver process address
		 * spaces. A driver instance does not imply a separate device
		 * instance, and in fact most driver instances will host
		 * multiple devices.
		 **/
		DriverInstance	*instances;

		sbit8		allRequirementsSatisfied;
		uarch_t		childEnumerationAttrSize;
		// Modules for this driver, and their indexes.
		sModule		*modules;
		// Regions in this driver and their indexes/module indexes, etc.
		sRegion		*regions;
		// All required libraries for this driver.
		sRequirement	*requirements;
		// Metalanguage indexes, names, etc.
		sMetalanguage	*metalanguages;
		sChildBop	*childBops;
		sParentBop	*parentBops;
		sInternalBop	*internalBops;
		utf8Char	(*classes)[DRIVER_CLASS_MAXLEN];

		// XXX: ONLY for use by libzbzcore, and ONLY in kernel-space.
		const udi_init_t	*driverInitInfo;
	};

	/**	DriverInstance
	 * Represents a driver process that has been instantiated. When a driver
	 * has device instances that span multiple NUMA domains, the kernel will
	 * have to create a new driver process per NUMA domain.
	 *
	 * This will facilitate the proper implementation of per-device NUMA
	 * locality optimization, since the threads for device instances of
	 * each driver instance (each of which is in its own NUMA domain) will
	 * be bound to CPUs within that domain. Furthermore, since the driver
	 * instance is its own process, the kernel can set its NUMA allocation
	 * policy to allocate from its bank. NUMA address space binding also
	 * gets used to its full extent.
	 **/
	class DriverInstance
	{
	public:
		DriverInstance(void)
		:
		driver(NULL), bankId(NUMABANKID_INVALID), pid(PROCID_INVALID),
		childBopVectors(NULL),
		nHostedDevices(0), hostedDevices(NULL)
		{}

		DriverInstance(
			Driver *driver, numaBankId_t bid, processId_t pid)
		:
		driver(driver), bankId(bid), pid(pid), childBopVectors(NULL),
		nHostedDevices(0), hostedDevices(NULL)
		{}

		error_t initialize(void);

	public:
		struct sChildBop
		{
			/* Parent BOps can only be uniquely identified by their
			 * metalanguage indexes.
			 **/
			ubit16			metaIndex;
			udi_ops_vector_t	*opsVector;
		};

		struct sManagementChannel
		{
			sManagementChannel(void)
			:
			opsVector(NULL), scratchSize(0)
			{}

			udi_mgmt_ops_t		*opsVector;
			uarch_t			scratchSize;
			udi_ubit32_t		opFlags;
		};

	public:
		void setChildBopVector(
			ubit16 metaIndex, udi_ops_vector_t *vaddr)
		{
			for (uarch_t i=0; i<driver->nChildBops; i++)
			{
				if (childBopVectors[i].metaIndex != metaIndex)
					{ continue; };

				childBopVectors[i].opsVector = vaddr;
				return;
			};
		}

		sChildBop *getChildBop(ubit16 metaIndex)
		{
			for (uarch_t i=0; i<driver->nChildBops; i++)
			{
				if (childBopVectors[i].metaIndex == metaIndex)
					{ return &childBopVectors[i]; };
			};

			return NULL;
		}

		void setMgmtChannelInfo(
			udi_mgmt_ops_t *mgmtOpsVector,
			uarch_t scratchSize,
			udi_ubit32_t opFlags)
		{
			managementChannel.opsVector = mgmtOpsVector;
			managementChannel.scratchSize = scratchSize;
			managementChannel.opFlags = opFlags;
		}

		error_t addHostedDevice(utf8Char *path);
		void removeHostedDevice(utf8Char *path);

	public:
		Driver				*driver;
		numaBankId_t			bankId;
		processId_t			pid;
		sChildBop			*childBopVectors;
		uarch_t				nHostedDevices;
		HeapArr<HeapArr<utf8Char> >	hostedDevices;
		sManagementChannel		managementChannel;
	};
}


/**	Inline methods.
 ******************************************************************************/

inline void fplainn::DeviceInstance::setRegionInfo(
	ubit16 index, processId_t tid, udi_init_context_t *rdata
	)
{
	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].index != index) { continue; };

		regions[i].tid = tid;
		regions[i].rdata = rdata;
		return;
	};
}

inline error_t fplainn::DeviceInstance::getRegionInfo(
	processId_t tid, ubit16 *index, udi_init_context_t **rdata
	)
{
	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].tid != tid) { continue; };
		*index = regions[i].index;
		*rdata = regions[i].rdata;
		return ERROR_SUCCESS;
	};

	return ERROR_NOT_FOUND;
}

inline error_t fplainn::DeviceInstance::getRegionInfo(
	ubit16 index, processId_t *tid, udi_init_context_t **rdata
	)
{
	for (uarch_t i=0; i<device->driverInstance->driver->nRegions; i++)
	{
		if (regions[i].index != index) { continue; };
		*tid = regions[i].tid;
		*rdata = regions[i].rdata;
		return ERROR_SUCCESS;
	};

	return ERROR_NOT_FOUND;
}

inline fplainn::DriverInstance *fplainn::Driver::getInstance(numaBankId_t bid)
{
	for (uarch_t i=0; i<nInstances; i++)
	{
		if (instances[i].bankId == bid)
			{ return &instances[i]; };
	};

	return NULL;
}

#endif
