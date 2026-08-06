#include "runtime/closure.hpp"

#include "gc/alloc.hpp"
#include "gc/roots.hpp"

namespace on1x::runtime {

FunctionObject* new_native_function(GcState* gc, On1x_CFn native) {
    auto* function = gc_alloc<FunctionObject>(gc);
    function->native = native;
    return function;
}

FunctionObject* new_closure(
    GcState* gc,
    vm::Chunk* chunk,
    const Value* captures,
    std::size_t capture_count) {
    auto* function = gc_alloc<FunctionObject>(gc);
    GcRoot function_root(function);
    function->chunk = chunk;
    function->capture_count = capture_count;
    if (capture_count != 0U) {
        function->captures = gc_alloc_array<Value>(gc, capture_count);
        for (std::size_t index = 0; index < capture_count; ++index) {
            function->captures[index] = captures[index];
        }
    }
    return function;
}

}  // namespace on1x::runtime
