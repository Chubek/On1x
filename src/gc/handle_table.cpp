#include "gc/handle_table.hpp"

#include "gc/alloc.hpp"
#include "util/assert.hpp"

extern "C" {
#include <gc.h>
}

namespace on1x {

GcHandleTable::~GcHandleTable() {
    while (head_ != nullptr) {
        release(head_);
    }
}

GcHandle* GcHandleTable::create(void* value) {
    ON1X_ASSERT(gc_ != nullptr && gc_->initialized, "handle creation requires an initialized GC");
    auto* handle = static_cast<GcHandle*>(GC_MALLOC_UNCOLLECTABLE(sizeof(GcHandle)));
    if (handle == nullptr) {
        throw std::bad_alloc();
    }
    handle->value = value;
    handle->next = head_;
    GC_add_roots(&handle->value, &handle->value + 1);
    head_ = handle;
    ++count_;
    return handle;
}

void GcHandleTable::release(GcHandle* handle) {
    if (handle == nullptr) {
        return;
    }
    GcHandle** cursor = &head_;
    while (*cursor != nullptr && *cursor != handle) {
        cursor = &(*cursor)->next;
    }
    ON1X_ASSERT(*cursor == handle, "attempted to release an unknown GC handle");
    *cursor = handle->next;
    GC_remove_roots(&handle->value, &handle->value + 1);
    GC_FREE(handle);
    --count_;
}

std::size_t GcHandleTable::count() const noexcept {
    return count_;
}

}
