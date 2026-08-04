#include "util/arena.hpp"

#include "util/bitops.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace on1x {

Arena::Arena(std::size_t block_size) : block_size_(block_size) {
    if (block_size_ == 0) {
        throw std::invalid_argument("arena block size must be positive");
    }
}

Arena::~Arena() {
    reset();
}

Arena::Arena(Arena&& other) noexcept
    : block_size_(other.block_size_),
      bytes_allocated_(other.bytes_allocated_),
      blocks_(std::move(other.blocks_)) {
    other.bytes_allocated_ = 0;
}

Arena& Arena::operator=(Arena&& other) noexcept {
    if (this != &other) {
        reset();
        block_size_ = other.block_size_;
        bytes_allocated_ = other.bytes_allocated_;
        blocks_ = std::move(other.blocks_);
        other.bytes_allocated_ = 0;
    }
    return *this;
}

void* Arena::allocate(std::size_t size, std::size_t alignment) {
    if (size == 0) {
        size = 1;
    }
    if (!is_power_of_two(alignment) || alignment > alignof(std::max_align_t)) {
        throw std::invalid_argument("arena alignment is unsupported");
    }
    if (size > std::numeric_limits<std::size_t>::max() - alignment) {
        throw std::bad_alloc();
    }

    if (blocks_.empty()) {
        add_block(size + alignment);
    }

    Block* block = &blocks_.back();
    auto address = reinterpret_cast<std::uintptr_t>(block->data + block->used);
    auto aligned = align_up(address, alignment);
    auto padding = static_cast<std::size_t>(aligned - address);
    if (padding > block->capacity - block->used ||
        size > block->capacity - block->used - padding) {
        add_block(size + alignment);
        block = &blocks_.back();
        address = reinterpret_cast<std::uintptr_t>(block->data);
        aligned = align_up(address, alignment);
        padding = static_cast<std::size_t>(aligned - address);
    }

    block->used += padding + size;
    bytes_allocated_ += size;
    return reinterpret_cast<void*>(aligned);
}

void Arena::reset() noexcept {
    for (const Block& block : blocks_) {
        ::operator delete(block.data);
    }
    blocks_.clear();
    bytes_allocated_ = 0;
}

std::size_t Arena::bytes_allocated() const noexcept {
    return bytes_allocated_;
}

void Arena::add_block(std::size_t minimum_capacity) {
    const std::size_t capacity = std::max(block_size_, minimum_capacity);
    auto* data = static_cast<std::byte*>(::operator new(capacity));
    blocks_.push_back({data, capacity, 0});
}

}
