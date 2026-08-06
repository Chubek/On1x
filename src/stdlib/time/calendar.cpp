#include "stdlib/time/time_impl.hpp"

#include "api/api_common.hpp"
#include "core/optional.hpp"
#include "core/string.hpp"
#include "core/table.hpp"
#include "core/tag_table.hpp"
#include "gc/roots.hpp"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

namespace on1x::stdlib::time_detail {

static On1x_Status bad(On1x_State* s, const char* m) noexcept {
    return push_api_error(s, m);
}

// Breakdown(ts: :Int) -> :Table with :Year :Month :Day :Hour :Minute :Second :Weekday
// UTC only in 0.1.0.
On1x_Status time_breakdown(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Time.Breakdown")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || !v.is_int())
        return bad(s, "Time.Breakdown expects an Int timestamp");
    std::int64_t ts = v.as_int();
    std::time_t t = static_cast<std::time_t>(ts);
    struct tm utc_tm;
    if (!gmtime_r(&t, &utc_tm)) {
        return bad(s, "Time.Breakdown: gmtime_r failed");
    }
    auto* table = new_table(&s->gc);
    if (!table) return bad(s, "Time.Breakdown: allocation failed");
    GcRoot table_root(table);

    auto set_field = [&](const char* name, std::int64_t val) {
        auto* key_tag = s->tags.intern(&s->gc, name);
        Value int_val = Value::integer(&s->gc, val);
        GcRoot kr(key_tag), vr; // int_val is an immediate, no root needed
        return table_set(&s->gc, table, value_from_object(key_tag), int_val);
    };

    if (!set_field("Year",    static_cast<std::int64_t>(utc_tm.tm_year + 1900)) ||
        !set_field("Month",   static_cast<std::int64_t>(utc_tm.tm_mon + 1)) ||
        !set_field("Day",     static_cast<std::int64_t>(utc_tm.tm_mday)) ||
        !set_field("Hour",    static_cast<std::int64_t>(utc_tm.tm_hour)) ||
        !set_field("Minute",  static_cast<std::int64_t>(utc_tm.tm_min)) ||
        !set_field("Second",  static_cast<std::int64_t>(utc_tm.tm_sec)) ||
        !set_field("Weekday", static_cast<std::int64_t>(utc_tm.tm_wday))) {
        return bad(s, "Time.Breakdown: table set failed");
    }

    return stack_push(s, value_from_object(table)) ? ON1X_OK : ON1X_ERR;
}

// Format(ts, fmt) -> :Some[:String] | :None
// Simple strftime wrapper. Returns :None on error.
On1x_Status time_format(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 2, "Time.Format")) return ON1X_ERR;
    Value ts_v, fmt_v;
    if (!read_argument(s, 1, ts_v) || !ts_v.is_int())
        return bad(s, "Time.Format expects an Int timestamp");
    if (!read_argument(s, 2, fmt_v) || fmt_v.kind() != Value::Kind::String)
        return bad(s, "Time.Format expects a String format");
    std::int64_t ts = ts_v.as_int();
    std::time_t t = static_cast<std::time_t>(ts);
    struct tm utc_tm;
    if (!gmtime_r(&t, &utc_tm)) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto fmt_text = string_view(as_string_const(fmt_v));
    std::string fmt_str(fmt_text);
    char buf[256];
    if (strftime(buf, sizeof(buf), fmt_str.c_str(), &utc_tm) == 0) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    auto* str = new_string(&s->gc, buf);
    if (!str) return bad(s, "Time.Format: allocation failed");
    return stack_push(s, make_some(&s->gc, s->reserved, value_from_object(str))) ? ON1X_OK : ON1X_ERR;
}

// Parse(s) -> :Some[:Int] | :None
// Parses ISO 8601 subset: "YYYY-MM-DD" or "YYYY-MM-DDTHH:MM:SS"
On1x_Status time_parse(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Time.Parse")) return ON1X_ERR;
    Value v;
    if (!read_argument(s, 1, v) || v.kind() != Value::Kind::String)
        return bad(s, "Time.Parse expects a String");
    auto text = string_view(as_string_const(v));
    std::string input(text);

    struct tm utc_tm = {};
    // Try ISO 8601 formats
    int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;
    int n = 0;
    // Try "YYYY-MM-DDTHH:MM:SS"
    n = sscanf(input.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &min, &sec);
    if (n < 3) {
        // Try "YYYY-MM-DD"
        n = sscanf(input.c_str(), "%d-%d-%d", &year, &month, &day);
        hour = 0; min = 0; sec = 0;
    }
    if (n < 3) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }

    utc_tm.tm_year = year - 1900;
    utc_tm.tm_mon  = month - 1;
    utc_tm.tm_mday = day;
    utc_tm.tm_hour = hour;
    utc_tm.tm_min  = min;
    utc_tm.tm_sec  = sec;
    utc_tm.tm_isdst = 0;

    std::time_t t = timegm(&utc_tm);
    if (t == static_cast<std::time_t>(-1)) {
        return stack_push(s, make_none(&s->gc, s->reserved)) ? ON1X_OK : ON1X_ERR;
    }
    return stack_push(s, make_some(&s->gc, s->reserved,
        Value::integer(&s->gc, static_cast<std::int64_t>(t)))) ? ON1X_OK : ON1X_ERR;
}

}  // namespace on1x::stdlib::time_detail
