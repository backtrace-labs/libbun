#include "bun/utils.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include <cxxabi.h>

namespace /* anonymous */ {

struct free_caller {
	template<typename T>
	void operator() (T* data) const {
		free(data);
	}
};

} /* anonymous namespace */

bool
bun_unwind_demangle(char *dest, size_t dest_size, const char *src)
{
	// TODO: T14745
	auto demangle_ret = abi::__cxa_demangle(src, nullptr, nullptr, nullptr);
	std::unique_ptr<char, free_caller> result(demangle_ret);
	if (result) {
		size_t len = strlen(result.get());

		if (len >= dest_size)
			goto error;

		strcpy(dest, result.get());
		return true;
	}

error:
	return false;
}
