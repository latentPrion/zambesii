
#include <__kstdlib/__kclib/string16.h>


void *memset16(void *_ptr, int value, size_t count)
{
	ubit16	*ptr = (ubit16 *)_ptr;

	if (ptr == __KNULL) { return ptr; };

	while (count--) {
		*ptr++ = (ubit16)value;
	};
	return ptr;
}

void *memcpy16(void *_dest, void *_src, size_t count)
{
	ubit16		*dest=(ubit16 *)_dest;
	ubit16		*src=(ubit16 *)_src;

	if (dest == __KNULL || src == __KNULL) { return dest; };

	while (count--) {
		*dest++ = *src++;
	};
	return dest;
}

utf16Char *strcpy16(utf16Char *dest, const utf16Char *src)
{
	if (dest == __KNULL || src == __KNULL) { return dest; };

	while (*src) {
		*dest++ = *src++;
	};
	*dest = 0;
	return dest;
}

size_t strlen16(const utf16Char *str)
{
	uarch_t		len=0;

	if (str == __KNULL) { return 0; };

	while (*str++) {
		len++;
	};
	return len;
}

int strcmp16(const utf16Char *str1, const utf16Char *str2)
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

int strncmp16(const utf16Char *str1, const utf16Char *str2, int count)
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

#ifdef __cplusplus
}
#endif

