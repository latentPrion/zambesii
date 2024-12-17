#ifndef _FLOODPLAINN_UDI_CHANNEL_H
	#define _FLOODPLAINN_UDI_CHANNEL_H

	#define UDI_VERSION		0x101
	#include <udi.h>
	#define UDI_PHYSIO_VERSION	0x101
	#include <udi_physio.h>
	#undef UDI_PHYSIO_VERSION
	#undef UDI_VERSION
	#include <zui.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kstdlib/__kmath.h>
	#include <__kclasses/heapList.h>
	#include <__kclasses/list.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/region.h>

class Thread;
class FloodplainnStream;
namespace fplainn
{
	class Zudi;
	struct sChannelMsg;
	class Channel;

	/**	class Endpoint
	 **********************************************************************/
	class Endpoint
	{
		friend class FStreamEndpoint;
		friend class RegionEndpoint;
		friend class Zudi;

		// Deliberately left private.
		Endpoint(Channel *parent, Endpoint *otherEnd)
		:
		parent(parent), otherEnd(otherEnd)
		{}

	public:
		virtual ~Endpoint(void) {}

		union anchorTargetU
		{
			anchorTargetU(Thread *t) : thread(t) {}
			anchorTargetU(Region *r) : region(r) {}
			Thread		*thread;
			Region		*region;
		};

		virtual void anchor(
			anchorTargetU, udi_ops_vector_t *ops_vector,
			void *privateData)
		{
			opsVector = ops_vector;
			this->privateData = privateData;
		}

		virtual sbit8 isAnchored(void) { return opsVector != NULL; }

		/* This version of anchor() calls the entity that the endpoint
		 * is connected to, and asks it to add the endpoint to its
		 * list.
		 **/
		virtual error_t anchor(void)=0;

		// send() calls enqueue() on the "otherEnd" endpoint.
		virtual error_t send(sChannelMsg *msg)
			{ return otherEnd->enqueue(msg); }

		virtual void dump(void)
			{ printf(CC"\topsvec %p, ", opsVector); }

	private:
		friend class D2DChannel;
		friend class D2SChannel;

	protected:
		// enqueue() is called by opposite end to enqueue on this end.
		virtual error_t enqueue(sChannelMsg *)=0;

	public:	Channel			*parent;
	private:Endpoint		*otherEnd;
		udi_ops_vector_t	*opsVector;
		void			*privateData;
	};

	/**	class FStreamEndpoint
	 **********************************************************************/
	class FStreamEndpoint
	: public Endpoint
	{
	public:
		FStreamEndpoint(Channel *parent, Endpoint *otherEnd)
		: Endpoint(parent, otherEnd),
		thread(NULL)
		{}

		virtual void anchor(
			anchorTargetU object, udi_ops_vector_t *ops_vector,
			void *privateData)
		{
			Endpoint::anchor(object, ops_vector, privateData);
			thread = object.thread;
		}

		virtual sbit8 isAnchored(void)
			{ return Endpoint::isAnchored() && thread != NULL; }

		virtual error_t anchor(void);

		virtual error_t enqueue(sChannelMsg *);

		virtual void dump(void);

	public:
		Thread		*thread;
	};

	/**	class RegionEndpoint
	 **********************************************************************/
	class RegionEndpoint
	: public Endpoint
	{
	public:
		RegionEndpoint(Channel *parent, Endpoint *otherEnd)
		: Endpoint(parent, otherEnd),
		region(NULL)
		{}

		virtual void anchor(
			anchorTargetU object, udi_ops_vector_t *ops_vector,
			void *privateData)
		{
			Endpoint::anchor(object, ops_vector, privateData);
			region = object.region;
		}

		virtual sbit8 isAnchored(void)
			{ return Endpoint::isAnchored() && region != NULL; }

		virtual error_t anchor(void);

		virtual error_t enqueue(sChannelMsg *);

		virtual void dump(void);

	public:
		Region		*region;
	};

	class IncompleteChannel;

	/**	class Channel
	 * Base class for all channels, implementing the fundamentals necessary
	 * for polymorphism.
	 **********************************************************************/
	class Channel
	{
		friend class D2DChannel;
		friend class D2SChannel;

	public:	enum bindChannelTypeE {
			// None means it's not a bind channel.
			BIND_CHANNEL_TYPE_NONE=0,
			BIND_CHANNEL_TYPE_INTERNAL, BIND_CHANNEL_TYPE_CHILD };

	private:
		Channel(
			utf8Char *metaName, bindChannelTypeE bct,
			Endpoint *otherEnd)
		: bindChannelType(bct), regionEnd0(this, otherEnd),
		parentId(0)
		{
			this->metaName[0] = '\0';
			if (metaName != NULL)
			{
				strncpy8(
					this->metaName, metaName,
					ZUI_DRIVER_METALANGUAGE_MAXLEN);

				this->metaName[
					ZUI_DRIVER_METALANGUAGE_MAXLEN - 1] =
					'\0';
			};

			endpoints[0] = &regionEnd0;
			endpoints[1] = NULL;
		}
	public:
		enum typeE { TYPE_D2D, TYPE_D2S };

		virtual ~Channel(void) {}

		error_t initialize(void)
			{ return incompleteChannels.initialize(); }

		void dump(void)
		{
			printf(NOTICE"%s channel, %d incchans.\n",
				((getType() == TYPE_D2D) ? "D2D" : "D2S"),
				incompleteChannels.getNItems());

			for (uarch_t i=0; i<2; i++) { endpoints[i]->dump(); };
		}

	public:
		IncompleteChannel *getIncompleteChannelBySpawnIndex(
			udi_index_t spawn_idx);

		error_t createIncompleteChannel(
			typeE type,
			utf8Char *metaName, bindChannelTypeE bct,
			udi_index_t spawn_idx,
			IncompleteChannel **ret);

		// Not a misnaming: does not free, only removes.
		sbit8 removeIncompleteChannel(
			typeE type, udi_index_t spawn_idx);

		sbit8 hasEndpoint(Endpoint *endp)
		{
			if (endpoints[0] == endp || endpoints[1] == endp)
				{ return 1; }
			else { return 0; };
		}

		virtual typeE getType(void)=0;

		// Custom operator delete().
		void operator delete(void *obj);

	public:
		bindChannelTypeE		bindChannelType;
		RegionEndpoint			regionEnd0;
		Endpoint			*endpoints[2];
		HeapList<IncompleteChannel>	incompleteChannels;
		utf8Char			metaName[
			ZUI_DRIVER_METALANGUAGE_MAXLEN];
		/* Only relevant for ParentBind channels:
		 *
		 * This is the ID of the parent, as known to the
		 * child. I.e, the ID that the child uses to refe
		 * to that parent.
		 */
		ubit16				parentId;
	};

	class IncompleteD2DChannel;

	/**	class D2SChannel.
	 * Represents the link between two driver regions.
	 **********************************************************************/
	class D2DChannel
	: public Channel
	{
	public:
		D2DChannel(utf8Char *metaName, bindChannelTypeE bct)
		: Channel(metaName, bct, &regionEnd1),
		regionEnd1(this, &regionEnd0)
		{
			endpoints[1] = &regionEnd1;
		}

		D2DChannel(IncompleteD2DChannel &ic);

		virtual typeE getType(void) { return TYPE_D2D; }

	public:
		RegionEndpoint		regionEnd1;
	};

	class IncompleteD2SChannel;

	/**	class D2SChannel.
	 * Represents a link between a driver region and a thread in a regular,
	 * non-driver process.
	 **********************************************************************/
	class D2SChannel
	: public Channel
	{
	public:
		D2SChannel(utf8Char *metaName, bindChannelTypeE bct)
		: Channel(metaName, bct, &fstreamEnd),
		fstreamEnd(this, &regionEnd0)
		{
			endpoints[1] = &fstreamEnd;
		}

		D2SChannel(IncompleteD2SChannel &ic);

		virtual typeE getType(void) { return TYPE_D2S; }

	public:
		FStreamEndpoint			fstreamEnd;
	};

	/**	classes	IncompleteChannel,
	 *		IncompleteD2DChannel, IncompleteD2SChannel
	 *
	 * These are used only as placeholder classes. When two matching spawn
	 * calls have been received, then "turn" these into normal channels.
	 *
	 * Technically, it should be possible to do that without any kind of
	 * reallocation, since we can just ignore the IncompleteChannel base
	 * portion.
	 **********************************************************************/
	class IncompleteChannel
	{
		friend class IncompleteD2DChannel;
		friend class IncompleteD2SChannel;

		IncompleteChannel(udi_index_t spawn_idx)
		:
		spawnIndex(spawn_idx)
		{
			originEndpoints[0] = originEndpoints[1] = NULL;
		}

	public:
		Channel *cast2BaseChannel(Channel::typeE type);

		void setOrigin(ubit8 idx, udi_channel_t origin)
			{ originEndpoints[idx] = origin; }

	public:
		udi_index_t		spawnIndex;
		udi_channel_t		originEndpoints[2];
	};

	class IncompleteD2DChannel
	: public D2DChannel, public IncompleteChannel
	{
	public:
		IncompleteD2DChannel(
			utf8Char *metaName, bindChannelTypeE bct,
			udi_index_t spawn_idx)
		: D2DChannel(metaName, bct), IncompleteChannel(spawn_idx)
		{}
	};

	class IncompleteD2SChannel
	: public D2SChannel, public IncompleteChannel
	{
	public:
		IncompleteD2SChannel(
			utf8Char *metaName, bindChannelTypeE bct,
			udi_index_t spawn_idx)
		: D2SChannel(metaName, bct), IncompleteChannel(spawn_idx)
		{}
	};

	struct sChannelMsg
	{
		sChannelMsg(
			processId_t targetId,
			ubit16 subsystem, ubit16 function,
			uarch_t size, uarch_t flags, void *privateData)
		:
		header(targetId, subsystem, function, size, flags, privateData),
		msgDataOff((ubit8 *)(this + 1) - (ubit8 *)this),
		dtypedLayoutNElements(0), msgDtypedLayoutOff(0)
		{
			metaName[0] = '\0';
		}

		void setPayloadSize(ubit16 dataSize)
			{ header.size = sizeof(*this) + dataSize; }

		void set(utf8Char *metaName, ubit16 metaOpsNum, ubit16 opsIndex)
		{
			this->metaOpsNum = metaOpsNum;
			this->opsIndex = opsIndex;

			if (metaName != NULL)
			{
				strncpy8(
					this->metaName, metaName,
					ZUI_DRIVER_METALANGUAGE_MAXLEN);

				this->metaName[ZUI_DRIVER_METALANGUAGE_MAXLEN - 1]
					= '\0';
			};
		}

		udi_cb_t *getPayloadAddr(void)
			{ return (udi_cb_t *)((ubit8 *)this + msgDataOff); }

		udi_layout_t *getDtypedLayoutAddr(void)
		{
			if (msgDtypedLayoutOff == 0) { return NULL; };
			return (udi_layout_t *)
				((ubit8 *)this + msgDtypedLayoutOff);
		}

		uarch_t getDataSize(void)
			{ return header.size - sizeof(*this); }

		// Custom allocator to facilitate the extra room at the end.
		void *operator new(size_t sz, uarch_t dataSize);

		MessageStream::sHeader		header;
		// "data" is initialized to point to the byte after this struct.
		ptrdiff_t			msgDataOff;
		// These 3 are set by Zudi::send()/FloodplainnStream::send().
		utf8Char			metaName[
			ZUI_DRIVER_METALANGUAGE_MAXLEN];
		ubit16				metaOpsNum,
						opsIndex;
		// These 3 are set by the target endpoint.
		fplainn::Endpoint		*__kendpoint;
		udi_ops_vector_t		*opsVector;
		void				*endpointPrivateData;
		ubit16				dtypedLayoutNElements;
		ptrdiff_t			msgDtypedLayoutOff;

	public:
		friend class Zudi;
		friend class ::FloodplainnStream;

		static sbit8 isInlineElement(udi_layout_t elem)
		{
			switch (elem)
			{
			case UDI_DL_INLINE_TYPED:
			case UDI_DL_INLINE_DRIVER_TYPED:
			case UDI_DL_MOVABLE_UNTYPED:
			case UDI_DL_MOVABLE_TYPED:
				return 1;
			default:
				/* UDI_DL_INLINE_UNTYPED is the same as void*.
				 * We can't do anything with it at all.
				 **/
				return 0;
			};
		}

		static sbit8 hasInlineElements(udi_layout_t *layout)
		{
			for (udi_layout_t *curr=layout;
				curr != NULL && *curr != UDI_DL_END;
				curr++)
			{
				if (isInlineElement(*curr)) { return 1; };
			};

			return 0;
		}

		static sbit8 zudi_layout_element_is_valid(udi_layout_t element)
		{
			/**	TODO:
			 * Implement this properly. Right now it is very
			 * generalized.
			 **/
			return element == UDI_DL_DMA_CONSTRAINTS_T
				|| element == UDI_DL_PIO_HANDLE_T
				|| (element > 0 && element <= 52);
		}

		// Signals error by setting skipCount to 0.
		static udi_size_t zudi_layout_get_element_size(
			const udi_layout_t element,
			const udi_layout_t nestedLayout[],
			udi_size_t *skipCount);

		static status_t zudi_layout_get_size(
			const udi_layout_t layout[],
			sbit8 inlineElementsAllowed)
		{
			udi_layout_t		*curr;
			udi_size_t		ret=0, skipCount;

			if (layout == NULL) { return ERROR_INVALID_ARG; };

			if (!inlineElementsAllowed)
			{
				if (hasInlineElements(layout))
					{ return ERROR_INVALID_FORMAT; };
			};

			for (curr=layout; *curr != UDI_DL_END; curr+=skipCount)
			{
				udi_size_t		currElemSize;

				currElemSize = zudi_layout_get_element_size(
					*curr, curr + 1, &skipCount);

				if (currElemSize == 0 || skipCount == 0)
					{ return ERROR_INVALID_FORMAT; };

				// Align "ret" before continuing.
				ret = align(ret, currElemSize);
				ret += currElemSize;
			};

			return align(ret, sizeof(void*));
		};

		static status_t getTotalMarshalSpaceInlineRequirements(
			udi_layout_t *layout, udi_layout_t *drvTypedLayout,
			udi_cb_t *srcCb);

		static status_t marshalStackArguments(
			ubit8 *dest, va_list src, udi_layout_t *layout);

		static status_t marshalInlineObjects(
			ubit8 *dest, udi_cb_t *_destCb, udi_cb_t *_srcCb,
			udi_layout_t *layout, udi_layout_t *drvTypedLayout);

		static status_t getTotalCbInlineRequirements(
			udi_layout_t *layout, udi_layout_t *drvTypedLayout);

		static error_t initializeCbInlinePointers(
			udi_cb_t *cb, ubit8 *inlineSpace,
			udi_layout_t *layout, udi_layout_t *drvTypedLayout);

		static status_t updateClonedCbInlinePointers(
			udi_cb_t *cb, udi_cb_t *marshalledCb,
			udi_layout_t *layout, udi_layout_t *marshalLayout);

		static void dumpLayout(udi_layout_t *lay)
		{
			uarch_t		nest=0;
			printf(NOTICE"layout: ");
			for (udi_layout_t *curr=lay;
				curr != NULL && (*curr != UDI_DL_END || nest > 0);
				curr++)
			{
				printf(CC"%d ", *curr);
				switch (*curr)
				{
				case UDI_DL_MOVABLE_TYPED:
				case UDI_DL_INLINE_TYPED:
					printf(CC"(nest) "); nest++; break;
				case UDI_DL_INLINE_DRIVER_TYPED:
					printf(CC"(drvtyped) "); break;
				case UDI_DL_MOVABLE_UNTYPED:
					printf(CC"(heapobj) "); break;
				case UDI_DL_ARRAY:
					printf(CC"(arr) "); nest++; break;
				case UDI_DL_BUF:
					printf(CC"(buf) "); break;
				};

				if (*curr == UDI_DL_END) { nest--; };
			};

			printf(CC"\n");
		}

		static status_t getLayoutNElements(
			udi_layout_t *lay, sbit8 inlineElementsAllowed)
		{
			uarch_t		nest=0;
			status_t	ret=1;

			if (lay == NULL) { return 0; };

			for (udi_layout_t *curr=lay;
				curr != NULL
					&& (*curr != UDI_DL_END || nest > 0);
				curr++, ret++)
			{
				if (!inlineElementsAllowed)
				{
					switch (*curr)
					{
					case UDI_DL_MOVABLE_TYPED:
					case UDI_DL_INLINE_TYPED:
					case UDI_DL_INLINE_DRIVER_TYPED:
					case UDI_DL_MOVABLE_UNTYPED:
						return ERROR_NON_CONFORMANT;
					};
				};

				switch (*curr)
				{
				case UDI_DL_MOVABLE_TYPED:
				case UDI_DL_INLINE_TYPED:
				case UDI_DL_ARRAY:
					nest++; break;
				};

				if (*curr == UDI_DL_END) { nest--; };
			};

			return ret;
		}

		static error_t send(
			fplainn::Endpoint *endp,
			udi_cb_t *gcb, va_list args, udi_layout_t *layouts[3],
			utf8Char *metaName,
			udi_index_t meta_ops_num, udi_index_t ops_idx,
			void *privateData);
	};
}


/**	Inline methods.
 ******************************************************************************/

inline fplainn::Channel *fplainn::IncompleteChannel::cast2BaseChannel(
	Channel::typeE type
	)
{
	IncompleteD2DChannel		*dtmp;
	IncompleteD2SChannel		*stmp;
	Channel			*ret;

	if (type == Channel::TYPE_D2D)
	{
		dtmp = static_cast<IncompleteD2DChannel *>(this);
		ret = dtmp;
	}
	else
	{
		stmp = static_cast<IncompleteD2SChannel *>(this);
		ret = stmp;
	};

	return ret;
}

#endif
