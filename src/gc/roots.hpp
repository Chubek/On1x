#pragma once

// Root-set management for the tracing GC.
// Any raw pointer to a GC object held across a possible allocation must
// be rooted. Use these utilities internally, and include/on1x/rooted.hpp
// at the C++ API boundary.
//
// AGENTS.md §7: "Any raw pointer to a GC object held across a possible
// allocation must be rooted."

#include <cstddef>
#include <cstdint>
#include <vector>

extern "C" {
#include "gc.h"
}

namespace on1x {

// A GC root: a location the collector scans for live pointers.
// One slot holds one pointer.
class GcRoot {
public:
    explicit GcRoot(void* ptr = nullptr);
    ~GcRoot();

    GcRoot(const GcRoot&) = delete;
    GcRoot& operator=(const GcRoot&) = delete;

    GcRoot(GcRoot&& other) = delete;
    GcRoot& operator=(GcRoot&& other) = delete;

    void* get() const noexcept { return ptr_; }
    void set(void* ptr) noexcept { ptr_ = ptr; }

    template <typename T>
    T* get_as() const noexcept {
        return static_cast<T*>(ptr_);
    }

private:
    void* ptr_;
};

// A root array: multiple slots, scanned contiguously.
template <std::size_t N>
class GcRootArray {
public:
    GcRootArray() { GC_add_roots(data_, data_ + N); }
    ~GcRootArray() { GC_remove_roots(data_, data_ + N); }

    GcRootArray(const GcRootArray&) = delete;
    GcRootArray& operator=(const GcRootArray&) = delete;

    void* operator[](std::size_t i) const noexcept { return data_[i]; }
    void set(std::size_t i, void* ptr) noexcept { data_[i] = ptr; }

    void clear() noexcept {
        for (std::size_t i = 0; i < N; ++i) data_[i] = nullptr;
    }

    static constexpr std::size_t size = N;

private:
    void* data_[N] = {};
};

// A dynamic root vector. Use this when the number of roots is not known
// at compile time, but prefer GcRoot or GcRootArray when possible.
//
// Note: this uses std::vector internally, but the pointers stored are
// registered as GC roots, so they are visible to the collector.
class GcRootVector {
public:
    GcRootVector() = default;
    ~GcRootVector();

    GcRootVector(const GcRootVector&) = delete;
    GcRootVector& operator=(const GcRootVector&) = delete;

    void push(void* ptr);

    void* operator[](std::size_t i) const;
    std::size_t size() const noexcept { return roots_.size(); }

    void clear() noexcept;

private:
    struct Slot {
        void* value = nullptr;
    };

    std::vector<Slot*> roots_;
};

// RAII scoped root: registers on construction, unregisters on destruction.
class ScopedRoot {
public:
    explicit ScopedRoot(void* ptr);
    ~ScopedRoot();

    ScopedRoot(const ScopedRoot&) = delete;
    ScopedRoot& operator=(const ScopedRoot&) = delete;

private:
    void* ptr_;
};

}  // namespace on1x
