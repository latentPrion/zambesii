#ifndef _x86_32_IBMPC_CPU_H
	#define _x86_32_IBMPC_CPU_H

/**	EXPLANATION:
 * We use the memory space as follows:
 * CPU poweron static data (temporary GDT, etc) is placed within the 2 frames
 * beginning at 0xA000. The code for CPU poweron begins after it, at 0xC000.
 *
 * Remember that the x86 real mode emulator (x86Emu) uses the frame at 0x4000.
 **/
#define PLATFORM_x86_32_POWERON_ENTRY_PADDR		0xC000
#define PLATFORM_x86_32_POWERON_DATA_PADDR		0xA000
#define PLATFORM_x86_32_POWERON_VECTOR		\
	(PLATFORM_x86_32_POWERON_PADDR >> 12)
