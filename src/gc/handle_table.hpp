#pragma once

#include "gc/gc.hpp"

#include <cstddef>

namespace on1x {

struct GcHandle {
    void* value = nullptr;
    GcHandle* next = nullptr;
};

class GcHandleTable {
public:
    explicit GcHandleTable(GcState* gc) : gc_(gc) {}
    ~GcHandleTable();

    GcHandleTable(const GcHandleTable&) = delete;
    GcHandleTable& operator=(const GcHandleTable&) = delete;

    [[nodiscard]] GcHandle* create(void* value);
    void release(GcHandle* handle);
    [[nodiscard]] std::size_t count() const noexcept;

private:
    GcState* gc_;
    GcHandle* head_ = nullptr;
    std::size_t count_ = 0;
};

}
