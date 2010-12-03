#ifndef _NUMA_BANK_H
	#define _NUMA_BANK_H

	#include <arch/paddr_t.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/stream.h>
	#include <kernel/common/numaTypes.h>

/**	EXPLANATION:
 * A NUMA stream is an meta data repository for the status of banks on a NUMA
 * node. Each NUMA stream holds a bitmap of CPUs and memory banks that exist on
 * a particular NUMA bank.
 **/

#define NUMASTREAM	"NUMA Stream: "

class numaStreamC
:
public streamC
{
public:
	numaStreamC(numaBankId_t bankId);
	error_t initialize(uarch_t nCpuBits);
	~numaStreamC(void);

	void dump(void);

public:
	/* These will pull down the whole bank. Use the individual cut() and
	 * bind() methods on the bank classes to control the individual parts
	 * of a bank.
	 *
	 *	NOTES:
	 * This is likely to be discarded soon.
	 **/
	void cut(void);
	void bind(void);

public:
	// ID of this stream's bank.
	numaBankId_t		bankId;
	// bitmaps for the CPUs and memory banks on this bank.
	bitmapC			cpus, memBanks;
};

#endif

