#include "gc/alloc.hpp"

namespace on1x {

static_assert(std::is_same_v<decltype(gc_alloc_raw(nullptr, 1)), void*>);

}
