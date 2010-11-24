#ifndef _PRIO_QUEUE_H
	#define _PRIO_QUEUE_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/slamCache.h>
	#include <__kclasses/cachePool.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/waitLock.h>

#define PRIOQUEUE_INSERT_AT_FRONT	(1<<0)

class prioQueueC
{
private:
	struct prioQueueNodeS;

public:
	prioQueueC(ubit16 nPriorities);
	error_t initialize(void);
	~prioQueueC(void);

public:
	error_t insert(void *item, ubit16 prio, ubit32 opt);
	void *pop(void);

private:
	prioQueueNodeS *getLastNodeIn(prioQueueNodeS *list, ubit16 prio);
	sbit16 getNextGreaterPrio(ubit16 prio);
	sbit16 getNextLesserPrio(ubit16 prio);

public:
	struct prioQueueHeaderS
	{
		ubit16		prio;
	};

private:
	struct prioQueueNodeS
	{
		void		*item;
		prioQueueNodeS	*next;
		ubit16		prio;
	};

	struct prioQueueStateS
	{
		prioQueueNodeS	*head;
		prioQueueNodeS	**prios;
	};

	slamCacheC	*nodeCache;
	ubit16		nPrios;
	sharedResourceGroupC<waitLockC, prioQueueStateS>	q;
};

#endif

