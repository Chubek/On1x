#include "stdlib/embed.hpp"
 #include <cstring>
 
 // Defined in generated embedded_sources.gen.cpp.
 extern const on1x::stdlib::EmbeddedSource embedded_sources[];
 extern const std::size_t g_embedded_source_count;
 
 namespace on1x::stdlib {
 
 const char* embedded_source_text(const char* name) noexcept {
     if (!name) return nullptr;
     for (std::size_t i = 0; i < g_embedded_source_count; ++i) {
         if (std::strcmp(embedded_sources[i].name, name) == 0) {
             return embedded_sources[i].text;
         }
     }
     return nullptr;
 }
 
 std::size_t embedded_source_count() noexcept {
     return g_embedded_source_count;
 }
 
 bool install_embedded_sources(On1x_State* /*state*/) noexcept {
     // Embedded On1x source modules will be installed once the
     // parser/evaluator pipeline is exposed to stdlib modules.
     // For now, sources are accessible via embedded_source_text().
     return true;
 }
 
 }  // namespace on1x::stdlib
