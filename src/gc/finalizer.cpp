#include "gc/gc.hpp"

namespace on1x {

static_assert(sizeof(GcFinalizer) == sizeof(void (*)(void*, void*)));

}
