#pragma once

#if defined(_WIN32) && defined(ON1X_BUILD_SHARED)
#  if defined(ON1X_BUILDING_LIBRARY)
#    define ON1X_API __declspec(dllexport)
#  else
#    define ON1X_API __declspec(dllimport)
#  endif
#else
#  define ON1X_API
#endif
