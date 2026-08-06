#include <on1x/on1x.hpp>
#include <on1x/on1x_config.h>

int main() {
    On1x_Capability capability = ON1X_CAP_NONE;
    return capability == ON1X_CAP_NONE ? 0 : 1;
}
