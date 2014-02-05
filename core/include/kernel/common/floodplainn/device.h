#ifndef _FLOODPLAINN_DEVICE_TREE_H
	#define _FLOODPLAINN_DEVICE_TREE_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <zudiIndex.h>
	#include <chipset/chipset.h>
	#include <chipset/memory.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kclib/string8.h>
	#include <__kstdlib/__kclib/string.h>
	#include <__kclasses/ptrlessList.h>
	#include <kernel/common/thread.h>
	#include <kernel/common/numaTypes.h>
	#include <kernel/common/zudiIndexServer.h>
	#include <kernel/common/floodplainn/fvfs.h>	// FVFS_TAG_NAME_MAXLEN

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
#define DRIVER_CLASS_MAXLEN			FVFS_TAG_NAME_MAXLEN

#define DEVICE_SHORTNAME_MAXLEN			DRIVER_SHORTNAME_MAXLEN
#define DEVICE_LONGNAME_MAXLEN			DRIVER_LONGNAME_MAXLEN
#define DEVICE_VENDORNAME_MAXLEN		DRIVER_VENDORNAME_MAXLEN
#define DEVICE_VENDORCONTACT_MAXLEN		DRIVER_VENDORCONTACT_MAXLEN
#define DEVICE_DRIVER_FULLNAME_MAXLEN		DRIVER_FULLNAME_MAXLEN
#define DEVICE_CLASS_MAXLEN			DRIVER_CLASS_MAXLEN


struct driverInitEntryS
{
	utf8Char	*shortName;
	udi_init_t	*udi_init_info;
};

struct metaInitEntryS
{
	utf8Char	*shortName;
	udi_mei_init_t	*udi_meta_info;
};

struct driverClassMapEntryS
{
	utf8Char	*metaName;
	uarch_t		classIndex;
} extern driverClassMap[];

extern utf8Char		*driverClasses[];

namespace fplainn
{
	class driverC;
	class deviceInstanceC;
	class driverInstanceC;

	/**	deviceC:
	 * Data type used to represent a single device in the kernel's device
	 * tree.
	 **********************************************************************/
	class deviceC
	:
	public vfs::dirInodeC<fvfs::tagC>
	{
	public:
		deviceC(numaBankId_t bid)
		:
		bankId(bid), driverInstance(NULL), instance(NULL),
		nEnumerationAttrs(0), nInstanceAttrs(0), nClasses(0),
		nParentTags(0),
		enumerationAttrs(NULL), instanceAttrs(NULL), classes(NULL),
		driverDetected(0),
		driverIndex(zudiIndexServer::INDEX_KERNEL),
		requestedIndex(zudiIndexServer::INDEX_KERNEL),
		parentTags(NULL), parentTagCounter(0)
		{
			this->longName[0]
				= this->driverFullName[0] = '\0';
		}

		error_t initialize(void)
		{
			return vfs::dirInodeC<fvfs::tagC>::initialize();
		}

		~deviceC(void) {};

	public:
		/**	EXPLANATION:
		 * These are the publicly exposed wrappers around the underlying
		 * vfs::dirInodeC:: namespace methods. We hid the *DirTag()
		 * functions with overloads, and then "renamed" them to
		 * *Child() so we could have more intuitive naming, and more
		 * suitable function prototypes.
		 **/
		error_t createChild(
			utf8Char *name, fvfs::tagC *parent,
			deviceC *device, fvfs::tagC **tag)
		{
			error_t		ret;

			if (name == NULL || tag == NULL || device == NULL)
				{ return ERROR_INVALID_ARG; };

			*tag = createDirTag(
				name, vfs::DEVICE, parent, device, &ret);

			if (ret != ERROR_SUCCESS) { return ret; };
			return ERROR_SUCCESS;
		}

		fvfs::tagC *getChild(utf8Char *name) { return getDirTag(name); }
		sarch_t removeChild(utf8Char *name)
			{ return removeDirTag(name); }

	public:
		struct parentTagS
		{
			parentTagS(void)
			:
			id(0), tag(NULL)
			{}

			parentTagS(ubit16 id, fvfs::tagC *tag)
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
			fvfs::tagC	*tag;
		};

		error_t addClass(utf8Char *name);
		error_t getEnumerationAttribute(
			utf8Char *name, udi_instance_attr_list_t *attrib);

		error_t setEnumerationAttribute(udi_instance_attr_list_t *attrib);

		error_t addParentTag(fvfs::tagC *tag, ubit16 *newIdRetval);
		void removeParentTag(fvfs::tagC *tag);

		void dumpEnumerationAttributes(void);

	private:
		/* Not meant to be used by callers. Used only internally as
		 * wrapper functions. Deliberately made private.
		 **/
		fvfs::tagC *createDirTag(
			utf8Char *name, vfs::tagTypeE type,
			fvfs::tagC *parent, vfs::dirInodeC<fvfs::tagC> *dev,
			error_t *err)
		{
			return vfs::dirInodeC<fvfs::tagC>::createDirTag(
				name, type, parent, dev, err);
		}

		sarch_t removeDirTag(utf8Char *n)
			{ return vfs::dirInodeC<fvfs::tagC>::removeDirTag(n); }

		fvfs::tagC *getDirTag(utf8Char *name)
			{ return vfs::dirInodeC<fvfs::tagC>::getDirTag(name); }

		/* The leaf functions are not meant to be used at all by anyone;
		 * not even internals within this class, and they override the
		 * base-class functions of the same name, returning error
		 * unconditionally.
		 **/
		fvfs::tagC *createLeafTag(
			utf8Char *, vfs::tagTypeE, fvfs::tagC *,
			vfs::inodeC *, error_t *err)
		{
			*err = ERROR_UNIMPLEMENTED;
			return NULL;
		}

		sarch_t removeLeafTag(utf8Char *) { return 0; }
		fvfs::tagC *getLeafTag(utf8Char *) { return NULL; }

	public:
		utf8Char	longName[ZUDI_MESSAGE_MAXLEN],
				driverFullName[DRIVER_FULLNAME_MAXLEN];

		/* Vendor name and contact info should be retrieved from the
		 * driver object, instead of unnecessarily being duplicated.
		 **/
		numaBankId_t		bankId;
		driverInstanceC		*driverInstance;
		deviceInstanceC		*instance;
		ubit8			nEnumerationAttrs, nInstanceAttrs,
					nClasses, nParentTags;
		udi_instance_attr_list_t
					**enumerationAttrs, **instanceAttrs;
		utf8Char		(*classes)[DEVICE_CLASS_MAXLEN];
		sbit8			driverDetected;
		// The index which enumerated this device's driver.
		zudiIndexServer::indexE	driverIndex, requestedIndex;
		parentTagS		*parentTags;
		uarch_t			parentTagCounter;
	};

	/**	deviceInstanceC:
	 * Device-instance specific attributes.
	 **********************************************************************/
	class deviceInstanceC
	{
	public:
		deviceInstanceC(deviceC *dev)
		:
		nChannels(0),
		device(dev),
		regions(NULL), channels(NULL)
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
		struct regionS
		{
			ubit16			index;
			processId_t		tid;
			udi_init_context_t	*rdata;
		};

		struct channelS
		{
			channelS(
				threadC *t0, udi_ops_vector_t *opsVec0,
				udi_init_context_t *chanContext0,
				threadC *t1, udi_ops_vector_t *opsVec1,
				udi_init_context_t *chanContext1)
			{
				new (&endpoints[0]) endpointS(
					this, t0, opsVec0, chanContext0);

				new (&endpoints[1]) endpointS(
					this, t1, opsVec1, chanContext1);
			}

			struct endpointS
			{
				friend class fplainn::deviceInstanceC::channelS;
				// Private, void constructor that does nothing.
				endpointS(void) {}

			public:
				endpointS(
					channelS *parent, threadC *thread,
					udi_ops_vector_t *opsVector,
					udi_init_context_t *chanContext)
				:
				parent(parent), thread(thread),
				opsVector(opsVector),
				channelContext(chanContext)
				{}

				channelS		*parent;
				threadC			*thread;
				udi_ops_vector_t	*opsVector;
				udi_init_context_t	*channelContext;
			};

			struct incompleteChannelS
			{
				incompleteChannelS(void)
				:
				spawnIndex(-1), channel(NULL)
				{}

				ptrlessListC<incompleteChannelS>::headerS
					listHeader;

				udi_index_t	spawnIndex;
				channelS	*channel;
			};

			/**	EXPLANATION:
			 * This is a list of all incomplete spawn operations
			 * that are taking place across this channel. The
			 * spawn_idx is recorded until two udi_channel_spawn()
			 * operations are paired up. At that point, a channel
			 * spawn is completed and the spawn_idx is removed from
			 * this list.
			 **/
			ptrlessListC<incompleteChannelS> incompleteChannels;
			endpointS				endpoints[2];
		};

		error_t addChannel(channelS *newChan);
		void removeChannel(channelS *chan);

		void udi_usage_ind(udi_ubit8_t usageLevel);
		void udi_enumerate_req(udi_ubit8_t enumerateLevel);
		void udi_devmgmt_req(udi_ubit8_t op, udi_ubit8_t parentId);
		void udi_final_cleanup_req(void);

	public:
		ubit16			nChannels;
		deviceC			*device;
		regionS			*regions;
		channelS		**channels;
	};

	class driverInstanceC;

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

		driverC(zudiIndexServer::indexE index)
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

		~driverC(void){}

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
		driverInstanceC *getInstance(numaBankId_t bid);

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
			dataSize(0), flags(flags)
			{}

			void dump(void);

			// 'moduleIndex' is this region's module's index.
			ubit16		index, moduleIndex;
			// Array of channel indexes belonging to this region.
			uarch_t		dataSize;
			ubit32		flags;

		private:
			friend class fplainn::driverC;
			regionS(void)
			:
			index(0), moduleIndex(0),
			dataSize(0), flags(0)
			{}
		};

		struct requirementS
		{
			requirementS(utf8Char *name, ubit32 version)
			:
			fullName(NULL), version(version)
			{
				strcpy8(this->name, name);
			}

			void dump(void);

			utf8Char	name[ZUDI_DRIVER_REQUIREMENT_MAXLEN],
					*fullName;
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
			metalanguageS(
				ubit16 index, utf8Char *name,
				udi_mei_init_t *udi_meta_info)
			:
			index(index), udi_meta_info(udi_meta_info)
			{
				strcpy8(this->name, name);
			}

			void dump(void);

			ubit16		index;
			utf8Char	name[ZUDI_DRIVER_METALANGUAGE_MAXLEN];
			// XXX: Only used by the kernel for kernelspace drivers.
			const udi_mei_init_t	*udi_meta_info;

		private:
			friend class fplainn::driverC;
			metalanguageS(void)
			:
			index(0), udi_meta_info(NULL)
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
		metalanguageS *getMetalanguage(ubit16 index)
		{
			for (uarch_t i=0; i<nMetalanguages; i++)
			{
				if (metalanguages[i].index == index)
					{ return &metalanguages[i]; };
			};

			return NULL;
		}

		metalanguageS *getMetalanguage(utf8Char *name)
		{
			for (uarch_t i=0; i<nMetalanguages; i++)
			{
				if (!strcmp8(metalanguages[i].name, name))
					{ return &metalanguages[i]; };
			};

			return NULL;
		}

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

		childBopS *getChildBop(ubit16 metaIndex)
		{
			for (uarch_t i=0; i<nChildBops; i++)
			{
				if (childBops[i].metaIndex == metaIndex)
					{ return &childBops[i]; };
			};

			return NULL;
		}

		parentBopS *getParentBop(ubit16 metaIndex)
		{
			for (uarch_t i=0; i<nParentBops; i++)
			{
				if (parentBops[i].metaIndex == metaIndex)
					{ return &parentBops[i]; };
			};

			return NULL;
		}

		internalBopS *getInternalBop(ubit16 regionIndex)
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
		zudiIndexServer::indexE		index;

		utf8Char	basePath[ZUDI_DRIVER_BASEPATH_MAXLEN],
				shortName[ZUDI_DRIVER_SHORTNAME_MAXLEN],
				longName[ZUDI_MESSAGE_MAXLEN],
				supplier[ZUDI_MESSAGE_MAXLEN],
				supplierContact[ZUDI_MESSAGE_MAXLEN];
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
		driverInstanceC	*instances;

		sbit8		allRequirementsSatisfied;
		uarch_t		childEnumerationAttrSize;
		// Modules for this driver, and their indexes.
		moduleS		*modules;
		// Regions in this driver and their indexes/module indexes, etc.
		regionS		*regions;
		// All required libraries for this driver.
		requirementS	*requirements;
		// Metalanguage indexes, names, etc.
		metalanguageS	*metalanguages;
		childBopS	*childBops;
		parentBopS	*parentBops;
		internalBopS	*internalBops;
		utf8Char	(*classes)[DRIVER_CLASS_MAXLEN];

		// XXX: ONLY for use by libzbzcore, and ONLY in kernel-space.
		const udi_init_t	*driverInitInfo;
	};

	/**	driverInstanceC
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
	class driverInstanceC
	{
	public:
		driverInstanceC(void)
		:
		driver(NULL), bankId(NUMABANKID_INVALID), pid(PROCID_INVALID),
		childBopVectors(NULL),
		nHostedDevices(0), hostedDevices(NULL)
		{}

		driverInstanceC(
			driverC *driver, numaBankId_t bid, processId_t pid)
		:
		driver(driver), bankId(bid), pid(pid), childBopVectors(NULL),
		nHostedDevices(0), hostedDevices(NULL)
		{}

		error_t initialize(void);

	public:
		struct childBopS
		{
			/* Parent BOps can only be uniquely identified by their
			 * metalanguage indexes.
			 **/
			ubit16			metaIndex;
			udi_ops_vector_t	*opsVector;
		};

		struct managementChannelS
		{
			managementChannelS(void)
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

		childBopS *getChildBop(ubit16 metaIndex)
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
		driverC			*driver;
		numaBankId_t		bankId;
		processId_t		pid;
		childBopS		*childBopVectors;
		uarch_t			nHostedDevices;
		utf8Char		**hostedDevices;
		managementChannelS	managementChannel;
	};
}


/**	Inline methods.
 ******************************************************************************/

inline void fplainn::deviceInstanceC::setRegionInfo(
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

inline error_t fplainn::deviceInstanceC::getRegionInfo(
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

inline error_t fplainn::deviceInstanceC::getRegionInfo(
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

inline fplainn::driverInstanceC *fplainn::driverC::getInstance(numaBankId_t bid)
{
	for (uarch_t i=0; i<nInstances; i++)
	{
		if (instances[i].bankId == bid)
			{ return &instances[i]; };
	};

	return NULL;
}

#endif
