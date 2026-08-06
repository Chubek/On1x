#include <on1x/on1x.h>
#include <cstdio>
#include <cstring>
#include <cmath>

int main() {
    On1x_State* state = on1x_open();
    on1x_open_std(state);
    
    // Test Math.Sqrt(4) stack behavior
    {
        const char* src = "Math.Sqrt(4)";
        on1x_eval(state, src, std::strlen(src), "test");
        printf("After Sqrt(4): top=%d type=%d is_some=%d\n", on1x_top(state), on1x_type(state,-1), on1x_is_some(state,-1));
        int po = on1x_payload_of(state, -1);
        printf("  payload_of=%d top=%d type=%d\n", po, on1x_top(state), on1x_type(state,-1));
        int lg = on1x_list_get(state, -1, 0);
        printf("  list_get[0]=%d top=%d type=%d\n", lg, on1x_top(state), on1x_type(state,-1));
        double v = on1x_as_float(state, -1);
        printf("  float=%f\n", v);
        on1x_pop(state, 20);
    }
    
    // Test Bit.Test(8, 3) stack behavior
    {
        const char* src = "Bit.Test(8, 3)";
        on1x_eval(state, src, std::strlen(src), "test");
        printf("After Test(8,3): top=%d type=%d is_some=%d\n", on1x_top(state), on1x_type(state,-1), on1x_is_some(state,-1));
        int po = on1x_payload_of(state, -1);
        printf("  payload_of=%d top=%d type=%d\n", po, on1x_top(state), on1x_type(state,-1));
        int lg = on1x_list_get(state, -1, 0);
        printf("  list_get[0]=%d top=%d type=%d\n", lg, on1x_top(state), on1x_type(state,-1));
        on1x_pop(state, 20);
    }
    
    // Test Tag.Name stack behavior
    {
        const char* src = "Tag.Name(:point)";
        on1x_eval(state, src, std::strlen(src), "test");
        printf("After Tag.Name: top=%d type=%d\n", on1x_top(state), on1x_type(state,-1));
        on1x_pop(state, 20);
    }
    
    on1x_close(state);
    return 0;
}
