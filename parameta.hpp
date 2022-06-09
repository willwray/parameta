//  Copyright (c) 2022 Lemurian Labs https://www.lemurianlabs.com/
//
//  Distributed under the Boost Software License, Version 1.0.
//        http://www.boost.org/LICENSE_1_0.txt
//
//  Repo: https://github.com/Lemurian-Labs/lml_parameta

#ifndef LML_PARAMETA_HPP
#define LML_PARAMETA_HPP

/*
  parameta.hpp: C++20 concepts and meta types for parameterization
  ============

  This header defines:

    * pval<v> and dval<T> - types representing 'static' and 'dynamic' values.
      As 'meta value types' they generalize the API of std::integral_constant.
      Both pval<v,x...> and dval<T,x...> can carry extra variadic metadata x...
      Let's call them parameta types.

    * val -> s_val -> ps_val; three concepts for matching meta value types.
      dval<T>, pval<(id)>, pval<v> model these increasingly static val types;

        'value' -> 'static value' -> 'pure static value'

    * metatype<T,x...> meta type that represents type T with optional metadata.
      Like std::type_identity, plus possible extra variadic metadata x...

    * typ - a concept matching meta types that represent types, as modeled by
      metatype, std::type_identity and other type-valued type traits.

  Note: 'integral_constant' is a misnomer; it's not constrained to carry values
      of integral types only; any valid non-type template argument is admitted
      including the address or 'id' of static-storage variables, const or not.
      It is a parameta type.

  pval<v> denotes a 'static' or 'parameter' value that, like integral_constant,
  has its value v statically encoded, manifestly usable as a template argument.
  As an empty type it can be laid out to occupy no storage in a class.

  dval<T> denotes a 'dynamic' or 'deferred' value that has the same access API
  as pval but its signature carries only the type T of the value, plus any meta
  data needed to determine its runtime storage, initialization, access and use.

  "Dynamic" implies that the value isn't know statically at compile-time so is
  deferred to runtime, i.e. dynamic initialization is assumed and the runtime-
  determined value can't be used as a template argument or encoded in the type.

  "Dynamic" does not imply mutability. Indeed, a dval usually models immutable
  data; runtime-determined constants rather than compile-time constants.

  "Static" does not imply immutability; non-pure pvals refer to mutable data
  (note that an NTTP referent variable has to have static storage duration).

  Variable templates, pv and ty:

    abbreviated static parameta value: pv<v> == pval<v>{}     // save 4 chars
    abbreviated metatype object: vv    ty<T> == metatype<T>{} // save 8 chars

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

  This API is captured in the val concept and refined by s_val and then ps_val:

       val<T> models type T with the API above; pval-like or dval-like
     s_val<T> models a val with static, compile-time value or id; pval-like
    ps_val<T> models a pval with pure compile-time value; pure pval-like

  Concepts                 concept name:      accepted types:
  
                   meta type       typ:        metatype [type_identity]

                  meta value       val:      dval, pval [integral_constant]
           meta static value     s_val:            pval [integral_constant]
      meta pure static value    ps_val:       pure pval [integral_constant]

  Any extra x... arguments are accessed by get<I> or xtra<I>

    * get<I>(val)  returns the Ith v,x... argument
    * xtra<I>(val) returns the Ith   x... argument == get<I+1>(val)
*/

#include <concepts>

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

namespace impl {

// impl::vfunctor<L> minimal 'value functor' requirement; has operator()()const
//
template <typename L>
concept vfunctor = requires (L const l) {l.operator()();};

// impl::const_if_reference<T> concept = true for non-reference T
//   else if T is a reference then true if it's a const reference
//
template <typename T>
concept const_if_reference =
       ! std::is_reference_v<T>
      || std::is_const_v<std::remove_reference_t<T>>;

// impl::functor_return<L>()
//   returns std::type_identity of L::operator()()const return type if present
//   else returns std::type_identity void if it doesn't exist
//
template <typename L>
auto functor_return()
{
  if constexpr (impl::vfunctor<L>)
    return []<typename R>(R(L::*)()const) {
      return std::type_identity<R>{};
    }(&L::operator());
  else
    return std::type_identity<void>{};
}
//
template <typename L>
using functor_return_t = typename decltype(functor_return<L>())::type;

// impl::structural<v>
//   true if v is a valid type of value for an NTTP non-type template parameter
//   else substitution fails
//
template <decltype(auto)> inline constexpr bool structural = true;

// impl::auto_structural<v>
//   true if v is a valid type of value for an auto deduced NTTP
//   else substitution fails
//
template <auto> inline constexpr bool auto_structural = true;

} // impl;

/* ********** val -> s_val -> ps_val meta-value-type concepts ************ */

// val <T, V = see-below*>
//
//   A concept to match the non-static part of std::integral_constant API
// T:
//   has a 'value_type' type alias member, T::value_type
//   *(V = T::value_type with cvref qualifiers removed)
//   has a no-arg, const qualified, call operator that returns value_type
//   has a 'value' member variable of value_type, possibly const qualified 
//   and is implicitly convertible to value_type
//
template <typename T,
          typename V = std::remove_cvref_t<impl::functor_return_t<T>>>
concept val =
     std::same_as<V, std::remove_cvref_t<typename T::value_type>>
  && requires (typename T::value_type(T::*p)()const) { p = &T::operator(); }
  && (std::same_as<typename T::value_type, std::remove_cv_t<decltype(T::value)>>
   || std::same_as<typename T::value_type, decltype(T::value)>)
  && std::is_convertible_v<T, typename T::value_type>;

// s_val <T, V = see-above*>
//   'static val' concept matches a val whose value or id is usable as an NTTP
//   (it is a structural type) and is itself a default constructible empty type
//
template <typename T,
          typename V = std::remove_cvref_t<impl::functor_return_t<T>>>
concept s_val = val<T,V>
             && std::is_empty_v<T>
             && impl::structural<T{}()>;

// ps_val <T, V = see-above*>
//   'pure static val' concept matches an s_val whose value is usable as an NTTP
//   (i.e. auto(value) is of structural type)
//
template <typename T,
          typename V = std::remove_cvref_t<impl::functor_return_t<T>>>
concept ps_val = s_val<T,V>
             && impl::auto_structural<T{}()>;

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
  requires impl::const_if_reference<decltype(v)>
struct pval
{
  using type = pval;
  using value_type = decltype(v); // may be an lvalue reference type
                                  // (but not an rvalue rererence)
  static constexpr value_type value = v;

  /* static */ // c++23 static operator() https://wg21.link/p1169
  constexpr value_type operator()() const noexcept {return v;}
  constexpr operator value_type() const noexcept {return v;}

  /* -- size(pv) and get<I>(pv) could be free, better hidden -- */

  friend consteval auto size(pval) { return 1 + sizeof...(x); }

  template <unsigned I>
    requires (I <= sizeof...(x))
  friend consteval decltype(auto) get(pval)
  {
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
  requires impl::const_if_reference<T>
struct dval
{
  using type = dval;
  using value_type = T;

  value_type value;

  constexpr value_type operator()() const noexcept {return value;}
  constexpr operator value_type() const noexcept {return value;}

  /* -- size(dv) and get<I>(dv) could be free, better hidden -- */

  friend consteval auto size(dval) { return 1 + sizeof...(x); }

  template <unsigned I>
    requires (I == 0)
  friend constexpr value_type get(dval dv) { return dv(); }
  //
  template <unsigned I>
    requires (I > 0 && I <= sizeof...(x))
  friend consteval decltype(auto) get(dval) {return get<I-1>(pval<(x)...>{});}
};
template <typename T> dval(T const&) -> dval<T>;// const;

/* ************** 'typ' meta-type concept ******************* */

// typ concept for types that represent types, e.g. std::type_identity
//   requires empty class type with member type alias 'type' and no no-arg
//   call operator() (to disqualify std::integral_constant and derived types)
//
template <typename T>
concept typ = requires { typename T::type; }
            && std::is_empty_v<T>
            && ! impl::vfunctor<T>;

// metatype<T,x...> equivalent to std::type_identity, plus xtra metadata x...
//
template <typename T, decltype(auto)...x>
struct metatype
{
  using type = T;

  /* -- size(dv) and get<I>(dv) could be free, better hidden -- */

  friend consteval auto size(metatype) { return 1 + sizeof...(x); }

  template <unsigned I>
    requires (I > 0 && I <= sizeof...(x))
  friend consteval decltype(auto) get(metatype)
  {
    return get<I-1>(pval<(x)...>{});
  }
};

// ty variable template for a metatype object
//
template <typename T, decltype(auto)...x>
inline constexpr metatype<T,(x)...> ty{};

/* ************************************************************************ */

// xtra<I>(val) get the Ith metadata of a parameta
//
template <unsigned I, val v>
consteval decltype(auto) xtra(v={}) { return get<I+1>(v{}); }

// xtra<I>(metatype) get the Ith metadata of a metatype
//
template <unsigned I, typ t>
consteval decltype(auto) xtra(t={}) { return get<I+1>(t{}); }

/* ************************************************************************ */

// type_t type alias to extract the concrete type from the meta type
//
template <typ T> using type_t = typename T::type;

// vtype_t type alias to extract the concrete type from the meta type
//
template <val T> using vtype_t = typename T::value_type;

/* ************************************************************************ */

#include "namespace.hpp" // close configurable namespace

#endif
