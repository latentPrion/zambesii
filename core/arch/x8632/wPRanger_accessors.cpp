
#include <arch/x8632/wPRanger_accessors.h>

// Accessors.
volatile pagingLevel1S		*const level1Accessor =
	reinterpret_cast<pagingLevel1S *>( 0xFFFFF000 );

#ifdef CONFIG_ARCH_x86_32_PAE
volatile pagingLevel2S		*const level2Accessor =
	reinterpret_cast<pagingLevel2S *>( 0xFFFFE000 );
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

