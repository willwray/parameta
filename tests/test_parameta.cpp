#include <utility> // as_const

#include "parameta.hpp"

using namespace LML_NAMESPACE_ID;

// Code from readme.md

  //  val auto value0 = 0;         // FAIL: 0 is not a val
      val auto value1 = dval{1};
      val auto value2 = pval<2>{};

  //  val<char> auto nchar = pval<0x0>{}; // FAIL: 0 is not a char
      val<char> auto char0 = pval<'0'>{};
      val<char> auto stdval = std::integral_constant<char,0>{};

  //  s_val auto wrong = dval{4};   // FAIL: dval is not s_val
      s_val auto right = pv<5>;
      const float flo[4][4]{};
      s_val<float[4][4]> auto floref = pv<(flo)>; // refers to global var flo

  //  cptype<float> auto floref = pval<(flo)>{}; // FAIL: flo not constexpr
      ps_val auto pi = pval<3.14159f>{};

// end code from readme

// Pass val by-value, can't use arg as constexpr in function!
constexpr auto sz = [](val auto v) {
      constexpr
      auto s = size(decltype(v){}); // size(v) is not constexpr
      return s;
    }(dval{1});

static_assert( xtra<0>(ty<int,1>) == 1 );

// is_auto_NTTP_functor(k)
// is_decltype_auto_NTTP_functor(k)
// functions to test if the functor argument is a valid NTTP
//
template <typename K>
constexpr bool is_auto_NTTP_functor(K) {
  return requires { impl::auto_structural<K{}()>; };
}
template <typename K>
constexpr bool is_decltype_auto_NTTP_functor(K) {
  return requires { impl::structural<K{}()>; };
}

// type

struct empty {};
struct trait : std::true_type {};
struct bc { using type = std::true_type; };

static_assert( ! typ<empty> );
static_assert(   typ<std::type_identity<int>> );
static_assert( ! typ<std::type_identity_t<int>> );
static_assert(   typ<std::add_lvalue_reference<int>> );

static_assert( ! typ<std::true_type> );
static_assert( ! typ<std::integral_constant<int,1>> );
static_assert( ! typ<trait> );
static_assert(   typ<bc> );

typ auto tint = ty<int>;
typ auto Tint = metatype<int>{};

metatype<int> Tt = ty<int>;
static_assert( std::same_as< int, type_t<metatype<int>> > );
static_assert( std::same_as< int, metatype<int>::type > );

// val

struct arg7 { using value_type = int; int value; int operator()()const;                 };
struct argb { using value_type = int; int value;                   operator int(); };
struct argd { using value_type = int;            int operator()()const; operator int(); };
struct arge {                         int value; int operator()()const; operator int(); };
struct argf { using value_type = int; int value; int operator()()const; operator int(); };

static_assert( ! val<arg7> );
static_assert( ! val<argb> );
static_assert( ! val<argd> );
static_assert( ! val<arge> );
static_assert(   val<argf> );

template <argf> struct Argf : pval<true> {};
Argf<{1}> argf_cnttp;

static_assert( val<std::true_type> );
static_assert( s_val<std::true_type> );
static_assert( ps_val<std::true_type> );

int ref;

static_assert( val<std::integral_constant<int&, ref>> );
static_assert( s_val<std::integral_constant<int&, ref>> );
static_assert( ! ps_val<std::integral_constant<int&, ref>> );

static_assert( ! val<decltype([]{return 1;})> );

// s_val

static_assert( ! s_val<argf> );

using VT = int const&;
struct pd { using value_type = VT; inline static VT value{ref};           VT operator()()const{return value;}; operator VT(); };
struct pe { using value_type = VT;               VT value{ref}; constexpr VT operator()()const{return value;}; operator VT(); };
struct pf { using value_type = VT; inline static VT value{ref}; constexpr VT operator()()     {return value;}; operator VT(); };
struct pg { using value_type = VT; inline static VT value{ref}; constexpr VT operator()()const{return value;}; operator VT(); };

static_assert( ! s_val<pd> );
static_assert( ! s_val<pe> );
static_assert( ! s_val<pf> );
static_assert(   s_val<pg> );

// ps_val

static_assert( ! ps_val<pg> );

struct cd { using value_type = int; static const int value{};           int operator()(){return value;}; operator int(); };
struct ce { using value_type = int; static       int value;             int operator()(){return value;}; operator int(); };
struct cf { using value_type = int; static const int value{}; constexpr int operator()()const{return value;}; operator int(); };

static_assert( ! ps_val<cd> );
static_assert( ! ps_val<ce> );
static_assert(   ps_val<cf> );


// pval

int const global_int_const = 1;
auto global_by_ref = pval<(global_int_const)>{};

static_assert( is_auto_NTTP_functor( global_by_ref ) ); // by id
static_assert( ps_val<decltype(global_by_ref)> ); // by id
static_assert( size(global_by_ref) == 1 );

auto ft() {

static int const int_const = 1;
static_assert( is_auto_NTTP_functor([]{return int_const;}) );

static int /*const*/ non_const = 1;
static_assert( ! is_auto_NTTP_functor([]{return non_const;}) ); // value
static_assert( ! is_auto_NTTP_functor([]()->auto&{return non_const;}) ); // id

static_assert( ! is_auto_NTTP_functor([]{static constexpr int i = 1;
                                        return i;}) );

auto param_by_value = pval<int_const>{};
static_assert( is_auto_NTTP_functor( param_by_value ) ); // by value
//static_assert( ps_val<decltype(param_by_value)> ); // by value (FAIL on gcc)
static_assert( ps_val<pval<int_const>> ); // by value

//auto param_by_ref = pval<std::as_const(int_const)>{};
auto param_by_ref = pval<static_cast<int const&>(int_const)>{};
#ifndef _MSC_VER  // MSVC thinks this is not a constant expression
static_assert( is_auto_NTTP_functor( param_by_ref ) ); // by id
static_assert( ps_val<decltype(param_by_ref)> ); // by id
#endif

//auto param_mutable = pval<non_const>{}; // not valid by value
auto param_mutable_id = pval<std::as_const(non_const)>{}; // ok by id, but
static_assert( ! is_auto_NTTP_functor( param_mutable_id ) ); // not constexpr
static_assert( ! ps_val<decltype(param_mutable_id)> ); // not constexpr

static_assert( s_val<pval<true>,bool> );
static_assert( s_val<pval<false>,bool> );
static_assert( ! s_val<pval<false>,int> );

s_val<bool> auto pbool = pval<true>{};
constexpr s_val<int> auto pint = param_by_ref;

return pbool + pint;
}



ps_val<bool> auto True = std::true_type{};
ps_val<bool> auto False = std::false_type{};
static_assert( True && ! False );
constexpr auto tk = []<ps_val<bool>>(){return true;}.template operator()<std::true_type>();

// xtras

static_assert(std::same_as<int, decltype(get<1>(pval<0,1,2,3>{}))> );
int glober=0;
consteval int const& conster(int& a){return a;}
static_assert(std::same_as<int const&, decltype(get<1>(pval<0,(int const&)(glober),2,3>{}))> );

val auto p0123 = pval<0,1,2,3>{};

static_assert( get<0>(p0123) == 0 );
static_assert( get<1>(p0123) == 1 );
static_assert( get<2>(p0123) == 2 );
static_assert( get<3>(p0123) == 3 );

static_assert( xtra<0>(p0123) == 1 );
static_assert( xtra<1>(p0123) == 2 );
static_assert( xtra<2>(p0123) == 3 );

constexpr
val auto d0123 = dval<int,1,2,3>{};

static_assert( get<0>(dval<int,1,2,3>{0}) == 0 );
static_assert( get<0>(d0123) == 0 );
static_assert( get<1>(dval<int,1,2,3>{}) == 1 );
static_assert( get<2>(dval<int,1,2,3>{}) == 2 );
static_assert( get<3>(dval<int,1,2,3>{}) == 3 );

#ifndef _MSC_VER
#define NUA [[msvc::no_unique_address]]
#else
#define NUA [[msvc::no_unique_address]]
#endif

template <typename elem, val extentt>
constexpr auto intrinsic_array_type()
{
  if constexpr (ps_val<extentt>)
    return ty<elem[extentt::value]>;
  else
    return ty<elem[]>;
};

template <typename elem, val extt>
using intrinsic_array_t =
      typename decltype(intrinsic_array_type<elem,extt>())::type;

template <typ elementt, val extentt>
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

template <typ elementt, val extentt>
struct span
{
  NUA elementt element_type{};
  NUA extentt extent{};
  type_t<elementt>* data;

  auto begin() {return data;}
  auto end() {return data+extent;}
};

#include <cassert>
#include <cstddef>
#include <cstdio>

int main()
{
  //std::array<int,4> a4;
  array< metatype<int>, pval<4> > int4;

  unsigned char storage[16*sizeof(int)];

  using ints = array< metatype<int>, dval<int> >;
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

  span spint4{ty<int>, pval<4>{}, int4.begin()};
  span dpint4{ty<int>, dval<char>{4}, int4.begin()};

  return !(spint4.begin() == dpint4.begin());
}
