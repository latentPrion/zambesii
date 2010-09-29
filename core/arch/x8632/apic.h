#ifndef _x86_32_APIC_H
	#define _x86_32_APIC_H

/**	EXPLANATION:
 * Driver for the I/O and Local APICs of the x86 architecture.
 **/

#define x8632_LAPIC_PADDR_BASE		0x

namespace lapic
{
	// ERROR_SUCCESS = yes, anything else = no.
	error_t detect(void);
	
