#include "bun/utils.h"

#include <cstring>
#include <string>

#include <cxxabi.h>

template<typename T>
struct deleter
{
	void operator() (T data) {
		free(data);
	}
};

bool
bun_unwind_demangle(char *dest, size_t dest_size, const char *src)
{
	// TODO: T14745
	std::unique_ptr<char, deleter<char*>> result(abi::__cxa_demangle(src, nullptr, nullptr, nullptr), deleter<char*>());
	char* result_cstr = result.get();
	if (result_cstr) {
		size_t len = strlen(result_cstr);

		if (len >= dest_size)
			goto error;

		strcpy(dest, result_cstr);
		return true;
	}

error:
	return false;
}
