#ifndef _SCGTH_T_PADDR_T_OPERATORS_H
	#define _SCGTH_T_PADDR_T_OPERATORS_H

	#define UDI_VERSION 0x101
	#define UDI_PHYSIO_VERSION 0x101
	#include <udi.h>
	#include <udi_physio.h>
	#undef UDI_VERSION

/**
 * These inline operators make it easier to code assignments to
 * UDI's scatter-gather bignum types.
 ******************************************************************************/

inline int operator ==(paddr_t p, udi_scgth_element_32_t s)
{
	return (p == s.block_busaddr);
}

inline int operator ==(paddr_t p, udi_scgth_element_64_t s)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	struct s64BitInt
	{
		ubit32		low, high;
	} *e = (s64BitInt *)&s.block_busaddr;

	if (e->high != 0) { return 0; };
	return p == e->low;
#else
	paddr_t		*p2 = (paddr_t *)&s.block_busaddr;

	return p == *p2;
#endif
}

inline void assign_paddr_to_scgth_block_busaddr(udi_scgth_element_32_t *u32, paddr_t p)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	// No-pae-32bit-paddr being assigned to a 32-bit-block_busaddr.
	u32->block_busaddr = p.getLow();
#else
	/* Pae-64bit-paddr being assigned to a 32bit-block_busaddr.
	 *
	 * Requires us to cut off the high bits of the pae-64bit-paddr.
	 *
	 * In such a case, we would have clearly chosen to build a 32-bit
	 * SGList, so we should not be trying to add any frames with the high
	 * bits set.
	 **/
	if ((p >> 32).getLow() != 0)
	{
		panic("Trying to add a 64-bit paddr with high bits set, to a "
			"32-bit SGList.");
	};

	u32->block_busaddr = p.getLow();
#endif
}

inline void assign_paddr_to_scgth_block_busaddr(udi_scgth_element_64_t *u64, paddr_t p)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	/* No-pae-32bit-paddr being assigned to a 64-bit-block_busaddr.
	 *
	 * Just requires us to assign the paddr to the low 32 bits of the
	 * block_busaddr. We also clear the high bits to be pedantic.
	 **/
	struct s64BitInt
	{
		ubit32		low, high;
	} *e = (s64BitInt *)&u64->block_busaddr;

	e->high = 0;
	e->low = p.getLow();
#else
	// Pae-64bit-paddr being assigned to a 64-bit-block_busaddr.
	paddr_t		*p2 = (paddr_t *)&u64->block_busaddr;

	*p2 = p;
#endif
}

inline void assign_scgth_block_busaddr_to_paddr(paddr_t &p, udi_busaddr64_t u64)
{
#if __VADDR_NBITS__ == 32 && !defined(CONFIG_ARCH_x86_32_PAE)
	struct s64BitInt
	{
		ubit32		low, high;
	} *s = (s64BitInt *)&u64;

	if (s->high != 0) {
		panic(CC"High bits set in udi_busaddr64_t on non-PAE build.\n");
	};

	p = s->low;
#else
	paddr_t			*p2 = (paddr_t *)&u64;

	p = *p2;
#endif
}

inline void assign_scgth_block_busaddr_to_paddr(paddr_t &p, udi_ubit32_t u32)
{
	p = u32;
}

#endif
