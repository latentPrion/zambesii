
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/panic.h>


void *memset(void *_ptr, int value, size_t count)
{
	if (_ptr == NULL)
	{
		printf(FATAL"memset: dest 0x%p, n %d, caller 0x%x.\n",
			_ptr, count, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	for (; count; count--) {
		((ubit8*)_ptr)[count-1] = (ubit8)value;
	};

	return _ptr;
}

void *memcpy(void *dest, const void *src, size_t count)
{
	if (dest == NULL || src == NULL)
	{
		printf(FATAL"memcpy: dest 0x%p, src 0x%p, n %d, caller 0x%x.\n",
			dest, src, count, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	if (dest == src || count == 0) { return dest; };

	for (uarch_t i=0; i<count; i++) {
		((ubit8 *)dest)[i] = ((ubit8 *)src)[i];
	};

	return dest;
}

int memcmp(const void *ptr1, const void *ptr2, size_t n)
{
	size_t			i;
	const ubit8		*p1=(const ubit8 *)ptr1,
				*p2=(const ubit8 *)ptr2;

	if (ptr1 == NULL || ptr2 == NULL)
	{
		printf(FATAL"memcmp: ptr1 0x%p, ptr2 0x%p, n %d, caller 0x%x.\n",
			ptr1, ptr2, n, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	if (ptr1 == ptr2 || n == 0) { return 0; };
	if (n == 0) { return 0; };

	for (i=0; i<n; i++) {
		if (p1[i] != p2[i]) { break; };
	};

	return p1[i] - p2[i];
}

void *memmove(void *dest, const void *src, size_t n)
{
	if (dest == NULL || src == NULL)
	{
		printf(FATAL"memmove: dest 0x%p, src 0x%p, n %d, caller 0x%x.\n",
			dest, src, n, __builtin_return_address(0));

		panic(ERROR_CRITICAL);
	};

	if (src == dest || n == 0) { return dest; };
	if (src > dest) {
		return memcpy(dest, src, n);
	} else
	{
		ubit8		*_dest=(ubit8 *)dest;
		const ubit8	*_src=(ubit8 *)src;

		for (; n; n--) {
			_dest[n - 1] = _src[n - 1];
		};

		return dest;
	};
}
