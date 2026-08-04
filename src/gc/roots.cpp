#include "gc/roots.hpp"

#include <new>

namespace on1x {

GcRoot::GcRoot(void* ptr) : ptr_(ptr) {
    GC_add_roots(&ptr_, &ptr_ + 1);
}

GcRoot::~GcRoot() {
    GC_remove_roots(&ptr_, &ptr_ + 1);
}

GcRootVector::~GcRootVector() {
    clear();
}

void GcRootVector::push(void* ptr) {
    auto* slot = new Slot{ptr};
    GC_add_roots(&slot->value, &slot->value + 1);
    try {
        roots_.push_back(slot);
    } catch (...) {
        GC_remove_roots(&slot->value, &slot->value + 1);
        delete slot;
        throw;
    }
}

void* GcRootVector::operator[](std::size_t index) const {
    return roots_[index]->value;
}

void GcRootVector::clear() noexcept {
    for (Slot* slot : roots_) {
        GC_remove_roots(&slot->value, &slot->value + 1);
        delete slot;
    }
    roots_.clear();
}

ScopedRoot::ScopedRoot(void* ptr) : ptr_(ptr) {
    GC_add_roots(&ptr_, &ptr_ + 1);
}

ScopedRoot::~ScopedRoot() {
    GC_remove_roots(&ptr_, &ptr_ + 1);
}

}
