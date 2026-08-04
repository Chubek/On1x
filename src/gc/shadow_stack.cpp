#include "gc/shadow_stack.hpp"

extern "C" {
#include <gc.h>
}

namespace on1x {

struct ShadowStack::Slot {
    void* value = nullptr;
    Slot* previous = nullptr;
};

ShadowStack::~ShadowStack() {
    while (top_ != nullptr) {
        pop();
    }
}

void ShadowStack::push(void* value) {
    auto* slot = new Slot{value, top_};
    GC_add_roots(&slot->value, &slot->value + 1);
    top_ = slot;
    ++size_;
}

void ShadowStack::pop() noexcept {
    if (top_ == nullptr) {
        return;
    }
    Slot* slot = top_;
    top_ = slot->previous;
    GC_remove_roots(&slot->value, &slot->value + 1);
    delete slot;
    --size_;
}

std::size_t ShadowStack::size() const noexcept {
    return size_;
}

}
