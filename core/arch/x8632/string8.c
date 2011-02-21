
#include <__kstdlib/__kclib/string8.h>


void *memset(void *_ptr, int value, size_t count)
{
	if (_ptr == __KNULL) { return _ptr; };

	for (; count; count--) {
		((ubit8*)_ptr)[count-1] = (ubit8)value;
	};
	return _ptr;
}

void *memcpy(void *dest, void *src, size_t count)
{
	if (dest == __KNULL || src == __KNULL) { return dest; };

	for (; count; count--) {
		((ubit8 *)dest)[count - 1] = ((ubit8 *)src)[count - 1];
	};
	return dest;
}

utf8Char *strcpy(utf8Char *dest, const utf8Char *src)
{
	uarch_t		i=0;

	if (dest == __KNULL || src == __KNULL) { return dest; };

	for (; src[i]; i++) {
		dest[i] = src[i];
	};
	dest[i] = '\0';
	return dest;
}

utf8Char *strncpy(utf8Char *dest, const utf8Char *src, size_t count)
{
	if (dest == __KNULL || src == __KNULL) { return dest; };

	for (; count && src[count - 1]; count--) {
		dest[count - 1] = src[count - 1];
	};
	for (; count; count--) {
		dest[count - 1] = '\0';
	}
	return dest;
}

size_t strlen(const utf8Char *str)
{
	uarch_t		len=0;

	if (str == __KNULL) { return 0; };

	for (; str[len]; len++) {};
	return len;
}

int strcmp(const utf8Char *str1, const utf8Char *str2)
{
	if (str1 == __KNULL || str2 == __KNULL) { return 1; };
	if (str1 == str2) { return 0; };

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

int strncmp(const utf8Char *str1, const utf8Char *str2, int count)
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

