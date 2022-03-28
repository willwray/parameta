#include "parameta.hpp"

#ifdef _MSC_VER
 # define MSVC_OR(M,...) M
#else
 # define MSVC_OR(M,...) __VA_ARGS__
#endif
