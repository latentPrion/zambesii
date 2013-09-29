
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


void *memset8(void *_ptr, int value, size_t count)
{
	if (_ptr == NULL)
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
	if (dest == NULL || src == NULL)
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

	if (dest == NULL || src == NULL)
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
	if (dest == NULL || src == NULL)
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

	if (str == NULL)
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

	if (str1 == NULL || str2 == NULL)
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

	if (str1 == NULL || str2 == NULL)
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

size_t strnlen8(const utf8Char *str1, size_t maxLen)
{
	size_t		len;

	if (str1 == NULL)
	{
		__kprintf(FATAL"strnlen8: str1 0x%p, caller 0x%x.\n",
			str1, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (len=0; len < maxLen && str1[len] != '\0'; len++) {};
	return len;
}

utf8Char *strnchr8(const utf8Char *str, size_t n, const utf8Char chr)
{
	uarch_t		i=0;

	for (; n > 0 && str[i] != '\0'; n--, i++)
	{
		if (str[i] == chr) { return (utf8Char *)&str[i]; };
	};

	/* "The terminating NULL is included in the search, so strnchr can be
	 * used to obtain a pointer to the end of the string.
	 **/
	if (chr == '\0') { return (utf8Char *)&str[i]; };
	return NULL;
}

