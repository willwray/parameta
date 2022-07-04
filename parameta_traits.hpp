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

#include <concepts>

#include "namespace.hpp" // open namespace LML_NAMESPACE_ID

// impl::const_if_reference<T> concept = true for non-reference T
//   else if T is a reference then true if it's a const reference
//
template <typename T>
concept const_if_reference =
       ! std::is_reference_v<T>
      || std::is_const_v<std::remove_reference_t<T>>;

namespace impl {

// impl::vfunctor<L> minimal 'value functor' requirement; has operator()()const
//
template <typename L>
concept vfunctor = requires (L const l) {l.operator()();};

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

/* ************** 'typ' meta-type concept ******************* */

// typ concept for types that represent types, e.g. std::type_identity
//   requires empty class type with member type alias 'type' and no no-arg
//   call operator() (to disqualify std::integral_constant and derived types)
//
template <typename T>
concept typ = requires { typename T::type; }
            && std::is_empty_v<T>
            && ! impl::vfunctor<T>;

#include "namespace.hpp" // close configurable namespace

#endif
