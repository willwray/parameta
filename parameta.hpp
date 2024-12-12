/*
 SPDX-FileCopyrightText: 2024 The Lemuriad <opensource@lemuriad.com>
 SPDX-License-Identifier: BSL-1.0
 Repo: https://github.com/lemuriad/parameta
*/

#ifndef LML_PARAMETA_HPP
#define LML_PARAMETA_HPP

/*
  parameta.hpp
  ============

  A C++20 library of meta-types and concepts for generic specification
  of parameters as either compile-time constants or runtime variables.
  Meta parameter types model concepts defined in parameta_traits.hpp
  and can also carry optional metadata.
*/

#include "parameta_traits.hpp"
#include "parameta_platform.hpp"

#include "namespace.hpp"

/* ****************************************************************** */

// metadata<x...> holds a variadic pack of values, a compile-time tuple
template <auto...> struct metadata;

// Forward declarations of class templates type_, dynamic_, static_
template <typename,       auto...> struct type_;
template <typename,       auto...> struct dynamic_;
template <decltype(auto), auto...> struct static_;
template <typename T,T v, auto...x> using static_t
                                        = static_<v,x...>;

template <decltype(auto), auto...> struct parameta;

/* ****************************************************************** */
/* metadata<x...>

  A tuple of values held as a pack of NTTP non-type template parameters.

  m.size: the number of elements in pack x... (static data member)
  m.get<I>() get the Ith element in pack x... (and optionally more values)
  m.map(f) returns f(x...) given functor f that accepts value pack x...
*/

template <auto...x>
struct metadata
{
  static constexpr auto size = sizeof...(x);

  template <int...I>
    requires (sizeof...(I) > 0
              && size > 0
              && (((I < int{size}) && (I >= -int{size})) && ...))
  static CONSTEVAL auto get() noexcept
    -> static_<auto_pack_element<(I >= 0 ? I : I+size), x...>()...>
  { return {}; }

  static constexpr decltype(auto) map(auto f) noexcept(noexcept(f(x...)))
  { return f(x...); }

  friend auto operator<=>(metadata,metadata) = default;
};

/* ****************************************************************** */
/* static_<v,x...> is a metavalue type that represents a static value.

  static_<v> is defined following std::integral_constant<decltype(v),v>
  with optional variadic 'xtra' NTTP metadata x... and CTAD.

  It encodes a value v, or its id, in the initial NTTP template argument
  using decltype(auto) to deduce the value category as prvalue or lvalue.
*/

template <decltype(auto) v, auto...x>
struct static_
{
  using type = static_;
  using value_type = decltype(v); // may be an lvalue ref
                                 // but not an rvalue ref
  static constexpr value_type value = v;
  static constexpr ::metadata<x...> metadata = {};

  STATIC_CALL_OP_IF_AVAILABLE(
  CONSTEVAL value_type operator()() /*const*/, noexcept {return v;}
  )
  CONSTEVAL operator value_type() const noexcept {return v;}

  static_() = default;

  // CTAD constructor
  consteval
  static_(metastatic<value_type> auto V, ::metadata<x...> = {}) noexcept
      requires (!metaconst<type> || V==v) {}
};

// CTAD guide
template <metastatic V, auto...x>
static_(V, metadata<x...>) -> static_<V::value, x...>;

/* ****************************************************************** */
/* dynamic_<T,x...> wraps a non-static data member, T value;
                  - has the metavalue access API of integral_constant
                  - has variadic 'xtra' NTTP metadata x...

  dynamic_ is a metavalue type that represents a value of type T, whose
  actual value is to be determined at runtime by dynamic initialization.
  The call operator()()const returns the value, and there's an implicit
  conversion to the value, both by value_type. Reference types allowed.
*/

template <typename T, auto...x>
struct dynamic_
{
  using type = dynamic_;
  using value_type = T;

  value_type value;
  [[no_unique_address]] ::metadata<x...> metadata = {};

  constexpr value_type operator()() const noexcept {return value;}
  constexpr operator value_type() const noexcept {return value;}

  friend auto operator<=>(dynamic_,dynamic_) = default;

  template <typename U, auto...y>
  friend constexpr bool operator==(dynamic_,dynamic_<U,y...>) noexcept
  { return false; }
};

// dynamic_ CTAD guides deduce the object value type of the argument
// (except functions deduce as reference type for non-nullability)
// (arrays deduce as array objects which fail to copy initialize).
//
template <typename T>
dynamic_(T const&)
  -> dynamic_<std::conditional_t<std::is_function_v<T>, T&, T >>;

template <typename T, auto...x>
dynamic_(T const&, metadata<x...>)
  -> dynamic_<std::conditional_t<std::is_function_v<T>, T&, T>,x...>;

/* ****************** type_  ************************* */
/* type_<T,x...> like std::type_identity, plus xtra metadata x...
*/
template <typename T, auto...x>
struct type_
{
  using type = T;

  static ::metadata<x...> metadata{};
};

/* ****************************************************************** */

/* makestatic<X>() function overloads deduce metaconst X if possible
                   else metastatic X, with no decay */

template <auto v, auto...x, typename...T>
constexpr auto makestatic(T...) noexcept
{ return static_<v,x...>{}; }

#if __cpp_concepts
template <auto const& v, auto...x>
constexpr auto makestatic() noexcept
  requires (
    impl::structural_non_value<v>()
    || std::is_function_v<typeof(v)>
    || std::is_array_v<typeof(v)>)
{ return static_<v,x...>{}; }

#else

template <auto const& v, auto...x>
constexpr auto makestatic() ->
std::enable_if_t<
    impl::structural_non_value<v>()
    || std::is_function_v<typeof(v)>
    || std::is_array_v<typeof(v)>, static_<v,x...>>
{ return {}; }

#endif

#include "namespace.hpp"

#include "parameta_platform.hpp"

#endif
