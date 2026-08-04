#include "util/string_builder.hpp"

#include "util/utf8.hpp"

namespace on1x {

StringBuilder::StringBuilder(std::size_t initial_capacity) {
    buffer_.reserve(initial_capacity);
}

void StringBuilder::append(std::string_view text) {
    buffer_.append(text);
}

void StringBuilder::append(char character) {
    buffer_.push_back(character);
}

bool StringBuilder::append_code_point(char32_t code_point) {
    return utf8::append(buffer_, code_point);
}

void StringBuilder::clear() noexcept {
    buffer_.clear();
}

std::size_t StringBuilder::size() const noexcept {
    return buffer_.size();
}

bool StringBuilder::empty() const noexcept {
    return buffer_.empty();
}

std::string_view StringBuilder::view() const noexcept {
    return buffer_;
}

const std::string& StringBuilder::str() const noexcept {
    return buffer_;
}

std::string StringBuilder::take() {
    std::string result = std::move(buffer_);
    buffer_.clear();
    return result;
}

}
