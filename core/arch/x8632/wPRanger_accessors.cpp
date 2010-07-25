
#include <arch/x8632/wPRanger_accessors.h>

// Accessors.
pagingLevel1S		*level1Accessor =
	reinterpret_cast<pagingLevel1S *>( 0xFFFFF000 );

#ifdef CONFIG_ARCH_x86_32_PAE
pagingLevel2S		*level2Accessor =
	reinterpret_cast<pagingLevel2S *>( 0xFFFFE000 );
#endif

// Modifiers.
#ifdef CONFIG_ARCH_x86_32_PAE
paddr_t		*level1Modifier = reinterpret_cast<paddr_t *>(
	(0xFFFFD000 + (511 * sizeof(paddr_t))) );

paddr_t		*level2Modifier = reinterpret_cast<paddr_t *>(
	(0xFFFFD000 + (510 * sizeof(paddr_t))) );
#else
paddr_t		*level1Modifier = reinterpret_cast<paddr_t *>(
	(0xFFFFE000 + (1023 * sizeof(paddr_t))) );
#endif

