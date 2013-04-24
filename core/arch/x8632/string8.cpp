
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


void *memset8(void *_ptr, int value, size_t count)
{
	if (_ptr == __KNULL)
	{
		__kprintf(FATAL"memset8: dest 0x%p, caller 0x%x.\n",
			_ptr, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit8*)_ptr)[count-1] = (ubit8)value;
	};
	return _ptr;
}

void *memcpy8(void *dest, void *src, size_t count)
{
	if (dest == __KNULL || src == __KNULL)
	{
		__kprintf(FATAL"memcpy8: dest 0x%p, src 0x%p, caller 0x%x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit8 *)dest)[count - 1] = ((ubit8 *)src)[count - 1];
	};
	return dest;
}

utf8Char *strcpy8(utf8Char *dest, const utf8Char *src)
{
	uarch_t		i=0;

	if (dest == __KNULL || src == __KNULL)
	{
		__kprintf(FATAL"strcpy8: dest 0x%p, src 0x%p, caller 0x%x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; src[i]; i++) {
		dest[i] = src[i];
	};

	dest[i] = '\0';
	return dest;
}

utf8Char *strncpy8(utf8Char *dest, const utf8Char *src, size_t count)
{
	if (dest == __KNULL || src == __KNULL)
	{
		__kprintf(FATAL"strncpy8: dest 0x%p, src 0x%p, caller 0x%x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	uarch_t		n=0;

	for (; count && src[n]; count--, n++) {
		dest[n] = src[n];
	};
	for (; count; count--, n++) {
		dest[n] = '\0';
	}

	return dest;
}

size_t strlen8(const utf8Char *str)
{
	uarch_t		len=0;

	if (str == __KNULL)
	{
		__kprintf(FATAL"strlen8: str 0x%p, caller 0x%x.\n",
			str, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; str[len]; len++) {};
	return len;
}

int strcmp8(const utf8Char *str1, const utf8Char *str2)
{
	if (str1 == str2) { return 0; };

	if (str1 == __KNULL || str2 == __KNULL)
	{
		__kprintf(FATAL"strcmp8: str1 0x%p, str2 0x%p, caller 0x%x.\n",
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

int strncmp8(const utf8Char *str1, const utf8Char *str2, int count)
{
	if (str1 == str2) { return 0; };

	if (str1 == __KNULL || str2 == __KNULL)
	{
		__kprintf(FATAL"strncmp8: str1 0x%p, str2 0x%p, caller 0x%x.\n",
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

