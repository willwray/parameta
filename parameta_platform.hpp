/*
 SPDX-FileCopyrightText: 2024 The Lemuriad <wjwray@gmail.com>
 SPDX-License-Identifier: BSL-1.0
 Repo: https://github.com/Lemuriad/parameta
*/

// "parameta_platform.hpp" platform-specific macros
// for conditional C++23 feature support in earlier std and C++17 backport

#if ! defined(CONSTEVAL) \
 && ! defined(STATIC_CALL_OP_IF_AVAILABLE) \
 && ! defined(typeof)

// CONSTEVAL
// Use consteval if available else fall back to constexpr for C++17
//          (exclude MSVC due to false 'not constant expression')
#if defined (__cpp_consteval) && ! defined (_MSC_VER)
#   define CONSTEVAL consteval
#else
#   define CONSTEVAL constexpr
#endif

// STATIC_CALL_OP_IF_AVAILABLE
// Use static call operator() if available pre C++23 https://wg21.link/p1169
//
#if defined (__cpp_static_call_operator)
#  if defined(__clang__) || defined(__GNUG__)
#    define STATIC_CALL_OP_IF_AVAILABLE(decl,body)\
_Pragma("GCC diagnostic push")\
_Pragma("GCC diagnostic ignored \"-Wc++23-extensions\"")\
 static decl body \
_Pragma("GCC diagnostic pop")
#  else
#    define STATIC_CALL_OP_IF_AVAILABLE(decl,body) static decl body
#  endif
#else
#   define STATIC_CALL_OP_IF_AVAILABLE(decl,body) decl const body
#endif

// typeof(T) convenience, is in C23, possibly in C++26
#define typeof(...)std::remove_reference_t<decltype(__VA_ARGS__)>

#else

#undef CONSTEVAL
#undef STATIC_CALL_OP_IF_AVAILABLE
#undef typeof

#endif
