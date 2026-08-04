#pragma once

#include <cstdint>

namespace on1x {

enum class ObjectKind : std::uint8_t {
    String,
    BigInt,
    List,
    Table,
    Tag,
    Function,
    Environment,
    NativeResource,
};

struct ObjectHeader {
    ObjectKind kind;
    std::uint8_t flags = 0;
    std::uint16_t reserved = 0;
};

}
