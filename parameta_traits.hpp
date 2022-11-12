//  Copyright (c) 2022 Lemurian Labs https://www.lemurianlabs.com/
//
//  Distributed under the Boost Software License, Version 1.0.
//        http://www.boost.org/LICENSE_1_0.txt
//
//  Repo: https://github.com/Lemurian-Labs/parameta

#ifndef LML_PARAMETA_TRAITS_HPP
#define LML_PARAMETA_TRAITS_HPP

/*
  parameta_traits.hpp
  ===================

  Concepts for 'meta parameterization' of template signatures.

  Depends on <type_traits> and targets C++20 language concepts.
  C++17 is supported with no concepts, only the equivalent traits.

  Meta value
  =concepts=       'metavalue' < 'metastatic' < 'metaconst'

  =traits=   is_metavalue[_v], is_metastatic[_v], is_metaconst[_v]

  Meta value concepts accept meta value *types*. They can replace NTTP
  *non-type* template parameters with constrained TTP *type* parameters
  for uniform and expressive template APIs and for 'generic staticity'.

  Based on the non-static value access API of std::integral_constant,
  these meta value concepts have increasingly constrained requirements;
  the metavalue concept refines to metastatic, requiring static value,
  and then metaconst, requiring constexpr value (the most constrained).

  The concepts are modeled by the meta types defined in "parameta.hpp";
  'dynameta' and 'staticmeta' metavalues, and 'typemeta' metatype.

  Concepts
  ========
  Meta type concept:

  * metatype<Q> concept: class types Q that represent types;
                empty class with 'type' member alias and no operator()

  Meta value concepts require the value access API of integral_constant:

  * metavalue<Q,VT> concept: class types Q that represent values;
                a 'value' data member of 'value_type' type alias,
                a no-arg function call operator()()const -> value_type;
                and an implicit conversion operator value_type() const;
                both returning 'value' (value_type may be a reference)

  * metastatic<Q,VT> concept: metavalue of constexpr value or static id;
                empty class, value accepted by template<decltype(auto)>

  * metaconst<Q,VT> concept: metastatic of constexpr value;
                             value accepted by template<auto>

                VT defaults to VT = return type of Q::operator()()const
                (if it exists, otherwise void and the concept is false)

  metaconst is most constrained; a constexpr-valued 'integral_constant'
  carrying a value of any type valid as an auto template argument.

  metastatic further accepts possibly non-constexpr values by reference,
  binding to a possibly-mutable variable id of static storage duration.

  metavalue is least constrained; it accepts all the above plus classes
  with non-static data member value for runtime dynamic initialization.

  Concept examples
  ================
    int g = {}; // a mutable global variable, static storage duration

    metavalue auto one = 1; // compile FAIL; int is not a metavalue

    metavalue<int> auto const1_int = integral_constant<int, 1>{};
    metavalue<int> auto static_int = integral_constant<int&, g>{};
    metavalue<int> auto dynamicint = dynameta{1}; // see parameta.hpp

    static_assert( metastatic<const1_int> &&  metaconst<const1_int>
               &&  metastatic<static_int> && !metaconst<static_int>
               && !metastatic<dynamicint> && !metaconst<dynamicint> );

    static_assert( metatype<type_identity<int>> && !metatype<int>
                && metatype<add_const<int>> && !metatype<const1_int> );

  Usage example
  =============
     template <typename Storage, metavalue<int> Extent>
       struct ray {
        [[no_unique_address]] Storage data;
        [[no_unique_address]] Extent extent{};
      };

  Here, Extent is a *type* but represents a value of value_type int.
  It's used as the type of data member 'extent' for 'generic staticity';
  this one definition can serve e.g. as an array or as a span whose size
  is dynamic, constexpr or a static variable, without specializations.
*/

#define SAME std::is_same_v

#define REMOVE_REF_T(...) std::remove_reference_t<__VA_ARGS__>
#if __cpp_lib_remove_cvref
#define REMOVE_CVREF_T(...) std::remove_cvref_t<__VA_ARGS__>
#else
#define REMOVE_CVREF_T(...) std::remove_cv_t<REMOVE_REF_T(__VA_ARGS__)>
#endif

#include <type_traits>

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

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

// as_return_t<T> : the return type of function type T()
//                  removes cv for scalar types, not for class types
// value_return_type<Q>: typename Q::value_type as a return type
//
template <class T> using as_return_t = decltype(std::declval<T()>()());
template <class Q>
using value_return_type = as_return_t<typename Q::value_type>;

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
            decltype(structural_value_or_fail<L{}()>{},(void)0)> = true;
#endif

// structural_non_value<v>() : v is not valid as an rvalue auto NTTP
//                          && is valid as an lvalue auto const& NTTP
template <auto const&, typename...T>
constexpr bool structural_non_value(T...) {return true;}
template <auto>
constexpr bool structural_non_value() {return false;}

} // impl;

/* ********** metavalue < metastatic < metaconst ************ */

// metavalue: A concept to match the access API of integral_constant

// metavalue <Q, V>
//   default V = unqualified return type of Q::operator()()const or void
// Q:
//   has a 'value_type' type alias member, possibly a reference type
//   has a 'value' data member of value_type, possibly const qualified
//   has no-arg, const qualified, call operator that returns value_type
//   and is implicitly const-convertible to value_type
//
#if __cpp_concepts
template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
concept metavalue = SAME<V, REMOVE_CVREF_T(typename Q::value_type)>
 &&
    SAME<impl::value_return_type<Q>, impl::functor_return_or_void<Q>>
 && (SAME<typename Q::value_type,std::remove_cv_t<decltype(Q::value)>>
  || SAME<typename Q::value_type, decltype(Q::value)>)
 && std::is_convertible_v<Q const, typename Q::value_type>;

template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
inline constexpr bool is_metavalue_v = metavalue<Q,V>;

#else
template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
inline constexpr bool is_metavalue_v = false;

template <typename Q>
inline constexpr bool is_metavalue_v<Q,
    REMOVE_CVREF_T(decltype(typename Q::value_type(Q::value)))>
  =
    SAME<impl::value_return_type<Q>, impl::functor_return_or_void<Q>>
 && (SAME<typename Q::value_type,std::remove_cv_t<decltype(Q::value)>>
  || SAME<typename Q::value_type, decltype(Q::value)>)
 && std::is_convertible_v<Q const, typename Q::value_type>;
#endif

// metastatic <Q, V = see-above*> 'static meta-value' concept
//   A metavalue Q whose value initializes template<decltype(auto)>
//   and is itself a default constructible empty type
//
#if __cpp_concepts
template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
concept metastatic = metavalue<Q,V>
             && std::is_empty_v<Q>
             && impl::structural_functor<Q{}>;

template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
inline constexpr bool is_metastatic_v = metastatic<Q,V>;

#else
template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
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
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
concept metaconst = metastatic<Q,V>
              && impl::structural_value_functor<Q{}>;

template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
inline constexpr bool is_metaconst_v = metaconst<Q,V>;

#else
template <typename Q,
          typename V = REMOVE_CVREF_T(impl::functor_return_or_void<Q>)>
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

#include "namespace.hpp" // close configurable namespace

#undef SAME
#undef REMOVE_REF_T
#undef REMOVE_CVREF_T

#endif
