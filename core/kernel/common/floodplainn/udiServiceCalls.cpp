
#define UDI_VERSION	0x101
#include <udi.h>
#include <stdarg.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/memory>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>
#include <kernel/common/process.h>
#include <kernel/common/floodplainn/floodplainn.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>


void __udi_assert(const char *expr, const char *file, int line)
{
	printf(FATAL"udi_assert failure: line %d of %s; expr '%s'.\n",
		line, file, expr);

	assert_fatal(0);
}

error_t fplainn::Zudi::udi_channel_spawn(
	udi_channel_spawn_call_t *cb, udi_cb_t *gcb,
	udi_channel_t channel_endp, udi_index_t spawn_idx,
	udi_ops_vector_t *ops_vector,
	udi_init_context_t *channel_context
	)
{
	error_t					ret;
	Thread					*self;
	fplainn::Region				*callerRegion;
	fplainn::Channel			*originChannel;
	fplainn::IncompleteChannel		*incChan;
	sbit8					emptyEndpointIdx;

	(void)cb; (void)gcb;
	/**	EXPLANATION:
	 * Only drivers are allowed to call this.
	 *
	 * We need to get the device instance for the caller, and:
	 *	* Check to see if a channel endpoint matching the pointer passed
	 * 	  to us exists.
	 * 	* Take the channel which is the parent of the passed endpoint,
	 *	  and check to see if it has an incompleteChannel with the
	 *	  spawn_idx passed to us here.
	 * 		* If an incompleteChannel with our spawn_idx value
	 *		  doesn't exist, create a new incompleteChannel and
	 *		  then anchor() it.
	 *		* If one exists, anchor the new end, then remove it
	 *		  from the incompleteChannel list, and create a new
	 *		  fully fledged channel based on it.
	 **/
	self = cpuTrib.getCurrentCpuStream()->taskStream.getCurrentThread();

	if (self->parent->getType() != ProcessStream::DRIVER)
		{ return ERROR_UNAUTHORIZED; };

	// Set by fplainn::DeviceInstance::setThreadRegionPointer().
	callerRegion = self->getRegion();
	// Make sure the "channel" endpoint passed to us is really an endpoint.
	originChannel = callerRegion->parent->getChannelByEndpoint(channel_endp);
	if (originChannel == NULL) { return ERROR_INVALID_ARG_VAL; };

	// Is there already a spawn in progress with this spawn_idx?
	incChan = originChannel->getIncompleteChannelBySpawnIndex(spawn_idx);
	if (incChan == NULL)
	{
		/* No incomplete channel with our spawn_idx exists. Create a
		 * new incomplete channel and anchor one of its endpoints.
		 *
		 * Endpoint0 is always a region endpoint. Since this is
		 **/
		ret = originChannel->createIncompleteChannel(
			originChannel->getType(), spawn_idx, &incChan);


		/* We can use originChannel to determine what type of incomplete
		 * channel (D2D or D2S) we have spawned because it is not
		 * possible for a D2D channel to be used to spawn anything but
		 * another D2D channel, and vice versa: a D2S channel can only
		 * be used to spawn another D2S channel.
		 **/
		incChan->cast2BaseChannel(originChannel->getType())
			->endpoints[0]->anchor(
				callerRegion, ops_vector, channel_context);

		incChan->setOrigin(0, channel_endp);
		return ERROR_SUCCESS;
	};

	/* Check to ensure that the same caller doesn't call spawn() twice and
	 * set up a channel to itself like a retard. If spawn() is called twice
	 * with the same originating endpoint, we cause the second call to
	 * overwrite the attributes that were set by the first.
	 **/
	for (uarch_t i=0; i<2; i++)
	{
		if (incChan->originEndpoints[i] == channel_endp)
		{
			// If it's a repeat call, just re-anchor (overwrite).
			incChan->cast2BaseChannel(originChannel->getType())
				->endpoints[i]->anchor(
					callerRegion, ops_vector,
					channel_context);

			return ERROR_SUCCESS;
		};

		// Prepare for below.
		if (incChan->originEndpoints[i] == NULL) {
			emptyEndpointIdx = i;
		};
	};

	// Else anchor the other end.
	incChan->cast2BaseChannel(originChannel->getType())
		->endpoints[emptyEndpointIdx]->anchor(
			callerRegion, ops_vector, channel_context);

	/* Now, if both ends have been claimed by calls to udi_channel_spawn()
	 * such that both ends have valid "parentEndpoint"s, we can proceed
	 * to create a new channel based on the incomplete channel.
	 **/
	if (incChan->originEndpoints[0] == NULL
		|| incChan->originEndpoints[1] == NULL)
	{
		panic(
			ERROR_FATAL,
			FATAL ZUDI"udi_ch_spawn: originating endpoints are "
			"NULL at point where that should not be possible.\n");
	};

	// If we're here, then both ends have been claimed and matched.
	if (originChannel->getType() == fplainn::Channel::TYPE_D2D)
	{
		fplainn::D2DChannel		*dummy;

		ret = createChannel(
			static_cast<IncompleteD2DChannel *>(incChan), &dummy);
	}
	else
	{
		fplainn::D2SChannel		*dummy;

		ret = FloodplainnStream::createChannel(
			static_cast<IncompleteD2SChannel *>(incChan), &dummy);
	};

	if (ret != ERROR_SUCCESS) { return ret; };

	// Next, remove the incompleteChannel from the originating channel.
	originChannel->destroyIncompleteChannel(
		originChannel->getType(), incChan->spawnIndex);

	return ERROR_SUCCESS;
}
