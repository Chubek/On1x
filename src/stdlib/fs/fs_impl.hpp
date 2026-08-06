#pragma once

#include <on1x/on1x_types.h>

struct On1x_State;

namespace on1x::stdlib::fs_detail {

On1x_Status fs_listdir(On1x_State* s, int argc) noexcept;
On1x_Status fs_makedir(On1x_State* s, int argc) noexcept;
On1x_Status fs_makedir_all(On1x_State* s, int argc) noexcept;
On1x_Status fs_removedir(On1x_State* s, int argc) noexcept;
On1x_Status fs_isdir(On1x_State* s, int argc) noexcept;
On1x_Status fs_isfile(On1x_State* s, int argc) noexcept;
On1x_Status fs_size(On1x_State* s, int argc) noexcept;
On1x_Status fs_metadata(On1x_State* s, int argc) noexcept;

}  // namespace on1x::stdlib::fs_detail
