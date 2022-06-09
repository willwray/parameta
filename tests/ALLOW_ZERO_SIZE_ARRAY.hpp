#if !defined(ALLOW_ZERO_SIZE_ARRAY)

#    define  ALLOW_ZERO_SIZE_ARRAY

#if defined(__clang__) || defined(__GNUG__)
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#elif defined(_MSC_VER)
__pragma(warning(push))
__pragma(warning(disable:4200)) // L2: nonstandard extension used: zero-sized array in struct/union
__pragma(warning(disable:4815)) // zero-sized array in stack object will have no elements...
__pragma(warning(disable:4816)) // L4: parameter has a zero-sized array which will be truncated ...
#else
#endif

#else

# undef ALLOW_ZERO_SIZE_ARRAY

#if defined(__clang__) || defined(__GNUG__)
_Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
__pragma(warning(pop))
#else
#endif

#endif
