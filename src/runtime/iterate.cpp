#include "runtime/iterate.hpp"

#include "core/list.hpp"
#include "core/table.hpp"
#include "gc/roots.hpp"

namespace on1x::runtime {

bool initialize_iteration(
    GcState* gc,
    Value iterable,
    Value& normalized,
    const char*& error) noexcept {
    try {
        if (iterable.kind() == Value::Kind::Table) {
            const TableObject* table = as_table_const(iterable);
            if (!table) {
                error = "for expects a List, Table, or range";
                return false;
            }
            GcRoot table_root(const_cast<TableObject*>(table));
            auto* keys = new_list(gc, table->length);
            GcRoot keys_root(keys);
            for (const TableEntry* entry = table->entries; entry; entry = entry->next) {
                if (!list_push(gc, keys, entry->key)) {
                    error = "unable to enumerate Table";
                    return false;
                }
            }
            normalized = value_from_object(keys);
            return true;
        }
        if (iterable.kind() != Value::Kind::List) {
            error = "for expects a List, Table, or range";
            return false;
        }
        normalized = iterable;
        return true;
    } catch (...) {
        error = "unable to initialize iteration";
        return false;
    }
}

}  // namespace on1x::runtime
