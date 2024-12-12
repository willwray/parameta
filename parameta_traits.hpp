/*
 SPDX-FileCopyrightText: 2024 The Lemuriad <opensource@lemuriad.com>
 SPDX-License-Identifier: BSL-1.0
 Repo: https://github.com/lemuriad/parameta
*/

#ifndef LML_PARAMETA_TRAITS_HPP
#define LML_PARAMETA_TRAITS_HPP

/*
  parameta_traits.hpp : Concepts for 'meta parameterization'
  ===================

  This header defines concepts for types that represent values;
  types like integral_constant that encode a 'static' value
  or types that hold a 'dynamic' value to be determined at runtime
  with equivalent access APIs, apart from constexpr-ness of value.

  Treating value parameters uniformly as types in template signatures
  enables expressive APIs with flexible 'generic staticity', useful
  e.g. for mixed static and dynamic multidimensional array extents.

  * Targets C++20 concepts and depends only on <concepts>.
  * There's C++17 trait-only support using <type_traits> as fallback.

  Meta value concepts:

               metavalue  <  metastatic  <  metaconst

  An object of metavalue type is used as a constant, read-only, parameter.
  It may further be metastatic or a fully metaconst compile-time constant.

  If the type is metaconst then the value is a constant expression encoded
  in the type; a static compile-time value initialized by an NTTP argument,
  e.g. integral_constant<S,v> for structural type S of constexpr value v.

  Otherwise, a non-metaconst metavalue's value has runtime storage.

  A (non-metaconst) metastatic type's value has static storage duration
  with the value's static id encoded in the type as a reference argument,
  e.g. integral_constant<T&,id> for referencable type T of static id.
 
  Otherwise, a non-metaconst non-metastatic metavalue type's value has
  dynamic storage duration. E.g. lml::dynamic_<int>{1}

 ***********************************************************************

  Concept synposis : the API of std::integral_constant, decomposed
  ================

 'metavalue'
   A type Q is metavalue if it is a class with:
  * Q::value_type; type alias member, possibly a reference type
  * Q::value; data member of value_type, possibly static, possibly const
  * Q::operator()() const -> value_type; no-arg call operator
  * Q::operator value_type() const; implicit conversion to value_type

 'metastatic'
   A type Q is metastatic if it is metavalue and:
  * Q::value can initialize a Q::value_type NTTP non-type template parameter

 'metaconst'
   A type Q is metaconst if it is metastatic and:
  * Q::value can copy-initialize an auto NTTP placeholder

 ***********************************************************************

  Examples
  ========
  std::integral_constant<T,v>   is a metaconst and metastatic metavalue
  or lml::static_<v>            if v is a constant expression of type T
                                (T must be a structural type for NTTP v)

  std::integral_constant<T&,id> is a metastatic metavalue (not metaconst
  or lml::static_<(id)>         if id is a non-constexpr static variable)

  lml::dynamic_<T>{v}           is a metavalue (not metastatic or metaconst)

 ***********************************************************************/

#if __cpp_concepts

#  include <concepts>
#  define SAME std::same_as
#  define REMOVE_CVREF_T(...) std::remove_cvref_t<__VA_ARGS__>

#else

#  include <type_traits>
#  define SAME std::is_same_v
#  define REMOVE_CVREF_T(...)\
   std::remove_cv_t<std::remove_reference_t<__VA_ARGS__>>

#endif

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

// as_return_t<T> : the qualified type of T as returned from a function
//                  removes cv for scalar types, not for class types
template <class T> using as_return_t = decltype(std::declval<T()>()());

namespace impl {

// ftor_ret_or_fail<L> : the return type R of L::operator()()const -> R
//                                            if it exists, else fail
template <typename L> using ftor_ret_or_fail
  = decltype(std::declval<REMOVE_CVREF_T(L) const>().operator()());

template <typename L> void ftor_ret_or_void(...);
template <typename L> auto ftor_ret_or_void(int) -> ftor_ret_or_fail<L>;

// functor_return_or_void<L> the return type of L::operator()()const->R
//                                              if it exists, else void
template <typename L>
using functor_return_or_void = decltype(ftor_ret_or_void<L>(0));

// has_call_op_const<L> : class L has a const-qualified call operator,
// L::operator()()const, that takes no arguments (and may return void)
//
#if __cpp_concepts
template <typename L>
concept has_call_op_const = requires (L const l) { l.operator()(); };
#else
template <typename L, typename = functor_return_or_void<L>>
inline constexpr bool has_call_op_const = false;

template <typename L>
inline constexpr bool has_call_op_const<L, ftor_ret_or_fail<L>> = true;
#endif

// element0(v) -> v if it's not of array type else element0(v[0])
//                     i.e. the initial element of v, recursively
template <typename T>
constexpr decltype(auto) element0(T&& v) {
  if constexpr (std::is_array_v<REMOVE_CVREF_T(decltype(v))>)
    return element0(v[0]);
  else
    return (T&&)v;
}

template <decltype(auto)> using structural_or_fail = bool;
template <auto>     using structural_value_or_fail = bool;

// structural_functor<f>,       f() initializes template<decltype(auto)>
// structural_value_functor<f>, f() initializes template<auto>
//
#if __cpp_concepts
template <auto f> concept structural_functor
                        = structural_or_fail<f()>{true};

template <auto f> concept structural_value_functor
                        = structural_value_or_fail<element0(f())>{true};
#else
template <typename L, typename = void>
inline constexpr bool is_structural_functor_v = false;
template <typename L>
inline constexpr bool is_structural_functor_v<L,
            decltype(structural_or_fail<L{}()>{},(void)0)> = true;

template <typename L, typename = void>
inline constexpr bool is_structural_value_functor_v = false;
template <typename L>
inline constexpr bool is_structural_value_functor_v<L,
  decltype(structural_value_or_fail<element0(L{}())>{},(void)0)> = true;
#endif

// structural_non_value<v>() : v is not valid as an rvalue auto NTTP
//                          && is valid as an lvalue auto const& NTTP
template <auto const&, typename...T>
constexpr bool structural_non_value(T...) {return true;}
template <auto>
constexpr bool structural_non_value() {return false;}

} // impl;

/* ********** metavalue < metastatic < metaconst ************ */
/*
   metavalue: A concept to match the access API of integral_constant
              BUT its value is NOT neccessarily a constant expression,
              i.e. it may not be statically auto encoded in the type.
*/
// metavalue <Q, V>
//   default V = unqualified return type of Q::operator()()const or void
// Q:
//   has a 'value_type' type alias member, possibly a reference type
//   has a 'value' data member of value_type, possibly const qualified
//   has no-arg, const qualified, call operator that returns value_type
//   and is implicitly const-convertible to value_type
//
#if __cpp_concepts

template <typename Q, typename V = impl::functor_return_or_void<Q>>
concept metavalue
  = SAME<REMOVE_CVREF_T(V), REMOVE_CVREF_T(typename Q::value_type)>
 && SAME<as_return_t<typename Q::value_type>, impl::functor_return_or_void<Q>>
 && (SAME<const typename Q::value_type, decltype(Q::value)>
     || SAME<typename Q::value_type, decltype(Q::value)>)
 && std::is_convertible_v<Q const, typename Q::value_type>;

template <typename Q, typename V = impl::functor_return_or_void<Q>>
inline constexpr
bool is_metavalue_v = metavalue<Q,V>;

#else

template <typename Q, typename V = impl::functor_return_or_void<Q>>
inline constexpr
bool is_metavalue_v = false;

template <typename Q>
inline constexpr
bool is_metavalue_v<Q, decltype(typename Q::value_type(Q::value))>
  =
    SAME<as_return_t<typename Q::value_type>, impl::functor_return_or_void<Q>>
 && (SAME<typename Q::value_type,std::remove_cv_t<decltype(Q::value)>>
  || SAME<typename Q::value_type, decltype(Q::value)>)
 && std::is_convertible_v<Q const, typename Q::value_type>;

#endif

// metastatic <Q, V = see-above*> 'static meta-value' concept
//   A metavalue Q whose value initializes template<decltype(auto)>
//   and is itself a default constructible empty type
//
#if __cpp_concepts
template <typename Q, typename V = impl::functor_return_or_void<Q>>
concept
metastatic = metavalue<Q,V>
             && std::is_empty_v<Q>
             && impl::structural_functor<Q{}>;

template <typename Q,
          typename V = impl::functor_return_or_void<Q>>
inline constexpr bool is_metastatic_v = metastatic<Q,V>;

#else
template <typename Q,
          typename V = impl::functor_return_or_void<Q>>
inline constexpr bool is_metastatic_v = is_metavalue_v<Q,V>
                                && std::is_empty_v<Q>
                                && impl::is_structural_functor_v<Q>;
#endif

// metaconst <Q, V = see-above*> 'pure static meta-value' concept
//   A metastatic Q whose value initializes template<auto>
//   i.e. constexpr value and value_type is a structural type
//
#if __cpp_concepts
template <typename Q,
          typename V = impl::functor_return_or_void<Q>>
concept metaconst = metastatic<Q,V>
              && impl::structural_value_functor<Q{}>;

template <typename Q,
          typename V = impl::functor_return_or_void<Q>>
inline constexpr bool is_metaconst_v = metaconst<Q,V>;

#else
template <typename Q,
          typename V = impl::functor_return_or_void<Q>>
inline constexpr bool is_metaconst_v = is_metastatic_v<Q,V>
                              && impl::is_structural_value_functor_v<Q>;
#endif

/* ************** 'metatype' meta-type concept ******************* */

// metatype concept for types that represent types, e.g. type_identity
//   requires empty class type with member type alias 'type'
//   and no call operator()() (to disqualify integral_constant etc.)
//
#if __cpp_concepts
template <typename Q>
concept metatype = requires { typename Q::type; }
            && std::is_empty_v<Q>
            && ! impl::has_call_op_const<Q>;

template <typename Q, typename = void>
inline constexpr bool is_metatype_v = metatype<Q>;

#else
template <typename Q, typename = void>
inline constexpr bool is_metatype_v = false;

template <typename Q>
inline constexpr bool is_metatype_v<Q, std::void_t<typename Q::type>>
                                    = std::is_empty_v<Q>
                                   && ! impl::has_call_op_const<Q>;
#endif

/* ************** 'metapara' meta-parameter concept ***************** */

// metapara concept for types that represent type or non-type parameters
//
#if __cpp_concepts
template <typename Q>
concept metapara = metatype<Q> || metavalue<Q>;

template <typename Q, typename = void>
inline constexpr bool is_metapara_v = metapara<Q>;

#else
template <typename Q, typename = void>
inline constexpr bool is_metapara_v = is_metatype_v<Q>
                                   || is_metavalue_v<Q>;
#endif


/* **** concepts and utilities for NTTP 'auto_list'  ***************** */

// auto_list
//
template <typename LV>
inline constexpr bool is_auto_list_v = false;

template <template <auto...> class L, auto...V>
inline constexpr bool is_auto_list_v<L<V...>> = true;

template <typename LV>
concept auto_list = is_auto_list_v<LV>;

// auto_list_size<T> the number of elements V in auto list T = L<V...>
//
template <auto_list LV> inline constexpr std::size_t auto_list_size
 = NOT_DEFINED(auto_list_size<LV>);
//
template <template <auto...> class L, auto...V>
inline constexpr auto auto_list_size<L<V...>> = sizeof...(V);

template <std::size_t I,
  decltype(auto)X0=0,decltype(auto)X1=0,decltype(auto)X2=0,decltype(auto)X3=0,
  decltype(auto)X4=0,decltype(auto)X5=0,decltype(auto)X6=0,decltype(auto)X7=0,
  decltype(auto)X8=0,decltype(auto)X9=0,decltype(auto)Xa=0,decltype(auto)Xb=0,
  decltype(auto)Xc=0,decltype(auto)Xd=0,decltype(auto)Xe=0,decltype(auto)Xf=0,
  decltype(auto)...X>
constexpr decltype(auto) auto_pack_element()
{
  switch (I)
  {
  case 0x0: if constexpr (I==0x0) return (X0);
  case 0x1: if constexpr (I==0x1) return (X1);
  case 0x2: if constexpr (I==0x2) return (X2);
  case 0x3: if constexpr (I==0x3) return (X3);
  case 0x4: if constexpr (I==0x4) return (X4);
  case 0x5: if constexpr (I==0x5) return (X5);
  case 0x6: if constexpr (I==0x6) return (X6);
  case 0x7: if constexpr (I==0x7) return (X7);
  case 0x8: if constexpr (I==0x8) return (X8);
  case 0x9: if constexpr (I==0x9) return (X9);
  case 0xa: if constexpr (I==0xa) return (Xa);
  case 0xb: if constexpr (I==0xb) return (Xb);
  case 0xc: if constexpr (I==0xc) return (Xc);
  case 0xd: if constexpr (I==0xd) return (Xd);
  case 0xe: if constexpr (I==0xe) return (Xe);
  case 0xf: if constexpr (I==0xf) return (Xf);
  default: if constexpr (I>=0x10) return auto_pack_element<I-0x10,(X)...>();
  }
}

namespace impl {
//
template <std::size_t I, template <auto...> class L, auto...V>
  //requires (I < sizeof...(V))
constexpr auto auto_list_element(L<V...>*)
{
  return auto_pack_element<I,V...>();
  static_assert(I < sizeof...(V), "auto_list_element index out of bounds");
}
}// impl

// auto_list_element<I,LV> type_identity of element I in auto list LV
//
template <std::size_t I, auto_list LV>
constexpr auto auto_list_element
       = impl::auto_list_element<I>(static_cast<LV*>(nullptr));

#include "namespace.hpp"

#undef SAME
#undef REMOVE_CVREF_T

#endif
