
#include <arch/arch.h>
#include <__kstdlib/__kmath.h>


static const uarch_t		shiftTab[] =
{
	1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384,
	32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304,
	8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912,
	1073741824

	// For 64-bit build, we'll generate more numbers here.
};
	

ubit16 getShiftFor(uarch_t num)
{
	if (num == 0) {
		return __WORDSIZE - 1;
	};

	for (ubit16 i=0; i<__WORDSIZE - 1; i++)
	{
		if (shiftTab[i] == num) {
			return i;
		};
	};

	return 0;
}

