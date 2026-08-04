#pragma once

#include <cstddef>
#include <new>
#include <utility>
#include <vector>

namespace on1x {

class Arena {
public:
    explicit Arena(std::size_t block_size = 16 * 1024);
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&& other) noexcept;
    Arena& operator=(Arena&& other) noexcept;

    [[nodiscard]] void* allocate(
        std::size_t size,
        std::size_t alignment = alignof(std::max_align_t));

    template <typename T, typename... Args>
    [[nodiscard]] T* make(Args&&... args) {
        return new (allocate(sizeof(T), alignof(T))) T(std::forward<Args>(args)...);
    }

    void reset() noexcept;
    [[nodiscard]] std::size_t bytes_allocated() const noexcept;

private:
    struct Block {
        std::byte* data = nullptr;
        std::size_t capacity = 0;
        std::size_t used = 0;
    };

    void add_block(std::size_t minimum_capacity);

    std::size_t block_size_;
    std::size_t bytes_allocated_ = 0;
    std::vector<Block> blocks_;
};

}
