#ifndef _NUMA_BANK_H
	#define _NUMA_BANK_H

	#include <arch/paddr_t.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/numaMemoryBank.h>
	#include <kernel/common/numaCpuBank.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * A NUMA Stream is technically an abstraction of an actual NUMA bank. It holds
 * the relevant information on the bank's memory ranges, and contained CPUs in
 * its member classes, numaMemoryBankC and numaCpuBankC.
 *
 * Much care has been taken to reduce the number of API functions which this
 * class exports to an absolute minimum. The idea is to offload specific APIs
 * to their relevant specific classes.
 **/

class numaStreamC
:
public streamC
{
public:
	numaStreamC(numaBankId_t bankId, paddr_t baseAddr, paddr_t size);
	error_t initialize(void *preAllocated=__KNULL);

	~numaStreamC(void);

public:
	/* These will pull down the whole bank. Use the individual cut() and
	 * bind() methods on the bank classes to control the individual parts
	 * of a bank.
	 **/
	void cut(void);
	void bind(void);

public:
	numaBankId_t		bankId;
	numaMemoryBankC		memoryBank;
	// Numa Cpu Bank may have to hold per bank task scheduling.
	numaCpuBankC		cpuBank;
};

#endif

