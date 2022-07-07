//  Copyright (c) 2022 Lemurian Labs https://www.lemurianlabs.com/
//
//  Distributed under the Boost Software License, Version 1.0.
//        http://www.boost.org/LICENSE_1_0.txt
//
//  Repo: https://github.com/Lemurian-Labs/lml_parameta

#ifndef LML_PARAMETA_HPP
#define LML_PARAMETA_HPP

/*
  parameta.hpp: meta types for parameterization
  ============

  This header defines:

    * parameta<v> and dynameta<T>: types representing static and dynamic values.
      As 'meta value types' they generalize the API of std::integral_constant.
      parameta<v,x...> and dynameta<T,x...> can carry variadic metadata x...

    * typemeta<T,x...> meta type that represents type T with optional metadata.
      Like std::type_identity, plus the possible extra variadic metadata x...

    * pv<v> and ty<T>: variable template abbreviations, saving eight characters:

      pv<v> == parameta<v>{} // abbreviated static parameta variable
      ty<T> == typemeta<T>{} // abbreviated typemeta object variable

  parameta<v> denotes a static or parameter value that, like integral_constant,
  has its value v statically encoded, manifestly usable as a template argument.
  As an empty type it can be laid out to occupy no storage in a class.

  Note: 'integral_constant' is a misnomer; it's not constrained to carry values
      of integral types only; any valid non-type template argument is admitted
      including the address or 'id' of static-storage variables, const or not.

  dynameta<T> denotes a dynamic or deferred value that has the same access API
  as parameta but its signature carries only the type T of the value, plus meta-
  data needed to determine its runtime storage, initialization, access and use.

  "Dynamic" implies that the value isn't know statically at compile-time so is
  deferred to runtime, i.e. dynamic initialization is assumed and the runtime-
  determined value can't be used as a template argument or encoded in the type.

  "Dynamic" doesn't imply mutability; a dynameta usually models immutable data -
  dynameta is a runtime-determined constant, parameta a compile-time constant.

  "Static" doesn't imply immutability; non-pure parameta refer to mutable data.

  Usage

  Both parameta and dynameta are intended as meta value types to be used as
  *type* template arguments. The types can carry unlimited variadic metadata.
  By using meta-types instead of values, the extra level of indirection allows
  a uniform and rich treatment of static and dynamic values, and of types.

  Contrast with std::dynamic_extent, a special value of size_t type that's used
  to select between static and dynamic specialization of std::span and mdspan.
  There's no clean way to extend a 'special value' API or to make it generic
  e.g. to specify a type other than size_t or to represent repeated extents.

  Examples

  The metavalue concept matches any dynameta-or-parameta-like meta-value type:

      metavalue auto value0 = 0;      // FAIL: 0 is an int, not a metavalue type
      metavalue auto value1 = dynameta{1};
      metavalue auto value2 = parameta<2>{};

  The value type can be constrained by adding a type T argument, metavalue<T>:

      metavalue<char> auto nchar = parameta<0x0>{}; // FAIL: 0x0 is not a char
      metavalue<char> auto char0 = parameta<'0'>{};
      metavalue<char> auto stdc0 = std::integral_constant<char,0>{}; // 0 -> '0'

  The metastatic concept matches only parameta-like 'static' meta-value types:

      metastatic auto dynNo = dynameta{4}; // FAIL: dynameta isn't metastatic
      const float mf[4][4]{};
      metastatic<float[4][4]> auto mfref = pv<(mf)>; // refers to static var mf

  Here, mfref refers to a non-constant-expression variable so is not pure.
  The metaconst 'pure static value' concept matches only pure parameta types:

      metaconst auto mfref = pv<(mf)>;  // FAIL: mf value isn't constexpr
      metaconst auto pi = pv<3.14159f>;

  Summary

  typemeta<T>, a type meta-type like std::type_identity, is joined by
  dynameta<T> and parameta<v>, value meta-types like std::integral_constant
  with optional extra metadata x...

    typemeta<T,x...> "meta type"; type = T

    dynameta<T,x...> "dynamic value"; value_type = T, value = runtime-determined
    parameta<v,x...> "static value"; value_type = decltype(v), value = v

  Signatures:

  dynameta:                                  "dynameta"
      template                                 |
      <typename T, decltype(auto)...x> struct dynameta {T value; value_type = T;
                |                   |                            |
          value_type              ['xtra' x...]               typedef
  parameta:                               |                      |
      template                            |                      |
      <decltype(auto) v, decltype(auto)...x> struct parameta { value_type = ...
                      |                              |           |
             NTTP arg value                        "parameta"  decltype(v)

  API:

  parameta<v> and dynameta<T> share the core API of std::integral_constant<T,v>:

    * A 'value_type' member type alias
    * A 'value' member variable of value_type, possibly const qualified
    * A no-arg call operator()()const -> value_type, returning the value
    * An implicit conversion operator value_type()const returning the value

  Any extra x... arguments are accessed by get<I> or xtra<I>

    * get<I>(metavalue)  returns the Ith v,x... argument
    * xtra<I>(metavalue) returns the Ith   x... argument == get<I+1>(metavalue)
*/

#include "parameta_traits.hpp"

#if defined (__cpp_consteval)
#   define CONSTEVAL consteval
#else
#   define CONSTEVAL constexpr
#endif

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

/* ************************************************************************ */
/* parameta<v,x...> generalizes std::integral_constant<T,v>
                - it eliminates the type parameter T
                - it admits variadic 'xtra' NTTP metadata x...

  parameta<v> encodes a value v, by value
              or a const& v, by id

  parameta 'parameter value' is a static meta-value type;
  an NTTP-templated type that encodes a value in its NTTP
  (non-type template parameter).

  The static constexpr member variable 'value' memos the parameter value v.
  The member type alias value_type = decltype(v) can be an lvalue reference.
  The call operator()()const takes no arguments and returns value_type.
  There is implicit conversion to value_type.
*/
template <decltype(auto) v, decltype(auto)...x>
struct parameta
{
  static_assert( const_if_reference_v<decltype(v)>
             && (const_if_reference_v<decltype(x)> && ...),
    "Reference-type value and metadata must be const& - use as_const(v)");

  using type = parameta;
  using value_type = decltype(v); // may be an lvalue reference type
                                  // (but not an rvalue rererence)
  static constexpr value_type value = v;

  /* static */ // c++23 static operator() https://wg21.link/p1169
  constexpr value_type operator()() const noexcept {return v;}
  constexpr operator value_type() const noexcept {return v;}

  /* size(pv) and get<I>(pv) should be free, clang doesn't agree */

  friend CONSTEVAL auto size(parameta) { return 1 + sizeof...(x); }

  template <unsigned I>
  friend CONSTEVAL decltype(auto) get(parameta)
  {
    static_assert(I <= sizeof...(x), "get<I> index out of bounds");
    if constexpr (I == 0) return (v);
    else return get<I-1>(parameta<(x)...>{});
  }
};

// pv variable template for a parameta object
//
template <decltype(auto) v, decltype(auto)...x>
inline constexpr parameta<(v),(x)...> pv{};

/* ************************************************************************ */

template <typename T, decltype(auto)...x>
struct dynameta
{
  static_assert((const_if_reference_v<decltype(x)> && ...),
    "Reference-type metadata must be const& - use as_const(x)");

  using type = dynameta;
  using value_type = T;

  value_type value;

  constexpr value_type operator()() const noexcept {return value;}
  constexpr operator value_type() const noexcept {return value;}

  /* size(pv) and get<I>(pv) should be free, clang doesn't agree */

  friend CONSTEVAL auto size(dynameta) { return 1 + sizeof...(x); }

#if __cpp_concepts
  template <unsigned I>
    requires (I == 0)
  friend constexpr value_type get(dynameta dv) { return dv(); }
  //
  template <unsigned I>
    requires (I > 0 && I <= sizeof...(x))
  friend CONSTEVAL decltype(auto) get(dynameta) {
    return get<I-1>(parameta<(x)...>{});
  }
#else
  template <unsigned I>
  friend constexpr decltype(auto) get(dynameta dv)
  {
    if constexpr (I == 0) return dv();
    else return get<I-1>(parameta<(x)...>{});
  }
#endif
};
template <typename T> dynameta(T const&) -> dynameta<T>;

/* ****************** typemeta  ************************* */

// typemeta<T,x...> like std::type_identity, plus xtra metadata x...
//
template <typename T, decltype(auto)...x>
struct typemeta
{
  static_assert((const_if_reference_v<decltype(x)> && ...),
    "Use as_const: reference type metadata must be const&");

  using type = T;

  /* -- size(dv) and get<I>(dv) could be free, better hidden -- */

  friend CONSTEVAL auto size(typemeta) { return 1 + sizeof...(x); }

  template <unsigned I>
  friend CONSTEVAL decltype(auto) get(typemeta)
  {
    static_assert(I > 0 && I <= sizeof...(x), "get<I> index out of bounds");
    return get<I-1>(parameta<(x)...>{});
  }
};

// ty variable template for a typemeta object
//
template <typename T, decltype(auto)...x>
inline constexpr typemeta<T,(x)...> ty{};

/* ************************************************************************ */

// xtra<I>(metavalue) get the Ith metadata of a parameta
// xtra<I>(metatype) get the Ith metadata of a metatype
//
#  if __cpp_concepts

template <unsigned I, metavalue v>
CONSTEVAL decltype(auto) xtra(v={}) { return get<I+1>(v{}); }

template <unsigned I, metatype t>
CONSTEVAL decltype(auto) xtra(t={}) { return get<I+1>(t{}); }

// type_t type alias to extract the concrete type from the meta type
//
template <metatype T> using type_t = typename T::type;

// vtype_t type alias to extract the concrete type from the meta type
//
template <metavalue T> using vtype_t = typename T::value_type;

#else

template <unsigned I, typename v, std::enable_if_t<is_val_v<v>>* = nullptr>
CONSTEVAL decltype(auto) xtra(v = {})
  { return get<I+1>(v{}); }

template <unsigned I, typename t, std::enable_if_t<is_typ_v<t>>* = nullptr>
CONSTEVAL decltype(auto) xtra(t = {})
  { return get<I+1>(t{}); }

// type_t type alias to extract the concrete type from the meta type
//
template <typename t, std::enable_if_t<is_typ_v<t>>* = nullptr>
using type_t = typename t::type;

// vtype_t type alias to extract the concrete type from the meta type
//
template <typename v, std::enable_if_t<is_val_v<v>>* = nullptr>
using vtype_t = typename v::value_type;

// get function declaration needed to find hidden friend get templates in C++17
// fixed in C++20 by P0846 but there's no feature test macro to compile it out
// (clang only warns about use of a C++20 extension, gcc fails to compile).
//
template <unsigned I, decltype(auto)...>
extern CONSTEVAL decltype(auto) get();  // bogus declaration for ADL in C++17
#endif

/* ************************************************************************ */

#include "namespace.hpp" // close configurable namespace

#endif
