#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/tagged_list.hpp"
#include "core/tag_table.hpp"
#include "core/value.hpp"
#include "ffi/dynalo_loader.hpp"
#include "ffi/ffi.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "stdlib/args.hpp"
#include "stdlib/dl/dl.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace on1x::stdlib {
namespace {

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Library handles stored in a global registry.
// DynamicLibrary is non-copyable and non-movable (has dtor but no move ctor),
// so we wrap in unique_ptr.
static constexpr const char dl_handle_tag[] = "DlHandle";

struct DlRegistry {
    std::mutex mtx;
    std::unordered_map<std::uintptr_t, std::unique_ptr<on1x::ffi::DynamicLibrary>> libs;
    std::uintptr_t next_id = 1;

    static DlRegistry& instance() {
        static DlRegistry reg;
        return reg;
    }
};

static TagObject* dl_handle_tag_obj(On1x_State* s) {
    return s->tags.intern(&s->gc, dl_handle_tag);
}

static std::string_view dl_handle_tag_sv() {
    return std::string_view(dl_handle_tag);
}

static Value make_dl_handle(On1x_State* s, std::uintptr_t id) {
    auto* tag = dl_handle_tag_obj(s);
    auto* tl = new_tagged_list(&s->gc, tag, 1);
    if (!tl) return Value();
    GcRoot tl_root(tl);
    list_push(&s->gc, tl, Value::integer(&s->gc, static_cast<std::int64_t>(id)));
    return value_from_object(tl);
}

static bool get_dl_handle_id(Value v, std::uintptr_t& id) {
    if (v.kind() != Value::Kind::List) return false;
    const auto* list = as_list_const(v);
    if (!list || !list->constructor) return false;
    if (tag_text(list->constructor) != dl_handle_tag_sv()) return false;
    if (list->length < 1) return false;
    Value id_val;
    if (!list_get(list, 0, id_val) || !id_val.is_int()) return false;
    id = static_cast<std::uintptr_t>(id_val.as_int());
    return true;
}

// Open(path) -> :Some[:DlHandle[ptr]] | :None
On1x_Status dl_open(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Dl.Open")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Dl.Open expects a String path");
    auto text = string_view(as_string_const(v));
    std::string path(text);

    auto& reg = DlRegistry::instance();
    std::lock_guard<std::mutex> lock(reg.mtx);

    auto lib = std::make_unique<on1x::ffi::DynamicLibrary>();
    if (!lib->open(path)) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::uintptr_t id = reg.next_id++;
    reg.libs[id] = std::move(lib);
    Value handle = make_dl_handle(s, id);
    if (handle.kind() != Value::Kind::List) {
        reg.libs.erase(id);
        return bad(s, "Dl.Open: allocation failed");
    }
    return stack_push(s, make_some(&s->gc, s->reserved, handle)) ? ON1X_OK : ON1X_ERR;
}

// Close(handle) -> :Unit
On1x_Status dl_close(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Dl.Close")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v)) return bad(s, "Dl.Close expects a handle");
    std::uintptr_t id = 0;
    if (!get_dl_handle_id(v, id)) return bad(s, "Dl.Close: invalid handle");
    auto& reg = DlRegistry::instance();
    std::lock_guard<std::mutex> lock(reg.mtx);
    auto it = reg.libs.find(id);
    if (it == reg.libs.end()) return bad(s, "Dl.Close: handle not found");
    it->second->close();
    reg.libs.erase(it);
    return stack_push(s, Value::unit()) ? ON1X_OK : ON1X_ERR;
}

// Symbol(handle, name) -> :Some[Int] | :None
// Returns the raw function pointer as an Int.
On1x_Status dl_symbol(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Dl.Symbol")) return ON1X_ERR;
    Value hv, nv;
    if (!read_argument(s, 1, hv) || !read_argument(s, 2, nv))
        return bad(s, "Dl.Symbol expects a handle and a String name");
    if (nv.kind() != Value::Kind::String)
        return bad(s, "Dl.Symbol expects a String name");
    std::uintptr_t id = 0;
    if (!get_dl_handle_id(hv, id)) return bad(s, "Dl.Symbol: invalid handle");

    auto& reg = DlRegistry::instance();
    std::lock_guard<std::mutex> lock(reg.mtx);
    auto it = reg.libs.find(id);
    if (it == reg.libs.end()) return bad(s, "Dl.Symbol: handle not found");

    auto name_text = string_view(as_string_const(nv));
    std::string name_str(name_text);
    void* sym = it->second->find(name_str);
    if (!sym) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto ptr_val = Value::integer(&s->gc, reinterpret_cast<std::int64_t>(sym));
    return stack_push(s, make_some(&s->gc, s->reserved, ptr_val)) ? ON1X_OK : ON1X_ERR;
}

// Bind(handle, name, signature) -> :Some[:Fn] | :None
// Full FFI binding — stub for 0.1.0.
On1x_Status dl_bind(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "Dl.Bind")) return ON1X_ERR;
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"Open",   dl_open},
    {"Close",  dl_close},
    {"Symbol", dl_symbol},
    {"Bind",   dl_bind},
};

const On1x_ModuleDesc desc{"Dl", ON1X_CAP_DL, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* dl_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
