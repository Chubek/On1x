#include "runtime/effect.hpp"

#include "core/optional.hpp"
#include "core/result.hpp"
#include "gc/roots.hpp"

namespace on1x::runtime {

Value capture_success(GcState* gc, const ReservedTags& tags, Value value) {
    const Value payload = value.is_unit()
        ? make_none(gc, tags)
        : make_some(gc, tags, value);
    GcRoot payload_root(payload.is_object() ? payload.as_object() : nullptr);
    return make_success(gc, tags, payload);
}

}  // namespace on1x::runtime
