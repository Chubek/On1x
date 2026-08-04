#include "gc/gc.hpp"

#include "util/assert.hpp"

extern "C" {
#include "gc.h"
}

#include <mutex>

namespace on1x {

namespace {

std::once_flag gc_once;

void initialize_collector() {
    GC_INIT();
    GC_enable_incremental();
}

}

void gc_init(GcState* gc) {
    ON1X_ASSERT(gc != nullptr, "gc_init: null state pointer");
    ON1X_ASSERT(!gc->initialized, "gc_init: already initialised");

    std::call_once(gc_once, initialize_collector);
    gc->initialized = true;
}

void gc_shutdown(GcState* gc) {
    ON1X_ASSERT(gc != nullptr, "gc_shutdown: null state pointer");
    if (!gc->initialized) return;

    gc->initialized = false;
}

void gc_collect(GcState* gc) {
    ON1X_ASSERT(gc != nullptr, "gc_collect: null state pointer");
    ON1X_ASSERT(gc->initialized, "gc_collect: not initialised");
    GC_gcollect();
}

void gc_disable(GcState* gc) {
    ON1X_ASSERT(gc != nullptr, "gc_disable: null state pointer");
    GC_disable();
}

void gc_enable(GcState* gc) {
    ON1X_ASSERT(gc != nullptr, "gc_enable: null state pointer");
    GC_enable();
}

std::size_t gc_bytes_allocated(const GcState* gc) {
    ON1X_ASSERT(gc != nullptr, "gc_bytes_allocated: null state pointer");
    return GC_get_heap_size() + GC_get_bytes_since_gc();
}

void gc_register_finalizer(GcState* gc, void* obj, GcFinalizer fn, void* client_data) {
    ON1X_ASSERT(gc != nullptr, "gc_register_finalizer: null state pointer");
    GC_REGISTER_FINALIZER(obj, fn, client_data, nullptr, nullptr);
}

}  // namespace on1x
