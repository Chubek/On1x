#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace on1x {

template <typename T, std::size_t InlineCapacity>
class SmallVector {
    static_assert(InlineCapacity > 0);

public:
    SmallVector() noexcept : data_(inline_data()) {}

    SmallVector(const SmallVector& other) : SmallVector() {
        reserve(other.size_);
        try {
            for (const T& value : other) {
                emplace_back(value);
            }
        } catch (...) {
            clear();
            release_heap();
            throw;
        }
    }

    SmallVector(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : SmallVector() {
        if (other.using_inline()) {
            for (T& value : other) {
                emplace_back(std::move(value));
            }
            other.clear();
        } else {
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = other.inline_data();
            other.size_ = 0;
            other.capacity_ = InlineCapacity;
        }
    }

    ~SmallVector() {
        clear();
        release_heap();
    }

    SmallVector& operator=(SmallVector other) noexcept(
        std::is_nothrow_move_constructible_v<T>) {
        swap(other);
        return *this;
    }

    [[nodiscard]] T* data() noexcept { return data_; }
    [[nodiscard]] const T* data() const noexcept { return data_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    [[nodiscard]] T* begin() noexcept { return data_; }
    [[nodiscard]] const T* begin() const noexcept { return data_; }
    [[nodiscard]] T* end() noexcept { return data_ + size_; }
    [[nodiscard]] const T* end() const noexcept { return data_ + size_; }

    [[nodiscard]] T& operator[](std::size_t index) noexcept { return data_[index]; }
    [[nodiscard]] const T& operator[](std::size_t index) const noexcept { return data_[index]; }

    [[nodiscard]] T& at(std::size_t index) {
        if (index >= size_) {
            throw std::out_of_range("SmallVector index out of range");
        }
        return data_[index];
    }

    [[nodiscard]] const T& at(std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("SmallVector index out of range");
        }
        return data_[index];
    }

    [[nodiscard]] T& back() noexcept { return data_[size_ - 1]; }
    [[nodiscard]] const T& back() const noexcept { return data_[size_ - 1]; }

    void reserve(std::size_t requested_capacity) {
        if (requested_capacity > capacity_) {
            grow(requested_capacity);
        }
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            grow(capacity_ * 2);
        }
        T* slot = data_ + size_;
        std::construct_at(slot, std::forward<Args>(args)...);
        ++size_;
        return *slot;
    }

    void push_back(const T& value) { emplace_back(value); }
    void push_back(T&& value) { emplace_back(std::move(value)); }

    void pop_back() noexcept {
        --size_;
        std::destroy_at(data_ + size_);
    }

    void clear() noexcept {
        std::destroy(data_, data_ + size_);
        size_ = 0;
    }

    void swap(SmallVector& other) noexcept(std::is_nothrow_move_constructible_v<T>) {
        if (this == &other) {
            return;
        }
        SmallVector temporary(std::move(other));
        other.move_from(std::move(*this));
        move_from(std::move(temporary));
    }

private:
    [[nodiscard]] T* inline_data() noexcept {
        return reinterpret_cast<T*>(inline_storage_);
    }

    [[nodiscard]] const T* inline_data() const noexcept {
        return reinterpret_cast<const T*>(inline_storage_);
    }

    [[nodiscard]] bool using_inline() const noexcept {
        return data_ == inline_data();
    }

    void grow(std::size_t requested_capacity) {
        const std::size_t new_capacity = std::max(requested_capacity, capacity_ + 1);
        T* new_data = allocator_.allocate(new_capacity);
        std::size_t constructed = 0;
        try {
            for (; constructed < size_; ++constructed) {
                std::construct_at(
                    new_data + constructed,
                    std::move_if_noexcept(data_[constructed]));
            }
        } catch (...) {
            std::destroy(new_data, new_data + constructed);
            allocator_.deallocate(new_data, new_capacity);
            throw;
        }

        std::destroy(data_, data_ + size_);
        release_heap();
        data_ = new_data;
        capacity_ = new_capacity;
    }

    void release_heap() noexcept {
        if (!using_inline()) {
            allocator_.deallocate(data_, capacity_);
        }
    }

    void move_from(SmallVector&& other) {
        clear();
        release_heap();
        data_ = inline_data();
        capacity_ = InlineCapacity;
        if (other.using_inline()) {
            for (T& value : other) {
                emplace_back(std::move(value));
            }
            other.clear();
        } else {
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            other.data_ = other.inline_data();
            other.size_ = 0;
            other.capacity_ = InlineCapacity;
        }
    }

    alignas(T) std::byte inline_storage_[sizeof(T) * InlineCapacity];
    std::allocator<T> allocator_;
    T* data_;
    std::size_t size_ = 0;
    std::size_t capacity_ = InlineCapacity;
};

template <typename T, std::size_t InlineCapacity>
void swap(
    SmallVector<T, InlineCapacity>& left,
    SmallVector<T, InlineCapacity>& right) noexcept(noexcept(left.swap(right))) {
    left.swap(right);
}

}
