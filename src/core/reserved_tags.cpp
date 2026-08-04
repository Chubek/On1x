#include "core/reserved_tags.hpp"

namespace on1x {

ReservedTags make_reserved_tags(GcState* gc, TagTable& tags) {
    return {
        tags.intern(gc, "Unit"), tags.intern(gc, "Bool"), tags.intern(gc, "Int"),
        tags.intern(gc, "Float"), tags.intern(gc, "String"), tags.intern(gc, "Tag"),
        tags.intern(gc, "List"), tags.intern(gc, "Table"), tags.intern(gc, "Fn"),
        tags.intern(gc, "Iota"), tags.intern(gc, "Some"), tags.intern(gc, "None"),
        tags.intern(gc, "Success"), tags.intern(gc, "Error"),
    };
}

}  // namespace on1x
