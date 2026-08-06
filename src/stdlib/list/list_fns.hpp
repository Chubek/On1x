#pragma once

#include <on1x/on1x_types.h>

struct On1x_State;

namespace on1x::stdlib {

// Construction / slicing / concat / copy (list.cpp)
On1x_Status list_new_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_copy_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_concat_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_append_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_slice_fn(On1x_State* s, int argc) noexcept;
 
 // Access (access.cpp)
 On1x_Status list_first_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_last_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_take_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_drop_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_insert_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_remove_fn(On1x_State* s, int argc) noexcept;
 On1x_Status list_reverse_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_reverse_in_place_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_fill_fn(On1x_State* s, int argc) noexcept;

// Higher-order (higher_order.cpp)
On1x_Status list_map_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_filter_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_reduce_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_for_each_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_any_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_all_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_find_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_find_index_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_partition_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_index_of_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_count_fn(On1x_State* s, int argc) noexcept;

// Sort (sort.cpp)
On1x_Status list_sort_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_sort_by_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_is_sorted_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_unique_fn(On1x_State* s, int argc) noexcept;

// Zip (zip.cpp)
On1x_Status list_zip_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_unzip_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_enumerate_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_flatten_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_chunk_fn(On1x_State* s, int argc) noexcept;
On1x_Status list_window_fn(On1x_State* s, int argc) noexcept;

}  // namespace on1x::stdlib
