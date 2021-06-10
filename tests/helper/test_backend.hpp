#include "bun/bun.h"

template<typename Unwind, typename Destroy>
static bool
initialize_test_backend(struct bun_handle *handle, Unwind &&u, Destroy &&d)
{
    using decayed_unwind = std::decay_t<Unwind>;
    using decayed_destroy = std::decay_t<Destroy>;
    using vtable = std::tuple<decayed_unwind, decayed_destroy>;

    handle->backend_context = static_cast<void *>(new vtable{
        std::forward<Unwind>(u), std::forward<Destroy>(d)});

    handle->unwind = +[](
        struct bun_handle *handle, void *buffer, size_t buffer_size) -> size_t {
        auto *ctx = static_cast<const vtable *>(handle->backend_context);
        return std::get<decayed_unwind>(*ctx)(handle, buffer, buffer_size);
    };

    handle->destroy = +[](struct bun_handle *handle) {
        auto *ctx = static_cast<vtable *>(handle->backend_context);
        std::get<decayed_destroy>(*ctx)(handle);
        delete ctx;
    };

    handle->flags = 0;

    return true;
}
