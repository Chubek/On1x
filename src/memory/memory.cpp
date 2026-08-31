#include "on1x/memory/memory.hpp"

#include <algorithm>
#include <utility>

#include "gc.h"
#include "kbarena.h"
#include "kmempool.h"

namespace on1x::memory {
namespace {

void ensure_initialized() noexcept {
  if (!GC_is_init_called()) {
    GC_INIT();
  }
}

}  // namespace

void initialize() noexcept {
  ensure_initialized();
}

bool initialized() noexcept {
  return GC_is_init_called() != 0;
}

void collect() noexcept {
  ensure_initialized();
  GC_gcollect();
}

std::size_t collection_count() noexcept {
  ensure_initialized();
  return static_cast<std::size_t>(GC_get_gc_no());
}

std::size_t heap_size() noexcept {
  ensure_initialized();
  return static_cast<std::size_t>(GC_get_heap_size());
}

std::size_t free_bytes() noexcept {
  ensure_initialized();
  return static_cast<std::size_t>(GC_get_free_bytes());
}

std::size_t total_bytes() noexcept {
  ensure_initialized();
  return static_cast<std::size_t>(GC_get_total_bytes());
}

void *allocate(std::size_t bytes) noexcept {
  ensure_initialized();
  return GC_MALLOC(bytes);
}

void *allocate_atomic(std::size_t bytes) noexcept {
  ensure_initialized();
  return GC_MALLOC_ATOMIC(bytes);
}

void *reallocate(void *ptr, std::size_t bytes) noexcept {
  ensure_initialized();
  return GC_REALLOC(ptr, bytes);
}

void deallocate(void *ptr) noexcept {
  ensure_initialized();
  GC_FREE(ptr);
}

arena::arena(unsigned block_size) noexcept : handle_(kba_init(block_size)) {}

arena::~arena() noexcept {
  if (handle_) {
    kba_destroy(handle_);
    handle_ = nullptr;
  }
}

arena::arena(arena &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

arena &arena::operator=(arena &&other) noexcept {
  if (this == &other) return *this;
  if (handle_) {
    kba_destroy(handle_);
  }
  handle_ = std::exchange(other.handle_, nullptr);
  return *this;
}

bool arena::valid() const noexcept {
  return handle_ != nullptr;
}

void *arena::allocate(std::size_t bytes, unsigned alignment) noexcept {
  if (!handle_) return nullptr;
  return kba_alloc(handle_, static_cast<unsigned>(bytes), alignment == 0 ? 1u : alignment);
}

bool arena::save() noexcept {
  return handle_ && kba_save(handle_) == 0;
}

bool arena::restore() noexcept {
  return handle_ && kba_restore(handle_) == 0;
}

std::size_t arena::capacity() const noexcept {
  return handle_ ? kba_capacity(handle_) : 0;
}

void arena::reset(unsigned block_size) noexcept {
  if (handle_) {
    kba_destroy(handle_);
  }
  handle_ = kba_init(block_size);
}

void *arena::native() const noexcept {
  return handle_;
}

pool::pool(std::size_t object_size) noexcept : handle_(kmp_init(static_cast<unsigned>(std::max<std::size_t>(object_size, 1)))) {}

pool::~pool() noexcept {
  if (handle_) {
    kmp_destroy(handle_);
    handle_ = nullptr;
  }
}

pool::pool(pool &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {}

pool &pool::operator=(pool &&other) noexcept {
  if (this == &other) return *this;
  if (handle_) {
    kmp_destroy(handle_);
  }
  handle_ = std::exchange(other.handle_, nullptr);
  return *this;
}

bool pool::valid() const noexcept {
  return handle_ != nullptr;
}

void *pool::allocate() noexcept {
  return handle_ ? kmp_alloc(handle_) : nullptr;
}

void pool::release(void *ptr) noexcept {
  if (handle_ && ptr) {
    kmp_free(handle_, ptr);
  }
}

void pool::reset(std::size_t object_size) noexcept {
  if (handle_) {
    kmp_destroy(handle_);
  }
  handle_ = kmp_init(static_cast<unsigned>(std::max<std::size_t>(object_size, 1)));
}

void *pool::native() const noexcept {
  return handle_;
}

}  // namespace on1x::memory
