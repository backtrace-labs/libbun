#include "demangle.h"

#include <string>

#include <cstring>

#include <cxxabi.h>

bool
bun_unwind_demangle(char *dest, size_t dest_size, const char *src)
{
	std::string name(dest_size, '\0');
	int status;
	size_t buf_size = dest_size;
	char *result;

	result = abi::__cxa_demangle(src, &name[0], &buf_size, &status);

	if (status == 0) {
		strcpy(dest, name.data());
		return true;
	} else {
		return false;
	}
}
