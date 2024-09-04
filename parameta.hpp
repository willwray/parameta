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
  Meta parameter types for meta parameterization of template signatures:

   'dynameta' and 'staticmeta'  meta value class templates
   'typemeta'                   meta type class template

  Targets C++20 concepts. C++17 is supported with no concepts or cNTTP.
  See the repo readme for more background, motivation and example usage.

  * staticmeta<v,x...> represents constexpr value v, or a static id
  * dynameta<T,x...> represents a value of type T, its value TBD at init
  * typemeta<T,x...> represents type T

  The variadic x... arguments are optional extra metadata
  (or staticmeta<v...> may be used as a list of constexpr values or ids)

  staticmeta is like std::integral_constant
  dynameta   is like std::dynamic_extent, in intent, but a meta *type*
  typemeta   is like std::type_identity

  integral_constant can be fully implemented in terms of staticmeta,
  and a type alias staticmetacast is provided with the same signature

    template <typename T, T v> using integral_constant = staticmeta<v>;
    template <typename T, T v> using staticmetacast = staticmeta<v>;

  staticmeta is an empty type. The value v, or its id, is carried in the
  template signature, so is manifestly usable as a template argument.
  Immutability is not implied; a static id may refer to mutable data.

  dynameta<T> type heralds a 'dynamic' or deferred value; its signature
  carries only the type of the value. "Dynamic" implies initialization
  is dynamic, to a runtime-determined value; mutability is not implied.

  typemeta<T> is included mostly for completeness; a type with metadata.

  Concepts
  ========
  The meta parameter types model the concepts in "parameta_traits.hpp".
  Here, c is a constexpr value and p a non-constexpr static value id:

  * staticmeta<c>   models 'metavalue' < 'metastatic' < 'metaconst'
  * staticmeta<(p)> models 'metavalue' < 'metastatic'
  * dynameta<T>     models 'metavalue'

  * typemeta<T>     models 'metatype'

  See the repo docs and header for concept definitions and usage notes.

  API Summary
  ===========
  The value of any metavalue object m is extracted as m.value, m() or by
  implicit conversion to typename decltype(m)::value_type. Both dynameta
  and staticmeta model the metavalue concept so implement the same value
  access API and are substitutable, as long as not presumed constexpr.

  A metastatic or metaconst value may be accessed directly from the meta
  type M, as M::value or M{}(), and is constexpr for metaconst<M>.

  The class synopses below highlight the template signature difference;
  dynameta wraps a non-static data member while staticmeta has a static
  constexpr data member 'value':

                    template <typename T, decltype(auto)... x>
    struct dynameta {
                      using value_type = T;
                      value_type value;
                    // API };

                      template <decltype(auto) v, decltype(auto)... x>
    struct staticmeta {
                        using value_type = decltype(v);
                        static constexpr value_type value = v; ...
                      // API };

  The remaining access API is identical to std::integral_constant

  Metadata access

  Three static member functions, metasize(), metaget<I>() and  meta(f)
  provide access to the metadata x...

  * M::metasize() returns number of x... arguments, i.e. -> sizeof...(x)
  * M::metaget<I>() returns the Ith x... argument
  * M::meta(f) returns f.template operator()<x...>();
*/

/* METADATA_ACCESS_H
  In-class accessors are not necessary, as x... is part of the type.
  They're a convenience. meta(f) assists generic out-of-class access.
  However, Clang bug https://github.com/llvm/llvm-project/issues/58682
  necessitates in-class API. The three static members are injected by
  #include "metadata_acess.h", as controlled by this filename symbol:
*/
#ifndef METADATA_ACCESS_H
#define METADATA_ACCESS_H "metadata_access.h"
#endif

/* ****************************************************************** */

// consteval if available else fall back to constexpr for C++17
//          (exclude MSVC due to false 'not constant expression')
#if ! defined (CONSTEVAL)
#  if defined (__cpp_consteval) && ! defined (_MSC_VER)
#     define CONSTEVAL consteval
#  else
#     define CONSTEVAL constexpr
#  endif
#endif

// static call operator() if available C++23 https://wg21.link/p1169
#if ! defined (STATIC_CALL)
#  if defined (__cpp_static_call_operator)
#     define STATIC_CALL static
#     define STATIC_CALL_CV
#  else
#     define STATIC_CALL
#     define STATIC_CALL_CV const
#  endif
#endif

// typeof(T) convenience, is in C23, possibly in C++26
#ifndef typeof
# ifdef _MSC_VER
#  define typeof(...)std::remove_reference_t<decltype(__VA_ARGS__)>
# else
#  define typeof(...)__typeof(__VA_ARGS__)
# endif
#endif

// MSVCONST(x) : MSVC sometimes seems to need NTTP const_cast
#ifndef MSVCONST
# ifdef _MSC_VER
#  define MSVCONST(X) const_cast<typeof(X)const&>(X)
# else
#  define MSVCONST(X)X
# endif
#endif

/* ****************************************************************** */

#include "parameta_traits.hpp"

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

// Class declarations; typemeta, dynameta, staticmeta
template <typename,       decltype(auto)...> struct typemeta;
template <typename,       decltype(auto)...> struct dynameta;
template <decltype(auto), decltype(auto)...> struct staticmeta;

// staticmetacast<T,v> : explicit-type alias of staticmeta
template <typename T, T v, decltype(auto)...x> using staticmetacast
                                                   = staticmeta<v,x...>;

/* ****************************************************************** */
/* staticmeta<v,x...> std::integral_constant<T,v> minus T plus x...
                    - eliminates the unneccessary type parameter T
                    - adds variadic 'xtra' NTTP metadata x...

  staticmeta is a metavalue type that represents a static value.
  staticmeta<v> encodes a value v, by value, or its id by reference,
                using decltype(auto) to deduce the value category of v

  The static data member 'value' memos an rvalue v else an lvalue id if
  the member type alias value_type = decltype(v) is an lvalue reference.
  The call operator()()const returns the value, and there's an implicit
  conversion to the value, both by value_type so possibly by reference.
*/

template <decltype(auto) v, decltype(auto)...x>
struct staticmeta
{
  using type = staticmeta;
  using value_type = decltype(v); // may be an lvalue ref
                                 // but not an rvalue ref
  static constexpr value_type value = v;
  STATIC_CALL
  CONSTEVAL value_type operator()() STATIC_CALL_CV noexcept {return v;}
  CONSTEVAL operator value_type() const noexcept {return v;}

#if __has_include(METADATA_ACCESS_H)
# include METADATA_ACCESS_H
#endif
};

/* ****************************************************************** */
/* dynameta<T,x...> wraps a non-static data member, T value;
                  - has the metavalue access API of integral_constant
                  - has variadic 'xtra' NTTP metadata x...

  dynameta is a metavalue type that represents a value of type T, whose
  actual value is to be determined at runtime by dynamic initialization.
  The call operator()()const returns the value, and there's an implicit
  conversion to the value, both by value_type. Reference types allowed.
*/

template <typename T, decltype(auto)...x>
struct dynameta
{
  using type = dynameta;
  using value_type = T;

  value_type value;

  constexpr value_type operator()() const noexcept {return value;}
  constexpr operator value_type() const noexcept {return value;}

#if __has_include(METADATA_ACCESS_H)
# include METADATA_ACCESS_H
#endif
};

// dynameta deduction guide
// deduces an object type, except for (unambiguous) function args which
// deduce a reference type (for full providence and non-nullability).
// A deduced array value type causes dynameta instantiation failure.
//
template <typename T> dynameta(T const&)
                   -> dynameta<std::conditional_t<
                               std::is_function_v<T>, T&, T >>;

/* ****************** typemeta  ************************* */
/* typemeta<T,x...> like std::type_identity, plus xtra metadata x...
*/
template <typename T, decltype(auto)...x>
struct typemeta
{
  using type = T;

#if __has_include(METADATA_ACCESS_H)
# include METADATA_ACCESS_H
#endif
};

/* ****************************************************************** */

/* makestatic<X>() function overloads deduce metaconst X if possible
                   else metastatic X, with no decay */

template <auto v, decltype(auto)...x, typename...T>
constexpr auto makestatic(T...) -> staticmeta<v,x...>
{ return {}; }

#ifdef _MSC_VER
#define STATICMETA_V_X decltype(staticmetacast<decltype(v)const&,v,x...>{})
#else
#define STATICMETA_V_X staticmeta<v,x...>
#endif

#if __cpp_concepts
template <auto const& v, decltype(auto)...x>
constexpr auto makestatic() -> STATICMETA_V_X
  requires (
    impl::structural_non_value<v>()
    || std::is_function_v<typeof(v)>
    || std::is_array_v<typeof(v)>)
{ return {}; }

#else
template <auto const& v, decltype(auto)...x>
constexpr auto makestatic() ->
std::enable_if_t<
    impl::structural_non_value<v>()
    || std::is_function_v<typeof(v)>
    || std::is_array_v<typeof(v)>, STATICMETA_V_X>
{ return {}; }

#endif

#include "namespace.hpp" // close configurable namespace

#undef typeof
#undef MSVCONST
#undef STATIC_CALL
#undef CONSTEVAL
#undef METADATA_ACCESS_H

#endif
