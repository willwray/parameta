#include <utility> // as_const

#include "parameta.hpp"

using namespace LML_NAMESPACE_ID;

#if __cpp_concepts
template <typename T> using is_structural_functor_value
    = std::bool_constant<impl::structural_functor_value<T{}>>;

template <typename T> using is_auto_structural_functor_value
    = std::bool_constant<impl::auto_structural_functor_value<T{}>>;
#else
using impl::structural_functor_value;
using impl::auto_structural_functor_value;
#endif

// Code from readme.md
#if __cpp_concepts
  //  metavalue auto value0 = 0;         // FAIL: 0 is not a metavalue
      metavalue auto value1 = dynameta{1};
      metavalue auto value2 = parameta<2>{};

  //  metavalue<char> auto nchar = parameta<0x0>{}; // FAIL: 0 is not a char
      metavalue<char> auto char0 = parameta<'0'>{};
      metavalue<char> auto stdval = std::integral_constant<char,0>{};

  //  metastatic auto wrong = dynameta{4};   // FAIL: dynameta is not metastatic
      metastatic auto right = pv<5>;
      const float flo[4][4]{};
      metastatic<float[4][4]> auto floref = pv<(flo)>; // refers to global var flo

  //  cptype<float> auto floref = parameta<(flo)>{}; // FAIL: flo not constexpr
#if not defined(__clang__)
      metaconst auto pi = parameta<3.14159f>{};
#endif

static_assert(&floref()==&flo);
// end code from readme

// Pass metavalue by-value, can't use arg as constexpr in function!
constexpr auto sz = [](metavalue auto v) {
      constexpr
      auto s = size(decltype(v){}); // size(v) is not constexpr
      return s;
    }(dynameta{1});
static_assert( sz == 1 );

metatype auto tint = ty<int>;
metatype auto Tint = typemeta<int>{};
#endif

static_assert( xtra<0>(ty<int,1>) == 1 );

// type

#if __cpp_concepts
#else
template <typename T, typename U = remove_cvref_t<impl::functor_value_type_or_void<T>>> inline constexpr bool metatype = is_typ_v<T,U>;
template <typename T, typename U = remove_cvref_t<impl::functor_value_type_or_void<T>>> inline constexpr bool metavalue = is_val_v<T,U>;
template <typename T, typename U = remove_cvref_t<impl::functor_value_type_or_void<T>>> inline constexpr bool metastatic = is_s_val_v<T,U>;
template <typename T, typename U = remove_cvref_t<impl::functor_value_type_or_void<T>>> inline constexpr bool metaconst = is_ps_val_v<T,U>;
#endif

struct empty {};
struct trait : std::true_type {};
struct bc { using type = std::true_type; };

static_assert( ! metatype<empty> );
static_assert(   metatype<typemeta<int>> );
static_assert( ! metatype<type_t<typemeta<int>>> );
static_assert(   metatype<std::add_lvalue_reference<int>> );

static_assert( ! metatype<std::true_type> );
static_assert( ! metatype<std::integral_constant<int,1>> );
static_assert( std::is_same_v<impl::functor_return_or_fail<trait>,bool> );
static_assert( /*! metatype<trait>*/ impl::is_value_functor_v<trait> );
static_assert(   metatype<bc> );

typemeta<int> Tt = ty<int>;
static_assert( std::is_same_v< int, type_t<typemeta<int>> > );
static_assert( std::is_same_v< int, typemeta<int>::type > );

// metavalue

struct arg7 { using value_type = int; int value; int operator()()const;                 };
struct argb { using value_type = int; int value;                        operator int(); };
struct argd { using value_type = int;            int operator()()const; operator int(); };
struct arge {                         int value; int operator()()const; operator int(); };
struct argf { using value_type = int; int value; int operator()()const; operator int(); };

static_assert( ! metavalue<arg7> );
static_assert( ! metavalue<argb> );
static_assert( ! metavalue<argd> );
static_assert( ! metavalue<arge> );
static_assert(   metavalue<argf> );

static_assert( metavalue<std::true_type> );
static_assert( metastatic<std::true_type> );
static_assert( metaconst<std::true_type> );

int ref;

static_assert( metavalue<std::integral_constant<int&, ref>> );
static_assert( metastatic<std::integral_constant<int&, ref>> );
static_assert( ! metaconst<std::integral_constant<int&, ref>> );

[[maybe_unused]] auto one = []{return 1;};
static_assert( ! metavalue<decltype(one)> );

// metastatic

static_assert( ! metastatic<argf> );

using VT = int const&;
struct pd { using value_type = VT; inline static VT value{ref};           VT operator()()const{return value;}; operator VT(); };
struct pe { using value_type = VT;               VT value{ref}; constexpr VT operator()()const{return value;}; operator VT(); };
struct pf { using value_type = VT; inline static VT value{ref}; constexpr VT operator()()     {return value;}; operator VT(); };
struct pg { using value_type = VT; inline static VT value{ref}; constexpr VT operator()()const{return value;}; operator VT(); };

static_assert( ! metastatic<pd> );
static_assert( ! metastatic<pe> );
static_assert( ! metastatic<pf> );
static_assert(   metastatic<pg> );

// metaconst

static_assert( ! metaconst<pg> );

struct cd { using value_type = int; static const int value{};           int operator()(){return value;}; operator int(); };
struct ce { using value_type = int; static       int value;             int operator()(){return value;}; operator int(); };
struct cf { using value_type = int; static const int value{}; constexpr int operator()()const{return value;}; operator int(); };

static_assert( ! metaconst<cd> );
static_assert( ! metaconst<ce> );
static_assert(   metaconst<cf> );


// parameta

int const global_int_const = 1;
using global_by_ref = parameta<(global_int_const)>;

static_assert( is_auto_structural_functor_value< global_by_ref>() ); // by id
static_assert( metaconst<global_by_ref> ); // by id
static_assert( size(global_by_ref{}) == 1 );

auto ft() {

static int const int_const = 1;
struct ic { constexpr int operator()()const{ return int_const; }};
static_assert( impl::auto_structural_or_fail<ic{}()> );
static_assert( is_auto_structural_functor_value<ic>() ); // value

static int /*const*/ non_const = 1;
struct nc { int operator()()const{ return non_const; }};
static_assert( ! is_auto_structural_functor_value<nc>() ); // value
struct ncref { constexpr int& operator()()const{ return non_const; }};
static_assert( ! is_auto_structural_functor_value<ncref>() ); // id
struct staticlocal { int const& operator()()const{static constexpr int i = 1; return i;} };
static_assert( ! is_auto_structural_functor_value<staticlocal>() );

using param_by_value = parameta<int_const>;
static_assert( is_auto_structural_functor_value< param_by_value >() ); // by value
static_assert( metaconst<param_by_value> ); // by value

//auto param_by_ref = parameta<std::as_const(int_const)>{};
using param_by_ref = parameta<static_cast<int const&>(int_const)>;
#ifndef _MSC_VER  // MSVC thinks this is not a constant expression
static_assert( is_auto_structural_functor_value< param_by_ref>() ); // by id
static_assert( metaconst<param_by_ref> ); // by id
#endif

//auto param_mutable = parameta<non_const>{}; // not valid by value
using param_mutable_id = parameta<std::as_const(non_const)>; // ok by id, but
static_assert( ! is_auto_structural_functor_value< param_mutable_id>() ); // not constexpr
static_assert( ! metaconst<param_mutable_id> ); // not constexpr

static_assert( metastatic<parameta<true>,bool> );
static_assert( metastatic<parameta<false>,bool> );
static_assert( ! metastatic<parameta<false>,int> );

#if __cpp_concepts
metastatic<bool> auto pbool = parameta<true>{};
constexpr metastatic<int> auto pint = param_by_ref{};

return pbool + pint;
#endif
return 1;
}


#if __cpp_concepts
template <argf> struct Argf : parameta<true> {};
#if not defined(__clang__)
Argf<{1}> argf_cnttp;
#else
Argf<argf{1}> argf_cnttp;
#endif

metaconst<bool> auto True = std::true_type{};
metaconst<bool> auto False = std::false_type{};
static_assert( True && ! False );
constexpr auto tk = []<metaconst<bool>>(){return true;}.template operator()<std::true_type>();
static_assert(tk);
#endif

// xtras

static_assert(std::is_same_v<int, decltype(get<1>(parameta<0,1,2,3>{}))> );
int glober=0;
constexpr int const& conster(int& a){return a;}
static_assert(std::is_same_v<int const&, decltype(get<1>(parameta<0,(int const&)(glober),2,3>{}))> );

auto p0123 = parameta<0,1,2,3>{};

static_assert( get<0>(p0123) == 0 );
static_assert( get<1>(p0123) == 1 );
static_assert( get<2>(p0123) == 2 );
static_assert( get<3>(p0123) == 3 );

static_assert( xtra<0>(p0123) == 1 );
static_assert( xtra<1>(p0123) == 2 );
static_assert( xtra<2>(p0123) == 3 );

constexpr
auto d0123 = dynameta<int,1,2,3>{};

static_assert( get<0>(dynameta<int,1,2,3>{0}) == 0 );
static_assert( get<0>(d0123) == 0 );
static_assert( get<1>(dynameta<int,1,2,3>{}) == 1 );
static_assert( get<2>(dynameta<int,1,2,3>{}) == 2 );
static_assert( get<3>(dynameta<int,1,2,3>{}) == 3 );

#ifdef _MSC_VER
#define NUA [[msvc::no_unique_address]]
#else
#define NUA [[no_unique_address]]
#endif

#if __cpp_concepts
template <typename elem, metavalue extentt>
constexpr auto intrinsic_array_type()
{
  if constexpr (metaconst<extentt>)
    return ty<elem[extentt::value]>;
  else
    return ty<elem[]>;
};

template <typename elem, metavalue extt>
using intrinsic_array_t =
      typename decltype(intrinsic_array_type<elem,extt>())::type;

template <metatype elementt, metavalue extentt>
struct array
{
  using array_t = intrinsic_array_t<type_t<elementt>, extentt>;

  NUA extentt extent{};

#include "ALLOW_ZERO_SIZE_ARRAY.hpp"
  NUA array_t data;
#include "ALLOW_ZERO_SIZE_ARRAY.hpp"

  auto begin() {return data;}
  auto end() {return data+extent;}
};

template <metatype elementt, metavalue extentt>
struct span
{
  NUA elementt element_type{};
  NUA extentt extent{};
  type_t<elementt>* data;

  auto begin() {return data;}
  auto end() {return data+extent;}
};
#if defined(__clang__)
template <metatype T, metavalue v>
span(T,v, type_t<T>*) -> span<T,v>;
#endif

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#endif

int main()
{
#if __cpp_concepts
  //std::array<int,4> a4;
  array< typemeta<int>, parameta<4> > int4;

  unsigned char storage[16*sizeof(int)];

  using ints = array< typemeta<int>, dynameta<int> >;
  constexpr auto ints_data = offsetof(ints,data);

#include "ALLOW_ZERO_SIZE_ARRAY.hpp"
  auto pint = reinterpret_cast<ints*>(new (storage) int{4});
#include "ALLOW_ZERO_SIZE_ARRAY.hpp"

  assert( pint->begin() == pint->data );

  for (int i=0; auto& e : int4) e = i++;
  for (int i=0; auto& e : *pint) e = i++;
  for (int i=0; i != 4; ++i)
    if (*(int4.begin()+i) != i
     || *reinterpret_cast<int*>(&storage[ints_data + i*sizeof(int)]) != i )
       assert(false);

  static_assert( int4.extent == 4 );
  static_assert( sizeof int4 == sizeof(int) * 4 );
  static_assert( sizeof(ints) == sizeof(int) );

  span spint4{ty<int>, parameta<4>{}, int4.begin()};
  span dpint4{ty<int>, dynameta<char>{4}, int4.begin()};

  return !(spint4.begin() == dpint4.begin());
#endif
}
