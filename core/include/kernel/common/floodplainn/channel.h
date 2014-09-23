#ifndef _FLOODPLAINN_UDI_CHANNEL_H
	#define _FLOODPLAINN_UDI_CHANNEL_H

	#define UDI_VERSION	0x101
	#include <udi.h>
	#undef UDI_VERSION
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/ptrList.h>
	#include <__kclasses/ptrlessList.h>
	#include <__kclasses/debugPipe.h>
	#include <kernel/common/messageStream.h>
	#include <kernel/common/floodplainn/region.h>

class Thread;
class FloodplainnStream;
namespace fplainn
{

	struct sChannelMsg;
	class Channel;

	/**	class Endpoint
	 **********************************************************************/
	class Endpoint
	{
		friend class FStreamEndpoint;
		friend class RegionEndpoint;

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
			udi_init_context_t *channel_context)
		{
			opsVector = ops_vector;
			channelContext = channel_context;
		}

		virtual sbit8 isAnchored(void)
			{ return opsVector != NULL && channelContext != NULL; }

		/* This version of anchor() calls the entity that the endpoint
		 * is connected to, and asks it to add the endpoint to its
		 * list.
		 **/
		virtual error_t anchor(void)=0;

		// send() calls enqueue() on the "otherEnd" endpoint.
		virtual error_t send(sChannelMsg *msg)
			{ return otherEnd->enqueue(msg); }

		// enqueue() is called by opposite end to enqueue on this end.
		virtual error_t enqueue(sChannelMsg *)=0;

		virtual void dump(void)
			{ printf(CC"\topsvec 0x%p, ", opsVector); }

	private:
		friend class D2DChannel;
		friend class D2SChannel;

	public:	Channel			*parent;
	private:Endpoint		*otherEnd;
		udi_ops_vector_t	*opsVector;
		udi_init_context_t	*channelContext;
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
			udi_init_context_t *channel_context)
		{
			Endpoint::anchor(object, ops_vector, channel_context);
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
			udi_init_context_t *channel_context)
		{
			Endpoint::anchor(object, ops_vector, channel_context);
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


		Channel(Endpoint *otherEnd)
		: regionEnd0(this, otherEnd)
		{
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
			udi_index_t spawn_idx,
			IncompleteChannel **ret);

		sbit8 destroyIncompleteChannel(
			typeE type, udi_index_t spawn_idx);

		sbit8 hasEndpoint(void *endp)
		{
			if (endpoints[0] == endp || endpoints[1] == endp)
				{ return 1; }
			else { return 0; };
		}

		virtual typeE getType(void)=0;

	public:
		RegionEndpoint			regionEnd0;
		Endpoint			*endpoints[2];
		PtrList<IncompleteChannel>	incompleteChannels;
	};

	class IncompleteD2DChannel;

	/**	class D2SChannel.
	 * Represents the link between two driver regions.
	 **********************************************************************/
	class D2DChannel
	: public Channel
	{
	public:
		D2DChannel(void)
		: Channel(&regionEnd1),
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
		D2SChannel(void)
		: Channel(&fstreamEnd),
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
	 * 		IncompleteD2DChannel, IncompleteD2SChannel
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
		IncompleteD2DChannel(udi_index_t spawn_idx)
		: IncompleteChannel(spawn_idx)
		{}
	};

	class IncompleteD2SChannel
	: public D2SChannel, public IncompleteChannel
	{
	public:
		IncompleteD2SChannel(udi_index_t spawn_idx)
		: IncompleteChannel(spawn_idx)
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
		data((ubit8 *)&this[1])
		{}

		void set(ubit16 dataSize)
			{ header.size = sizeof(*this) + dataSize; }

		uarch_t getDataSize(void)
			{ return header.size - sizeof(*this); }


		// Custom allocator to facilitate the extra room at the end.
		void *operator new(size_t sz, uarch_t dataSize);

		MessageStream::sHeader		header;
		// "data" is initialized to point to the byte after this struct.
		ubit8				*data;

	private:
		friend class Zudi;
		friend class ::FloodplainnStream;

		static error_t send(
			Endpoint *endp,
			udi_cb_t *cb, uarch_t size, void *privateData);
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
