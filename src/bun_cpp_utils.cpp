#include "bun/utils.h"

#include <cstring>
#include <string>

#include <cxxabi.h>

bool
bun_unwind_demangle(char *dest, size_t dest_size, const char *src)
{
	char *result;

	result = abi::__cxa_demangle(src, nullptr, nullptr, nullptr);
	if (result) {
		size_t len = strlen(result);

		if (len >= dest_size)
			goto error;

		strcpy(dest, result);
		return true;
	}

error:
	free(result);
	return false;
}
