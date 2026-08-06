#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/tagged_list.hpp"
#include "core/value.hpp"
#include "gc/alloc.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include "stdlib/rand/rand.hpp"
#include <on1x/on1x_config.h>
#include "stdlib/rand/rand_impl.hpp"

#include <cstdint>
#include <cstring>

namespace on1x::stdlib {
namespace {

// Fallback SeedFromSystem when ON1X_STDLIB_PURE_ONLY — returns :None
#if ON1X_STDLIB_PURE_ONLY
static On1x_Status rand_seed_from_system_fallback(On1x_State* s, int argc) noexcept {
    (void)argc;
    return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
}
#endif

On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// xoshiro256** state: four 64-bit values
struct XoshiroState {
    std::uint64_t s[4];
};

static std::uint64_t rotl(std::uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

static std::uint64_t xoshiro_next(XoshiroState& state) {
    const std::uint64_t result = rotl(state.s[1] * 5, 7) * 9;
    const std::uint64_t t = state.s[1] << 17;
    state.s[2] ^= state.s[0];
    state.s[3] ^= state.s[1];
    state.s[1] ^= state.s[2];
    state.s[0] ^= state.s[3];
    state.s[2] ^= t;
    state.s[3] = rotl(state.s[3], 45);
    return result;
}

// SplitMix64 seed init
static void xoshiro_seed(XoshiroState& state, std::uint64_t seed) {
    auto splitmix = [](std::uint64_t& z) {
        z += 0x9e3779b97f4a7c15ULL;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    };
    std::uint64_t z = seed;
    state.s[0] = splitmix(z);
    state.s[1] = splitmix(z);
    state.s[2] = splitmix(z);
    state.s[3] = splitmix(z);
}

static const char rng_tag_name[] = "Rng";

static TagObject* rng_tag(On1x_State* s) {
    return s->tags.intern(&s->gc, rng_tag_name);
}

static Value make_rng(On1x_State* s, const XoshiroState& st) {
    auto* state_list = new_list(&s->gc, 4);
    if (!state_list) {
        push_api_error(s, "Rand: allocation failed");
        return Value();
    }
    GcRoot root(state_list);
    list_push(&s->gc, state_list, Value::integer(&s->gc, static_cast<std::int64_t>(st.s[0])));
    list_push(&s->gc, state_list, Value::integer(&s->gc, static_cast<std::int64_t>(st.s[1])));
    list_push(&s->gc, state_list, Value::integer(&s->gc, static_cast<std::int64_t>(st.s[2])));
    list_push(&s->gc, state_list, Value::integer(&s->gc, static_cast<std::int64_t>(st.s[3])));
    auto* tag = rng_tag(s);
    auto* tl = new_tagged_list(&s->gc, tag, 1);
    if (!tl) {
        push_api_error(s, "Rand: allocation failed");
        return Value();
    }
    GcRoot tl_root(tl);
    list_push(&s->gc, tl, value_from_object(state_list));
    return value_from_object(tl);
}

static bool get_rng_state(On1x_State* s, Value v, XoshiroState& st) {
    if (v.kind() != Value::Kind::List) return false;
    const auto* list = as_list_const(v);
    if (!list || !list->constructor || list->constructor != rng_tag(s)) return false;
    if (list->length < 1) return false;
    Value state_val;
    if (!list_get(list, 0, state_val) || state_val.kind() != Value::Kind::List) return false;
    const auto* state_list = as_list_const(state_val);
    if (state_list->length < 4) return false;
    Value sv0, sv1, sv2, sv3;
    if (!list_get(state_list, 0, sv0) || !list_get(state_list, 1, sv1) ||
        !list_get(state_list, 2, sv2) || !list_get(state_list, 3, sv3))
        return false;
    if (!sv0.is_int() || !sv1.is_int() || !sv2.is_int() || !sv3.is_int()) return false;
    st.s[0] = static_cast<std::uint64_t>(sv0.as_int());
    st.s[1] = static_cast<std::uint64_t>(sv1.as_int());
    st.s[2] = static_cast<std::uint64_t>(sv2.as_int());
    st.s[3] = static_cast<std::uint64_t>(sv3.as_int());
    return true;
}

static Value step_rng(On1x_State* s, Value rng_val, std::uint64_t& result) {
    XoshiroState st;
    if (!get_rng_state(s, rng_val, st)) return Value();
    result = xoshiro_next(st);
    return make_rng(s, st);
}

static Value pair_result(On1x_State* s, Value new_rng, Value v) {
    auto* pair = new_list(&s->gc, 2);
    if (!pair) return Value();
    GcRoot pair_root(pair);
    list_push(&s->gc, pair, new_rng);
    list_push(&s->gc, pair, v);
    return value_from_object(pair);
}

// ---- Rand.New ----

On1x_Status rand_new(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Rand.New")) return ON1X_ERR;
    Value seed_v;
    if (!read_argument(s, 1, seed_v) || !seed_v.is_int())
        return bad(s, "Rand.New expects an Int seed");
    XoshiroState st;
    xoshiro_seed(st, static_cast<std::uint64_t>(seed_v.as_int()));
    Value rng = make_rng(s, st);
    if (rng.kind() != Value::Kind::List)
        return bad(s, "Rand.New: allocation failed");
    return stack_push(s, rng) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.Int ----

On1x_Status rand_int(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Rand.Int")) return ON1X_ERR;
    Value rng_v;
    if (!read_argument(s, 1, rng_v)) return bad(s, "Rand.Int expects a generator");
    std::uint64_t result = 0;
    Value new_rng = step_rng(s, rng_v, result);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.Int: invalid generator");
    Value int_val = Value::integer(&s->gc, static_cast<std::int64_t>(result));
    Value pair = pair_result(s, new_rng, int_val);
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.Int: allocation failed");
    return stack_push(s, pair) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.IntBelow ----

On1x_Status rand_int_below(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Rand.IntBelow")) return ON1X_ERR;
    Value rng_v, n_v;
    if (!read_argument(s, 1, rng_v) || !read_argument(s, 2, n_v))
        return bad(s, "Rand.IntBelow expects a generator and a bound");
    if (!n_v.is_int() || n_v.as_int() <= 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::int64_t limit = n_v.as_int();
    std::uint64_t result = 0;
    Value new_rng = step_rng(s, rng_v, result);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.IntBelow: invalid generator");
    Value int_val = Value::integer(&s->gc, static_cast<std::int64_t>(result % static_cast<std::uint64_t>(limit)));
    Value pair = pair_result(s, new_rng, int_val);
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.IntBelow: allocation failed");
    Value some_pair = make_some(&s->gc, s->reserved, pair);
    return stack_push(s, some_pair) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.Range ----

On1x_Status rand_range(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 3, "Rand.Range")) return ON1X_ERR;
    Value rng_v, lo_v, hi_v;
    if (!read_argument(s, 1, rng_v) || !read_argument(s, 2, lo_v) || !read_argument(s, 3, hi_v))
        return bad(s, "Rand.Range expects a generator and two bounds");
    if (!lo_v.is_int() || !hi_v.is_int())
        return bad(s, "Rand.Range expects Int bounds");
    std::int64_t lo = lo_v.as_int();
    std::int64_t hi = hi_v.as_int();
    if (lo >= hi) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::uint64_t result = 0;
    Value new_rng = step_rng(s, rng_v, result);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.Range: invalid generator");
    std::int64_t val = lo + static_cast<std::int64_t>(result % static_cast<std::uint64_t>(hi - lo));
    Value int_val = Value::integer(&s->gc, val);
    Value pair = pair_result(s, new_rng, int_val);
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.Range: allocation failed");
    Value some_pair = make_some(&s->gc, s->reserved, pair);
    return stack_push(s, some_pair) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.Float ----

On1x_Status rand_float(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Rand.Float")) return ON1X_ERR;
    Value rng_v;
    if (!read_argument(s, 1, rng_v)) return bad(s, "Rand.Float expects a generator");
    std::uint64_t result = 0;
    Value new_rng = step_rng(s, rng_v, result);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.Float: invalid generator");
    double float_val = static_cast<double>(result >> 11) * 0x1.0p-53;
    Value pair = pair_result(s, new_rng, Value::floating(float_val));
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.Float: allocation failed");
    return stack_push(s, pair) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.Bool ----

On1x_Status rand_bool(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Rand.Bool")) return ON1X_ERR;
    Value rng_v;
    if (!read_argument(s, 1, rng_v)) return bad(s, "Rand.Bool expects a generator");
    std::uint64_t result = 0;
    Value new_rng = step_rng(s, rng_v, result);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.Bool: invalid generator");
    Value pair = pair_result(s, new_rng, Value::boolean((result & 1) != 0));
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.Bool: allocation failed");
    return stack_push(s, pair) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.Choice ----

On1x_Status rand_choice(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Rand.Choice")) return ON1X_ERR;
    Value rng_v, list_v;
    if (!read_argument(s, 1, rng_v) || !read_argument(s, 2, list_v))
        return bad(s, "Rand.Choice expects a generator and a List");
    if (list_v.kind() != Value::Kind::List)
        return bad(s, "Rand.Choice expects a generator and a List");
    const auto* list = as_list_const(list_v);
    if (list->length == 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    std::uint64_t result = 0;
    Value new_rng = step_rng(s, rng_v, result);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.Choice: invalid generator");
    std::size_t idx = static_cast<std::size_t>(result % list->length);
    Value item;
    list_get(list, idx, item);
    Value pair = pair_result(s, new_rng, item);
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.Choice: allocation failed");
    Value some_pair = make_some(&s->gc, s->reserved, pair);
    return stack_push(s, some_pair) ? ON1X_OK : ON1X_ERR;
}

// ---- Rand.Shuffle ----

On1x_Status rand_shuffle(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Rand.Shuffle")) return ON1X_ERR;
    Value rng_v, list_v;
    if (!read_argument(s, 1, rng_v) || !read_argument(s, 2, list_v))
        return bad(s, "Rand.Shuffle expects a generator and a List");
    if (list_v.kind() != Value::Kind::List)
        return bad(s, "Rand.Shuffle expects a generator and a List");
    const auto* src = as_list_const(list_v);
    auto* dst = new_list(&s->gc, src->length);
    if (!dst) return bad(s, "Rand.Shuffle: allocation failed");
    GcRoot dst_root(dst);
    for (std::size_t i = 0; i < src->length; ++i) {
        Value item;
        list_get(src, i, item);
        list_push(&s->gc, dst, item);
    }
    XoshiroState st;
    if (!get_rng_state(s, rng_v, st)) return bad(s, "Rand.Shuffle: invalid generator");
    for (std::size_t i = dst->length; i > 1; --i) {
        std::uint64_t r = xoshiro_next(st);
        std::size_t j = static_cast<std::size_t>(r % i);
        Value vi, vj;
        list_get(dst, i - 1, vi);
        list_get(dst, j, vj);
        list_set(dst, i - 1, vj);
        list_set(dst, j, vi);
    }
    Value new_rng = make_rng(s, st);
    if (new_rng.kind() != Value::Kind::List) return bad(s, "Rand.Shuffle: allocation failed");
    Value pair = pair_result(s, new_rng, value_from_object(dst));
    if (pair.kind() != Value::Kind::List) return bad(s, "Rand.Shuffle: allocation failed");
    return stack_push(s, pair) ? ON1X_OK : ON1X_ERR;
}

const On1x_FnDesc fns[] = {
    {"New", rand_new},
    {"Int", rand_int},
    {"IntBelow", rand_int_below},
    {"Range", rand_range},
    {"Float", rand_float},
    {"Bool", rand_bool},
    {"Choice", rand_choice},
    {"Shuffle", rand_shuffle},
#if !ON1X_STDLIB_PURE_ONLY
    {"SeedFromSystem", rand_detail::rand_seed_from_system},
#else
    {"SeedFromSystem", rand_seed_from_system_fallback},
#endif
};

const On1x_ModuleDesc desc{"Rand", ON1X_CAP_NONE, fns, sizeof(fns) / sizeof(*fns)};

}  // namespace

const On1x_ModuleDesc* rand_module() noexcept { return &desc; }

}  // namespace on1x::stdlib
