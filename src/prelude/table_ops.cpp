#include "prelude/prelude.hpp"

#include "core/list.hpp"
#include "core/table.hpp"
#include "gc/roots.hpp"

namespace on1x::prelude {

namespace {

On1x_Status collect(On1x_State* state, int argc, bool collect_keys) {
    const char* name = collect_keys ? "Keys expects one argument" : "Values expects one argument";
    if (!has_arity(state, argc, 1, name)) return ON1X_ERR;
    Value value;
    if (!argument(state, 1, value)) return raise(state, "Table operation received an invalid argument");
    const TableObject* table = as_table_const(value);
    if (!table) return raise(state, collect_keys ? "Keys expects a Table" : "Values expects a Table");
    try {
        GcRoot table_root(const_cast<TableObject*>(table));
        auto* result = new_list(&state->gc, table->length);
        GcRoot result_root(result);
        for (const TableEntry* entry = table->entries; entry; entry = entry->next) {
            if (!list_push(&state->gc, result, collect_keys ? entry->key : entry->value)) {
                return raise(state, "unable to enumerate Table");
            }
        }
        return push_result(state, value_from_object(result));
    } catch (...) {
        return raise(state, "unable to enumerate Table");
    }
}

}  // namespace

On1x_Status keys(On1x_State* state, int argc) {
    return collect(state, argc, true);
}

On1x_Status values(On1x_State* state, int argc) {
    return collect(state, argc, false);
}

}  // namespace on1x::prelude
