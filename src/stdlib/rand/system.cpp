#include "api/api_common.hpp"
#include "core/string.hpp"
 #include "core/optional.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace on1x::stdlib {
namespace rand_detail {

// SeedFromSystem() -> :Int (64-bit seed from /dev/urandom)
// Returns :None if entropy source is unavailable.
On1x_Status rand_seed_from_system(On1x_State* s, int argc) noexcept {
    (void)argc;
    // Read 8 bytes from /dev/urandom
    std::FILE* f = std::fopen("/dev/urandom", "rb");
    if (!f) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::uint64_t seed = 0;
    std::size_t n = std::fread(&seed, 1, sizeof(seed), f);
    std::fclose(f);
    if (n != sizeof(seed)) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, make_some(&s->gc, s->reserved,
        Value::integer(&s->gc, static_cast<std::int64_t>(seed)))) ? ON1X_OK : ON1X_ERR;
}

}  // namespace rand_detail
}  // namespace on1x::stdlib
