#include <on1x/on1x.h>
#include <on1x/on1x_config.h>
#include <on1x/on1x_export.h>
#include <on1x/on1x_types.h>

int main(void) {
    On1x_Capability capability = ON1X_CAP_NONE;
    return capability == ON1X_CAP_NONE ? 0 : 1;
}
