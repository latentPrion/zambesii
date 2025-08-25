#ifndef _MEMORY_H
	#define _MEMORY_H

//Virtual load base for the kernel. 3GB mark on x86-32.
#define ARCH_MEMORY___KLOAD_VADDR_BASE			0xC0000000
#define ARCH_MEMORY_VADDR_IS_IN___KVADDRSPACE(vaddr) \
	((((uintptr_t)(vaddr)) >= ARCH_MEMORY___KLOAD_VADDR_BASE) \
		&& (((uintptr_t)(vaddr)) < (ARCH_MEMORY___KLOAD_VADDR_BASE + \
		ARCH_MEMORY___KLOAD_VADDR_NBYTES)))
#define ARCH_MEMORY___KLOAD_VADDR_NBYTES		0x40000000
#define ARCH_MEMORY___KLOAD_VADDR_END			0xFFFFFFFF

#endif

