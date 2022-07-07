//  Copyright (c) 2022 Lemurian Labs https://www.lemurianlabs.com/
//
//  Distributed under the Boost Software License, Version 1.0.
//        http://www.boost.org/LICENSE_1_0.txt
//
//  Repo: https://github.com/Lemurian-Labs/parameta

#ifndef LML_PARAMETA_TRAITS_HPP
#define LML_PARAMETA_TRAITS_HPP

/*
  parameta_traits.hpp: traits and concepts for parameterization by meta types
  ===================

  This header defines:

  Concepts (when compiled with C++20 concepts support):

    * metatype - a concept matching meta types that represent types
      as modeled by lml::metatype, std::type_identity and other type traits

    * metavalue -> metastatic -> metaconst, for types that represent values
      meta-'value' -> -'static value' -> -'pure static value'
      as modeled by increasingly static meta-value types
      dynameta<T>, parameta<(id)>, parameta<v>

    metavalue<T> matches T with the non-static API of std::integral_constant
    metastatic<T> matches metavalue T with static, compile-time value or id
    metaconst<T>  matches metastatic T with pure compile-time value (not id)

                         concept name:   Modeled by / accepted types:
  
              meta type    metatype:     metatype [type_identity]

              meta value   metavalue:    dynameta, parameta [integral_constant]
       meta static value   metastatic:             parameta [integral_constant]
  meta pure static value   metaconst:         pure parameta [integral_constant]

  Equivalent variable template predicate traits are defined, available in C++17

   is_metatype_v
   is_metavalue_v -> is_metastatic_v -> is_metaconst_v
*/

#include <type_traits>

#if __cpp_concepts
#define CPP_CONCEPTS(...)__VA_ARGS__
#define NO_CPP_CONCEPTS(...)
#else
#define CPP_CONCEPTS(...)
#define NO_CPP_CONCEPTS(...)__VA_ARGS__

#endif

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

// Provide remove_cvref_t backport c++20 -> c++17
//
#if __cpp_lib_remove_cvref
using std::remove_cvref_t;
#else
template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
#endif

// const_if_reference_v<T> trait = true for non-reference T
//   else if T is a reference then true if it's a const reference
//
template <typename T>
inline constexpr bool const_if_reference_v =
                     ! std::is_reference_v<T>
                    || std::is_const_v<std::remove_reference_t<T>>;

namespace impl {

// impl::functor_return_or_fail<T>
// the return type R of T::operator()()const -> R,
// if the const-qualified operator()() exists, else SFINAE fail
//
template <typename Tcvr, typename Tc = remove_cvref_t<Tcvr> const,
                         typename R = decltype(Tc{}.operator()())>
using functor_return_or_fail = R;

CPP_CONCEPTS(

template <typename L>
concept value_functor = requires (L const l) {l.operator()();};
)

NO_CPP_CONCEPTS(

template <typename T, typename = void>
inline constexpr bool is_value_functor_v = false;
)
template <typename T>
inline constexpr bool is_value_functor_v
         CPP_CONCEPTS( = value_functor<T>)
      NO_CPP_CONCEPTS(<T,std::void_t<functor_return_or_fail<T>>>
                    = ! std::is_void<functor_return_or_fail<T>>());

template <typename L, typename = void>
void (*value_func)() = nullptr;
//
template <typename L>
auto (*value_func<L, std::void_t<functor_return_or_fail<L>>>)()
                  = std::decay_t<functor_return_or_fail<L>()>{nullptr};

template <typename L>
using functor_value_type_or_void = decltype(value_func<L>());

template <decltype(auto)> inline constexpr bool structural_or_fail = true;

template <auto> inline constexpr bool auto_structural_or_fail = true;

CPP_CONCEPTS(
template <auto F> concept structural_functor_value = structural_or_fail<F()>;

template <auto F> concept auto_structural_functor_value
                                              = auto_structural_or_fail<F()>;
)
NO_CPP_CONCEPTS(

template <typename T> const T global_default_constructed{};

template <auto const& F>
auto is_structural_functor_value_f(long) -> std::false_type;

template <auto const& F, decltype(auto) = F()>
auto is_structural_functor_value_f(int) -> std::true_type;

template <typename T>
using is_structural_functor_value = decltype(
      is_structural_functor_value_f<global_default_constructed<T>>(1)
);

template <auto const& F>
auto is_auto_structural_functor_value_f(long) -> std::false_type;

template <auto const& F, auto = F()>
auto is_auto_structural_functor_value_f(int) -> std::true_type;

template <typename T>
using is_auto_structural_functor_value = decltype(
      is_auto_structural_functor_value_f<global_default_constructed<T>>(1)
);

)
} // impl;

/* ********** metavalue -> metastatic -> metaconst ************ */

// metavalue <T, V = unqualified return type of T::operator()()const, or void>
//
//   A concept to match the non-static part of std::integral_constant API
// T:
//   has a 'value_type' type alias member, T::value_type
//   has a no-arg, const qualified, call operator that returns value_type
//   has a 'value' member variable of value_type, possibly const qualified 
//   and is implicitly convertible to value_type
//
CPP_CONCEPTS(

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
concept metavalue =
    std::is_same_v<V, remove_cvref_t<typename T::value_type>>
 && std::is_same_v<typename T::value_type, impl::functor_return_or_fail<T>>
 && (std::is_same_v<typename T::value_type,std::remove_cv_t<decltype(T::value)>>
  || std::is_same_v<typename T::value_type, decltype(T::value)>)
 && std::is_convertible_v<T, typename T::value_type>;
)

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
inline constexpr bool is_metavalue_v =
                          CPP_CONCEPTS(metavalue<T,V>)
                       NO_CPP_CONCEPTS(false);
NO_CPP_CONCEPTS(
template <typename T>
inline constexpr bool is_metavalue_v<T, std::common_type_t<decltype(T::value),
                                                     typename T::value_type>> =
    std::is_same_v<typename T::value_type, impl::functor_return_or_fail<T>>
 && (std::is_same_v<typename T::value_type,std::remove_cv_t<decltype(T::value)>>
  || std::is_same_v<typename T::value_type, decltype(T::value)>)
 && std::is_convertible_v<T, typename T::value_type>;
)

// metastatic <T, V = see-above*> 'static meta-value' concept
//   matches metavalue T with value or id usable as an NTTP (structural type)
//   and is itself a default constructible empty type
//
CPP_CONCEPTS(

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
concept metastatic = metavalue<T,V>
             && std::is_empty_v<T>
             && impl::structural_functor_value<T{}>;
)
template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
inline constexpr bool is_metastatic_v =
                     CPP_CONCEPTS(metastatic<T,V>)
                  NO_CPP_CONCEPTS(is_metavalue_v<T,V>
                               && std::is_empty_v<T>
                               && impl::is_structural_functor_value<T>());

// metaconst <T, V = see-above*> 'pure static meta-value' concept 
//   matches metastatic T with value usable as an NTTP
//   (i.e. auto(value) is of structural type)
//
CPP_CONCEPTS(

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
concept metaconst = metastatic<T,V>
              && impl::auto_structural_functor_value<T{}>;
)
template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
inline constexpr bool is_metaconst_v =
                      CPP_CONCEPTS(metaconst<T,V>)
                   NO_CPP_CONCEPTS(is_metastatic_v<T,V>
                          && impl::is_auto_structural_functor_value<T>());

/* ************** 'metatype' meta-type concept ******************* */

// metatype concept for types that represent types, e.g. std::type_identity
//   requires empty class type with member type alias 'type' and no no-arg
//   call operator() (to disqualify std::integral_constant and derived types)
//
CPP_CONCEPTS(

template <typename T>
concept metatype = requires { typename T::type; }
            && std::is_empty_v<T>
            && ! impl::value_functor<T>;
)
template <typename T, typename = void>
inline constexpr bool is_metatype_v =
                         CPP_CONCEPTS(metatype<T>)
                      NO_CPP_CONCEPTS(false);
NO_CPP_CONCEPTS(
template <typename T>
inline constexpr bool is_metatype_v<T, std::void_t<typename T::type>>
                                    = std::is_empty_v<T>
                                   && ! impl::is_value_functor_v<T>;
)

#include "namespace.hpp" // close configurable namespace

#endif
