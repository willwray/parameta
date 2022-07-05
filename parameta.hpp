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

    * pval<v> and dval<T> - types representing 'static' and 'dynamic' values.
      As 'meta value types' they generalize the API of std::integral_constant.
      Both pval<v,x...> and dval<T,x...> can carry extra variadic metadata x...
      Let's call them parameta types.

    * metatype<T,x...> meta type that represents type T with optional metadata.
      Like std::type_identity, plus possible extra variadic metadata x...

    * pv<v> and ty<T>: variable template abbreviations 

      pv<v> == pval<v>{}     // save 4 chars, abbreviated static parameta
      ty<T> == metatype<T>{} // save 8 chars, abbreviated metatype object

  pval<v> denotes a 'static' or 'parameter' value that, like integral_constant,
  has its value v statically encoded, manifestly usable as a template argument.
  As an empty type it can be laid out to occupy no storage in a class.

  Note: 'integral_constant' is a misnomer; it's not constrained to carry values
      of integral types only; any valid non-type template argument is admitted
      including the address or 'id' of static-storage variables, const or not.
      It is a parameta type.

  dval<T> denotes a 'dynamic' or 'deferred' value that has the same access API
  as pval but its signature carries only the type T of the value, plus any meta
  data needed to determine its runtime storage, initialization, access and use.

  "Dynamic" implies that the value isn't know statically at compile-time so is
  deferred to runtime, i.e. dynamic initialization is assumed and the runtime-
  determined value can't be used as a template argument or encoded in the type.

  "Dynamic" doesn't imply mutability; a dval usually models immutable data
  dval is a runtime-determined constant while pval is a compile-time constant.

  "Static" doesn't imply immutability; non-pure pvals refer to mutable data
  (note that an NTTP referent variable has to have static storage duration).

  Usage

  Both pval and dval are primarily intended as meta value types to be used as
  *type* template arguments. The types can carry unlimited variadic metadata.
  By using meta-types instead of values, the extra level of indirection allows
  a uniform and rich treatment of static and dynamic values, and of types.

  Contrast with std::dynamic_extent, a special value of size_t type that's used
  to select between static and dynamic specialization of std::span and mdspan.
  There's no clean way to extend a 'special value' API or to make it generic
  e.g. to specify a type other than size_t or to represent repeated extents.

  Examples

  The val 'value' concept matches any dval-or-pval-like meta value type:

      val auto value0 = 0;      // FAIL: 0 is an int, not a val meta value type
      val auto value1 = dval{1};
      val auto value2 = pval<2>{};

  The value type can be constrained by adding a type T argument, val<T>:

      val<char> auto nchar = pval<0x0>{}; // FAIL: 0 is not a char
      val<char> auto char0 = pval<'0'>{};
      val<char> auto stdval = std::integral_constant<char,0>{}; // 0 -> '0'

  The s_val 'static value' concept matches only pval-like meta value types:

      s_val auto dynNo = dval{4}; // FAIL: dval isn't s_val; dynamic not static
      s_val auto statY = pv<5>;
      const float flo[4][4]{};
      s_val<float[4][4]> auto floref = pv<(flo)>; // refers to global var flo

  Here, floref refers to a non-constant-expression variable so is not pure.
  The ps_val 'pure static value' concept matches only pure pval types:

      ps_val auto floref = pv<(flo)>;    // FAIL: flo value isn't constexpr
      ps_val auto pi = pval<3.14159f>{};

  Summary

  metatype<T>, a type meta-type like std::type_identity, is joined by
  dval<T> and pval<v>, value meta-types like std::integral_constant
  with optional extra metadata x...

    metatype<T,x...> "meta type"; type = T

    dval<T,x...>  "dynamic value"; value_type = T, value = runtime-determined
    pval<v,x...> "parameter value"; value_type = decltype(v), value = v

  Signatures:

  dval:                                      "dval"
      template                                 |
      <typename T, decltype(auto)...x> struct dval {T value; value_type = T; };
                |                   |                            |
          value_type              ['xtra' x...]               typedef
  pval:                                   |                      |
      template                            |                      |
      <decltype(auto) v, decltype(auto)...x> struct pval { value_type = ...
                      |                              |           |
             NTTP arg value                        "pval"  decltype(v)

  API:

  pval<v> and dval<T> share the common API of std::integral_constant<T,v>:

    * A 'value_type' member type alias
    * A 'value' member variable of value_type, possibly const qualified
    * A no-arg call operator()()const -> value_type, returning the value
    * An implicit conversion operator value_type()const returning the value

  Any extra x... arguments are accessed by get<I> or xtra<I>

    * get<I>(val)  returns the Ith v,x... argument
    * xtra<I>(val) returns the Ith   x... argument == get<I+1>(val)
*/

#include "parameta_traits.hpp"

#if defined (__cpp_consteval)
#   define CONSTEVAL consteval
#else
#   define CONSTEVAL constexpr
#endif

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

// get function declaration needed to find hidden friend get templates in C++17
// fixed in C++20 by P0846 but there's no feature test macro to compile it out
// (clang only warns about use of a C++20 extension, gcc fails to compile).
//
template <unsigned I, decltype(auto)...>
extern CONSTEVAL decltype(auto) get();  // bogus declaration for ADL, see above

/* ************************************************************************ */
/* pval<v,x...> generalizes std::integral_constant<T,v>
                - it eliminates the type parameter T
                - it admits variadic 'xtra' NTTP metadata x...

  pval<v> encodes a value v, by value
              or a const& v, by id

  pval 'parameter value' is a static meta value type; an NTTP-templated type
  that encodes a value in its non-type template parameter.

  The static constexpr member variable 'value' memos the parameter value v.
  The member type alias value_type = decltype(v) can be an lvalue reference.
  The call operator()()const takes no arguments and returns value_type.
  There is implicit conversion to value_type.
*/
template <decltype(auto) v, decltype(auto)...x>
struct pval
{
  static_assert( const_if_reference_v<decltype(v)>
             && (const_if_reference_v<decltype(x)> && ...),
    "Use as_const: reference type value and metadata must be const&");

  using type = pval;
  using value_type = decltype(v); // may be an lvalue reference type
                                  // (but not an rvalue rererence)
  static constexpr value_type value = v;

  /* static */ // c++23 static operator() https://wg21.link/p1169
  constexpr value_type operator()() const noexcept {return v;}
  constexpr operator value_type() const noexcept {return v;}

  /* -- size(pv) and get<I>(pv) could be free, better hidden -- */

  friend CONSTEVAL auto size(pval) { return 1 + sizeof...(x); }

  template <unsigned I>
  friend CONSTEVAL decltype(auto) get(pval)
  {
    static_assert(I <= sizeof...(x), "get<I> index out of bounds");
    if constexpr (I == 0) return (v);
    else return get<I-1>(pval<(x)...>{});
  }
};

// pv variable template for a pval object
//
template <decltype(auto) v, decltype(auto)...x>
inline constexpr pval<(v),(x)...> pv{};

/* ************************************************************************ */

template <typename T, decltype(auto)...x>
struct dval
{
  static_assert((const_if_reference_v<decltype(x)> && ...),
    "Use as_const: reference type metadata must be const&");

  using type = dval;
  using value_type = T;

  value_type value;

  constexpr value_type operator()() const noexcept {return value;}
  constexpr operator value_type() const noexcept {return value;}

  /* -- size(dv) and get<I>(dv) could be free, better hidden -- */

  friend CONSTEVAL auto size(dval) { return 1 + sizeof...(x); }

#if __cpp_concepts
  template <unsigned I>
    requires (I == 0)
  friend constexpr value_type get(dval dv) { return dv(); }
  //
  template <unsigned I>
    requires (I > 0 && I <= sizeof...(x))
  friend CONSTEVAL decltype(auto) get(dval) {return get<I-1>(pval<(x)...>{});}
#else
  template <unsigned I>
  friend CONSTEVAL decltype(auto) get(dval dv)
  {
    if constexpr (I == 0) return dv();
    else return get<I-1>(pval<(x)...>{});
  }
#endif
};
template <typename T> dval(T const&) -> dval<T>;// const;

/* ****************** metatype  ************************* */

// metatype<T,x...> equivalent to std::type_identity, plus xtra metadata x...
//
template <typename T, decltype(auto)...x>
struct metatype
{
  static_assert((const_if_reference_v<decltype(x)> && ...),
    "Use as_const: reference type metadata must be const&");

  using type = T;

  /* -- size(dv) and get<I>(dv) could be free, better hidden -- */

  friend CONSTEVAL auto size(metatype) { return 1 + sizeof...(x); }

  template <unsigned I>
  friend CONSTEVAL decltype(auto) get(metatype)
  {
    static_assert(I > 0 && I <= sizeof...(x), "get<I> index out of bounds");
    return get<I-1>(pval<(x)...>{});
  }
};

// ty variable template for a metatype object
//
template <typename T, decltype(auto)...x>
inline constexpr metatype<T,(x)...> ty{};

/* ************************************************************************ */

// xtra<I>(val) get the Ith metadata of a parameta
// xtra<I>(metatype) get the Ith metadata of a metatype
//
#  if __cpp_concepts

template <unsigned I, val v>
CONSTEVAL decltype(auto) xtra(v={}) { return get<I+1>(v{}); }

template <unsigned I, typ t>
CONSTEVAL decltype(auto) xtra(t={}) { return get<I+1>(t{}); }

// type_t type alias to extract the concrete type from the meta type
//
template <typ T> using type_t = typename T::type;

// vtype_t type alias to extract the concrete type from the meta type
//
template <val T> using vtype_t = typename T::value_type;

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

#endif

/* ************************************************************************ */

#include "namespace.hpp" // close configurable namespace

#endif
