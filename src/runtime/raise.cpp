#include "runtime/raise.hpp"

#include "core/optional.hpp"
#include "core/result.hpp"
#include "core/string.hpp"
#include "gc/roots.hpp"

namespace on1x::runtime {

Value capture_error(GcState* gc, const ReservedTags& tags, std::string_view message) {
    auto* text = new_string(gc, message);
    GcRoot text_root(text);
    const Value payload = make_some(gc, tags, value_from_object(text));
    GcRoot payload_root(payload.as_object());
    return make_error(gc, tags, payload);
}

}  // namespace on1x::runtime
