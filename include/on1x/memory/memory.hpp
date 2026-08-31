#ifndef ON1X_MEMORY_MEMORY_HPP
#define ON1X_MEMORY_MEMORY_HPP

#include <cstddef>
#include <utility>

namespace on1x::memory {

void initialize() noexcept;
bool initialized() noexcept;
void collect() noexcept;

std::size_t collection_count() noexcept;
std::size_t heap_size() noexcept;
std::size_t free_bytes() noexcept;
std::size_t total_bytes() noexcept;

void *allocate(std::size_t bytes) noexcept;
void *allocate_atomic(std::size_t bytes) noexcept;
void *reallocate(void *ptr, std::size_t bytes) noexcept;
void deallocate(void *ptr) noexcept;

class arena {
 public:
  explicit arena(unsigned block_size = 65536) noexcept;
  ~arena() noexcept;

  arena(arena &&other) noexcept;
  arena &operator=(arena &&other) noexcept;

  arena(const arena &) = delete;
  arena &operator=(const arena &) = delete;

  bool valid() const noexcept;
  void *allocate(std::size_t bytes, unsigned alignment = 1) noexcept;
  bool save() noexcept;
  bool restore() noexcept;
  std::size_t capacity() const noexcept;
  void reset(unsigned block_size = 65536) noexcept;

  void *native() const noexcept;

 private:
  void *handle_ = nullptr;
};

class pool {
 public:
  explicit pool(std::size_t object_size) noexcept;
  ~pool() noexcept;

  pool(pool &&other) noexcept;
  pool &operator=(pool &&other) noexcept;

  pool(const pool &) = delete;
  pool &operator=(const pool &) = delete;

  bool valid() const noexcept;
  void *allocate() noexcept;
  void release(void *ptr) noexcept;
  void reset(std::size_t object_size) noexcept;

  void *native() const noexcept;

 private:
  void *handle_ = nullptr;
};

}  // namespace on1x::memory

#endif
