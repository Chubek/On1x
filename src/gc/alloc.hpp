#pragma once

// GC-aware allocation entry points.
// Nothing outside src/gc may call GC_MALLOC directly; use these wrappers.
//
// spec §16: Memory is managed by a tracing garbage collector inside the state.

#include "gc/gc.hpp"
#include "util/assert.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <new>
#include <type_traits>
#include <utility>

extern "C" {
#include "gc.h"
}

namespace on1x {

// Allocate raw memory with no C++ constructor call.
// Returns nullptr on allocation failure (OOM).
inline void* gc_alloc_raw(GcState* gc, std::size_t size) {
    ON1X_ASSERT(gc != nullptr && gc->initialized, "gc allocation requires an initialized state");
    if (size == 0) {
        size = 1;
    }
    void* ptr = GC_MALLOC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

// Allocate an object of type T and default-construct it.
// GC-collected; no matching free() call.
template <typename T, typename... Args>
T* gc_alloc(GcState* gc, Args&&... args) {
    void* memory = gc_alloc_raw(gc, sizeof(T));
    return new (memory) T(std::forward<Args>(args)...);
}

// Allocate an array of T with count elements, value-initialised.
template <typename T>
T* gc_alloc_array(GcState* gc, std::size_t count) {
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(T)) {
        throw std::bad_array_new_length();
    }
    void* memory = gc_alloc_raw(gc, sizeof(T) * count);
    auto* array = static_cast<T*>(memory);
    for (std::size_t i = 0; i < count; ++i) {
        new (&array[i]) T();
    }
    return array;
}

// Allocate and copy raw bytes; returns a pointer to the copy.
inline void* gc_alloc_copy(GcState* gc, const void* src, std::size_t size) {
    void* memory = gc_alloc_raw(gc, size);
    if (size != 0) {
        std::memcpy(memory, src, size);
    }
    return memory;
}

// Reallocate a GC-allocated block. Semantics match C realloc().
inline void* gc_realloc(GcState* gc, void* ptr, std::size_t new_size) {
    ON1X_ASSERT(gc != nullptr && gc->initialized, "gc reallocation requires an initialized state");
    void* new_ptr = GC_REALLOC(ptr, new_size);
    if (!new_ptr) throw std::bad_alloc();
    return new_ptr;
}

// Free a GC pointer explicitly (rare; normally let the GC handle it).
inline void gc_free(GcState* gc, void* ptr) {
    ON1X_ASSERT(gc != nullptr && gc->initialized, "gc free requires an initialized state");
    GC_FREE(ptr);
}

// Allocate atomically (pointer-free memory that the GC need not scan).
inline void* gc_alloc_atomic(GcState* gc, std::size_t size) {
    ON1X_ASSERT(gc != nullptr && gc->initialized, "gc allocation requires an initialized state");
    if (size == 0) {
        size = 1;
    }
    void* ptr = GC_MALLOC_ATOMIC(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

}  // namespace on1x
