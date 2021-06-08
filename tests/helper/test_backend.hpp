#include "bun/bun.h"

template<typename Unwind, typename Destroy>
static bool
initialize_test_backend(struct bun_handle *handle, Unwind &&u, Destroy &&d)
{
    using decayed_unwind = std::decay_t<Unwind>;
    using decayed_destroy = std::decay_t<Destroy>;
    using pair = std::pair<decayed_unwind, decayed_destroy>;

    handle->backend_context = static_cast<void *>(new pair{
        std::forward<Unwind>(u), std::forward<Destroy>(d)});
    handle->unwind = +[](
        struct bun_handle *handle, void *buffer, size_t buffer_size) -> size_t {
        auto *ctx = static_cast<const pair *>(handle->backend_context);
        return ctx->first(handle, buffer, buffer_size);
    };
    handle->destroy = +[](struct bun_handle *handle) {
        auto *ctx = static_cast<pair *>(handle->backend_context);
        ctx->second(handle);
        delete ctx;
    };
    return true;
}
