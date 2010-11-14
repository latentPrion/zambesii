
#include <__kstdlib/__kclib/string8.h>

void *memset8(void *_ptr, int value, size_t count)
{
	ubit8	*ptr = (ubit8 *)_ptr;

	if (ptr == __KNULL) { return ptr; };

	while (count--) {
		*ptr++ = (ubit8)value;
	};
	return ptr;
}

void *memcpy8(void *_dest, void *_src, size_t count)
{
	ubit8		*dest=(ubit8 *)_dest;
	ubit8		*src=(ubit8 *)_src;

	if (dest == __KNULL || src == __KNULL) { return dest; };

	while (count--) {
		*dest++ = *src++;
	};
	return dest;
}

utf8Char *strcpy8(utf8Char *dest, const utf8Char *src)
{
	if (dest == __KNULL || src == __KNULL) { return dest; };

	while (*src) {
		*dest++ = *src++;
	};
	*dest = '\0';
	return dest;
}

size_t strlen8(const utf8Char *str)
{
	uarch_t		len=0;

	if (str == __KNULL) { return 0; };

	while (*str++) {
		len++;
	};
	return len;
}

int strcmp8(const utf8Char *str1, const utf8Char *str2)
{
	if (str1 == __KNULL || str2 == __KNULL) { return 1; };
	if (str1 == str2) { return 0; };

	for (; (*str1 && *str2); str1++, str2++)
	{
		if (*str1 != *str2) {
			return ((*str1 > *str2) ? 1 : -1);
		};
	};
	return 0;
}

int strncmp8(const utf8Char *str1, const utf8Char *str2, int count)
{
	if (str1 == __KNULL || str2 == __KNULL || count == 0) { return 1; };
	if (str1 == str2) { return 0; };

	for (; count > 0; count--, str1++, str2++)
	{
		if (*str1 != *str2) {
			return ((*str1 > *str2) ? 1 : -1);
		};
	};
	return 0;
}

