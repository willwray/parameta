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

    * val -> s_val -> ps_val; three concepts for matching meta value types;
      val 'value' -> s_val 'static value' -> ps_val 'pure static value'
      as modeled by increasingly static val types dval<T>, pval<(id)>, pval<v>

    * typ - a concept matching meta types that represent types, as modeled by
      metatype, std::type_identity and other type-valued type traits.

  This API is captured in the val concept and refined by s_val and then ps_val:

       val<T> models type T with the API above; pval-like or dval-like
     s_val<T> models a val with static, compile-time value or id; pval-like
    ps_val<T> models a pval with pure compile-time value; pure pval-like

  Concepts                 concept name:      Modeled by / accepted types:
  
                   meta type       typ:        metatype [type_identity]

                  meta value       val:      dval, pval [integral_constant]
           meta static value     s_val:            pval [integral_constant]
      meta pure static value    ps_val:       pure pval [integral_constant]
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

/* ********** val -> s_val -> ps_val meta-value-type concepts ************ */

// val <T, V = unqualified return type of T::operator()()const, or void>
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
concept val =
    std::is_same_v<V, remove_cvref_t<typename T::value_type>>
 && std::is_same_v<typename T::value_type, impl::functor_return_or_fail<T>>
 && (std::is_same_v<typename T::value_type,std::remove_cv_t<decltype(T::value)>>
  || std::is_same_v<typename T::value_type, decltype(T::value)>)
 && std::is_convertible_v<T, typename T::value_type>;
)

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
inline constexpr bool is_val_v =
                     CPP_CONCEPTS(val<T,V>)
                  NO_CPP_CONCEPTS(false);
NO_CPP_CONCEPTS(
template <typename T>
inline constexpr bool is_val_v<T, std::common_type_t<decltype(T::value),
                                                     typename T::value_type>> =
    std::is_same_v<typename T::value_type, impl::functor_return_or_fail<T>>
 && (std::is_same_v<typename T::value_type,std::remove_cv_t<decltype(T::value)>>
  || std::is_same_v<typename T::value_type, decltype(T::value)>)
 && std::is_convertible_v<T, typename T::value_type>;
)

// s_val <T, V = see-above*>
//   'static val' concept matches a val whose value or id is usable as an NTTP
//   (it is a structural type) and is itself a default constructible empty type
//
CPP_CONCEPTS(

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
concept s_val = val<T,V>
             && std::is_empty_v<T>
             && impl::structural_functor_value<T{}>;
)
template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
inline constexpr bool is_s_val_v =
                     CPP_CONCEPTS(s_val<T,V>)
                  NO_CPP_CONCEPTS(is_val_v<T,V>
                               && std::is_empty_v<T>
                               && impl::is_structural_functor_value<T>());

// ps_val <T, V = see-above*>
//   'pure static val' concept matches an s_val whose value is usable as an NTTP
//   (i.e. auto(value) is of structural type)
//
CPP_CONCEPTS(

template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
concept ps_val = s_val<T,V>
              && impl::auto_structural_functor_value<T{}>;
)
template <typename T,
          typename V = remove_cvref_t<impl::functor_value_type_or_void<T>>>
inline constexpr bool is_ps_val_v =
                      CPP_CONCEPTS(ps_val<T,V>)
                   NO_CPP_CONCEPTS(is_s_val_v<T,V>
                          && impl::is_auto_structural_functor_value<T>());

/* ************** 'typ' meta-type concept ******************* */

// typ concept for types that represent types, e.g. std::type_identity
//   requires empty class type with member type alias 'type' and no no-arg
//   call operator() (to disqualify std::integral_constant and derived types)
//
CPP_CONCEPTS(

template <typename T>
concept typ = requires { typename T::type; }
            && std::is_empty_v<T>
            && ! impl::value_functor<T>;
)
template <typename T, typename = void>
inline constexpr bool is_typ_v =
                    CPP_CONCEPTS(typ<T>)
                 NO_CPP_CONCEPTS(false);
NO_CPP_CONCEPTS(
template <typename T>
inline constexpr bool is_typ_v<T, std::void_t<typename T::type>>
                               = std::is_empty_v<T>
                              && ! impl::is_value_functor_v<T>;
)

#include "namespace.hpp" // close configurable namespace

#endif
