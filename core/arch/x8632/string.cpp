
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

#if 0
/**	EXPLANATION:
 * Do not use these functions on strings. Use the type-safe UTF-8 functions in
 * string8.cpp or string16.cpp for UTF-16.
 *
 * Do not use "char *" for strings in the kernel.
 **/
char *strcpy(char *dest, const char *src)
{
	uarch_t		i=0;

	for (; src[i]; i++) {
		dest[i] = src[i];
	};
	dest[i] = '\0';
	return dest;
}

char *strncpy(char *dest, const char *src, size_t count)
{
	for (; count && src[count - 1]; count--) {
		dest[count - 1] = src[count - 1];
	};
	for (; count; count--) {
		dest[count - 1] = '\0';
	}
	return dest;
}

size_t strlen(const char *str)
{
	uarch_t		len=0;

	for (; str[len]; len++) {};
	return len;
}
#endif

int strcmp(const char *str1, const char *str2)
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

int strncmp(const char *str1, const char *str2, int count)
{
	if (str1 == str2) { return 0; };

	if (str1 == __KNULL || str2 == __KNULL)
	{
		__kprintf(FATAL"strncmp8: str1 0x%p, str2 0x%p, caller 0x%x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count > 0 && (*str1) && (*str2); count--, str1++, str2++)
	{
		if (*str1 != *str2) {
			return ((*str1 > *str2) ? 1 : -1);
		};
	};

	if (count)
	{
		if (((*str1) && (!(*str2))) || ((!(*str1)) && (*str2))) {
			return ((*str1 > *str2) ? 1 : -1);
		};
	};

	return 0;
}

