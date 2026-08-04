#include "ffi/marshal.hpp"

#include "core/string.hpp"

extern "C" {
#include <dyncall.h>
}

namespace on1x::ffi {

bool push_argument(DCCallVM* vm, SignatureType type, Value value) noexcept {
    switch (type) {
    case SignatureType::Bool:
        if (!value.is_bool()) return false;
        dcArgBool(vm, static_cast<DCbool>(value.as_bool()));
        return true;
    case SignatureType::Int:
        if (!value.is_int()) return false;
        dcArgLongLong(vm, static_cast<DClonglong>(value.as_int()));
        return true;
    case SignatureType::Float:
        if (!value.is_float()) return false;
        dcArgDouble(vm, static_cast<DCdouble>(value.as_float()));
        return true;
    case SignatureType::Pointer:
        if (value.is_unit()) {
            dcArgPointer(vm, nullptr);
            return true;
        }
        if (const auto* string = as_string_const(value)) {
            dcArgPointer(vm, const_cast<char*>(string->data));
            return true;
        }
        return false;
    case SignatureType::Void:
        return false;
    }
    return false;
}

Value read_result(GcState* gc, DCCallVM* vm, void* function, SignatureType type) {
    switch (type) {
    case SignatureType::Void:
        dcCallVoid(vm, function);
        return Value::unit();
    case SignatureType::Bool:
        return Value::boolean(dcCallBool(vm, function) != 0);
    case SignatureType::Int:
        return Value::integer(gc, static_cast<std::int64_t>(dcCallLongLong(vm, function)));
    case SignatureType::Float:
        return Value::floating(static_cast<double>(dcCallDouble(vm, function)));
    case SignatureType::Pointer:
        static_cast<void>(dcCallPointer(vm, function));
        return Value::unit();
    }
    return Value::unit();
}

}  // namespace on1x::ffi
