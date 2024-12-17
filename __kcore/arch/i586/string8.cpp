
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


utf8Char *strstr8(const utf8Char *str1, const utf8Char *const str2)
{
	uarch_t		s1len, s2len;

	if (str1 == NULL || str2 == NULL)
	{
		printf(FATAL"strstr8: str1 %p, str2 %p, caller %x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	s1len = strlen8(str1);
	s2len = strlen8(str2);

	// Lazy implementation: reuse strncmp.
	for (uarch_t i=0; i<s1len; i++)
	{
		if (strncmp8(&str1[i], str2, s2len) == 0)
			{ return (utf8Char *)&str1[i]; };
	};

	return NULL;
}

utf8Char *strcpy8(utf8Char *dest, const utf8Char *src)
{
	uarch_t		i=0;

	if (dest == NULL || src == NULL)
	{
		printf(FATAL"strcpy8: dest %p, src %p, caller %x.\n",
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
		printf(FATAL"strncpy8: dest %p, src %p, caller %x.\n",
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
		printf(FATAL"strlen8: str %p, caller %x.\n",
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
		printf(FATAL"strcmp8: str1 %p, str2 %p, caller %x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; (*str1 && *str2); str1++, str2++) {
		if (*str1 != *str2) { return *str1 - *str2; };
	};

	if (*str1 != *str2) { return *str1 - *str2; };
	return 0;
}

int strncmp8(const utf8Char *str1, const utf8Char *str2, int count)
{
	if (str1 == str2) { return 0; };

	if (str1 == NULL || str2 == NULL)
	{
		printf(FATAL"strncmp8: str1 %p, str2 %p, caller %x.\n",
			str1, str2, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	uarch_t		n=0;

	for (; count > 0; count--, n++)
	{
		if (str1[n] == '\0' || str2[n] == '\0') { break; };
		if (str1[n] != str2[n]) {
			return str1[n] - str2[n];
		};
	};

	if (count)
	{
		if ((str1[n] && (str2[n] == '\0'))
			|| ((str1[n] == '\0') && str2[n]))
		{
			return str1[n] - str2[n];
		};
	};

	return 0;
}

size_t strnlen8(const utf8Char *str1, size_t maxLen)
{
	size_t		len;

	if (str1 == NULL)
	{
		printf(FATAL"strnlen8: str1 %p, caller %x.\n",
			str1, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (len=0; len < maxLen && str1[len] != '\0'; len++) {};
	return len;
}

utf8Char *strnchr8(const utf8Char *str, size_t n, const utf8Char chr)
{
	uarch_t		i=0;

	if (str == NULL)
	{
		printf(FATAL"strnchr8: str1 %p, caller %x.\n",
			str, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

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

utf8Char *strcat8(utf8Char *dest, const utf8Char *src)
{
	uarch_t		len;

	len = strlen8(dest);
	strcpy8(&dest[len], src);
	return dest;
}

