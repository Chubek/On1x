#pragma once

#include <cstddef>
#include <cstdint>

// spec §16: memory is managed by a tracing garbage collector inside the state.
// This header is the public interface of the GC subsystem; nothing outside
// src/gc may include libgc headers directly.

namespace on1x {

struct GcState {
    bool initialized = false;
};

// Initialise the GC for a state. Called once at state creation.
void gc_init(GcState* gc);

// Shut down the GC and release resources. Called at state destruction.
void gc_shutdown(GcState* gc);

// Trigger a full collection.
void gc_collect(GcState* gc);

// Disable / enable the collector temporarily (for critical sections).
void gc_disable(GcState* gc);
void gc_enable(GcState* gc);

// Return total bytes allocated in GC-managed heaps.
std::size_t gc_bytes_allocated(const GcState* gc);

// Register a finalizer callback: called when obj is collected.
using GcFinalizer = void (*)(void* obj, void* client_data);
void gc_register_finalizer(GcState* gc, void* obj, GcFinalizer fn, void* client_data);

}  // namespace on1x
