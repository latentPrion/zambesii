
#include <__kstdlib/__kclib/string16.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


void *memset16(void *_ptr, int value, size_t count)
{
	if (_ptr == NULL)
	{
		printf(FATAL"memset16: dest %p, caller %x.\n",
			_ptr, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit16*)_ptr)[count-1] = (ubit16)value;
	};
	return _ptr;
}

void *memcpy16(void *dest, void *src, size_t count)
{
	if (dest == NULL || src == NULL)
	{
		printf(FATAL"memcpy16: dest %p, src %p, caller %x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit16 *)dest)[count - 1] = ((ubit16 *)src)[count - 1];
	};
	return dest;
}

utf16Char *strcpy16(utf16Char *dest, const utf16Char *src)
{
	uarch_t		i=0;

	if (dest == NULL || src == NULL)
	{
		printf(FATAL"strcpy16: dest %p, src %p, caller %x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; src[i]; i++) {
		dest[i] = src[i];
	};
	dest[i] = '\0';
	return dest;
}

utf16Char *strncpy16(utf16Char *dest, const utf16Char *src, size_t count)
{
	if (dest == NULL || src == NULL)
	{
		printf(FATAL"strncpy16: dest %p, src %p, caller %x.\n",
			dest, src, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count && src[count - 1]; count--) {
		dest[count - 1] = src[count - 1];
	};
	for (; count; count--) {
		dest[count - 1] = '\0';
	}
	return dest;
}

size_t strlen16(const utf16Char *str)
{
	uarch_t		len=0;

	if (str == NULL)
	{
		printf(FATAL"strlen16: str %p, caller %x.\n",
			str, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; str[len]; len++) {};
	return len;
}

int strcmp16(const utf16Char *str1, const utf16Char *str2)
{
	if (str1 == str2) { return 0; };

	if (str1 == NULL || str2 == NULL)
	{
		printf(FATAL"strcmp16: str1 %p, str2 %p, caller %x.\n",
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

int strncmp16(const utf16Char *str1, const utf16Char *str2, int count)
{
	if (str1 == str2) { return 0; };

	if (str1 == NULL || str2 == NULL)
	{
		printf(FATAL"strcpy16: str1 %p, str2 %p, caller %x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count > 0; count--, str1++, str2++)
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

size_t strnlen16(const utf16Char *str1, size_t maxLen)
{
	size_t		len;

	if (str1 == NULL)
	{
		printf(FATAL"strnlen16: str1 %p, caller %x.\n",
			str1, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (len=0; len < maxLen && str1[len] != '\0'; len++) {};
	return len;
}

utf16Char *strnchr16(const utf16Char *str, size_t n, const utf16Char chr)
{
	uarch_t		i=0;

	for (; n > 0 && str[i] != '\0'; n--, i++)
	{
		if (str[i] == chr) { return (utf16Char *)&str[i]; };
	};

	/* "The terminating NULL is included in the search, so strnchr can be
	 * used to obtain a pointer to the end of the string.
	 **/
	if (chr == '\0') { return (utf16Char *)&str[i]; };
	return NULL;
}

utf16Char *strcat16(utf16Char *dest, const utf16Char *src)
{
	uarch_t		len;

	len = strlen16(dest);
	strcpy16(&dest[len], src);
	return dest;
}

