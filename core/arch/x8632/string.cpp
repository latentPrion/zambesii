
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


void *memset(void *_ptr, int value, size_t count)
{
	if (_ptr == __KNULL)
	{
		__kprintf(FATAL"memset: dest 0x%p, caller 0x%x.\n",
			_ptr, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit8*)_ptr)[count-1] = (ubit8)value;
	};
	return _ptr;
}

void *memcpy(void *dest, void *src, size_t count)
{
	if (dest == __KNULL || src == __KNULL)
	{
		__kprintf(FATAL"memcpy: dest 0x%p, src 0x%p, caller 0x%x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit8 *)dest)[count - 1] = ((ubit8 *)src)[count - 1];
	};
	return dest;
}

int strcmp(const char *str1, const char *str2)
{
	if (str1 == str2) { return 0; };

	if (str1 == __KNULL || str2 == __KNULL)
	{
		__kprintf(FATAL"strcmp: str1 0x%p, str2 0x%p, caller 0x%x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; (*str1 && *str2); str1++, str2++)
	{
		if (*str1 != *str2) {
			return ((*str1 > *str2) ? 1 : -1);
		};
	};
	if (*str1 != *str2) {
		return ((*str1 > *str2) ? 1 : -1);
	};

	return 0;
}

int strncmp(const char *str1, const char *str2, int count)
{
	if (str1 == str2) { return 0; };

	if (str1 == __KNULL || str2 == __KNULL)
	{
		__kprintf(FATAL"strncmp: str1 0x%p, str2 0x%p, caller 0x%x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	uarch_t		n=0;

	for (; count > 0; count--, n++)
	{
		if (!str1[n] || !str2[n]) { break; };
		if (str1[n] != str2[n]) {
			return ((str1[n] > str2[n]) ? 1 : -1);
		};
	};

	if (count)
	{
		if ((str1[n] && (!str2[n])) || ((!str1[n]) && str2[n])) {
			return (str1[n] > str2[n]) ? 1 : -1;
		};
	};

	return 0;
}

