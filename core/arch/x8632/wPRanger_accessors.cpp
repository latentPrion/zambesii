
#include <arch/x8632/wPRanger_accessors.h>

// Accessors.
volatile sPagingLevel1		*const level1Accessor =
	reinterpret_cast<sPagingLevel1 *>( 0xFFFFF000 );

#ifdef CONFIG_ARCH_x86_32_PAE
volatile sPagingLevel2		*const level2Accessor =
	reinterpret_cast<sPagingLevel2 *>( 0xFFFFE000 );
#endif

// Modifiers.
#ifdef CONFIG_ARCH_x86_32_PAE
paddr_t	*const level1Modifier = reinterpret_cast<paddr_t *>(
	(0xFFFFD000 + (511 * sizeof(paddr_t))) );

paddr_t	*const level2Modifier = reinterpret_cast<paddr_t *>(
	(0xFFFFD000 + (510 * sizeof(paddr_t))) );
#else
paddr_t	*const level1Modifier = reinterpret_cast<paddr_t *>(
	(0xFFFFE000 + (1023 * sizeof(paddr_t))) );
#endif

