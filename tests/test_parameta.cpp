#include "parameta_traits.hpp"

#define SAME std::is_same_v

#define REMOVE_REF_T(...) std::remove_reference_t<__VA_ARGS__>
#if __cpp_lib_remove_cvref
#define REMOVE_CVREF_T(...) std::remove_cvref_t<__VA_ARGS__>
#else
#define REMOVE_CVREF_T(...) std::remove_cv_t<REMOVE_REF_T(__VA_ARGS__)>
#endif

template <typename T>
struct typex { using type = T; };

template <typename T> auto make_unsigned();

template <typename T> using make_unsigned_t = typename decltype(
                            make_unsigned<REMOVE_REF_T(T)>())::type;

template <typename T> auto make_unsigned()
{
  if constexpr (! std::is_reference_v<T>) {
    if constexpr (std::is_integral_v<T>)
      return std::make_unsigned<T>{};
    else
      return typex<T>{};
  } else {
    using U = make_unsigned_t<T>;
    if constexpr (std::is_lvalue_reference_v<T>)
      return std::add_lvalue_reference<U>{};
    else
      return std::add_rvalue_reference<U>{};
  }
}
template <decltype(auto) v>
constexpr inline decltype(auto) make_unsigned_v
                              = make_unsigned_t<decltype(v)>{v};

using namespace LML_NAMESPACE_ID;

// Test parameta_traits concepts and/or corresponding C++17 traits

#if __cpp_concepts
#  define METAVALUE(...) (metavalue<__VA_ARGS__> \
                    && is_metavalue_v<__VA_ARGS__>)
#  define METASTATIC(...)(metastatic<__VA_ARGS__> \
                    && is_metastatic_v<__VA_ARGS__>)
#  define METACONST(...) (metaconst<__VA_ARGS__> \
                    && is_metaconst_v<__VA_ARGS__>)
#  define METATYPE(...)  (metatype<__VA_ARGS__> \
                    && is_metatype_v<__VA_ARGS__>)
#  define METAPARA(...)  (metapara<__VA_ARGS__> \
                    && is_metapara_v<__VA_ARGS__>)
#else
#  define METAVALUE(...) is_metavalue_v<__VA_ARGS__>
#  define METASTATIC(...) is_metastatic_v<__VA_ARGS__>
#  define METACONST(...) is_metaconst_v<__VA_ARGS__>
#  define METATYPE(...) is_metatype_v<__VA_ARGS__>
#  define METAPARA(...) is_metapara_v<__VA_ARGS__>
#endif

/****** METATYPE *********/
// Test metatype concept

struct empty {};
struct voidt {using type = void;};
struct ftorm : voidt {void operator()(){};};
struct ftorc : voidt {void operator()()const{};};

static_assert( ! METATYPE(empty) &&  ! METAPARA(empty) );
static_assert(   METATYPE(voidt) &&    METAPARA(voidt) );
static_assert(   METATYPE(ftorm) &&    METAPARA(ftorm) );
static_assert( ! METATYPE(ftorc) &&  ! METAPARA(ftorc) );

static_assert(   METATYPE(std::add_lvalue_reference<int>) );
static_assert( ! METATYPE(std::true_type) );

/****** META VALUE std **********/
// Test meta value concepts on std types

static_assert( METAVALUE (std::true_type) );
static_assert( METASTATIC(std::true_type) );
static_assert( METACONST (std::true_type) );

static_assert( METAVALUE (std::true_type, bool) );

int global;

static_assert(  METAVALUE (std::integral_constant<int&, global>) );
static_assert(  METASTATIC(std::integral_constant<int&, global>) );
static_assert( ! METACONST(std::integral_constant<int&, global>) );

static_assert(  METAVALUE (std::integral_constant<int&, global>, int) );

#if __cpp_concepts
template <metatype t, metavalue<int> n> struct meta_array;

metavalue auto any_value_type = std::integral_constant<int,42>{}; // ok
metavalue<char> auto charchar = std::integral_constant<char,42>{}; // ok
//metavalue<char> auto mismatch = std::integral_constant<int,42>{}; // FAIL
#endif

/****** METAVALUE *********/
//      metavalue<T,V>

template <typename VT> struct test_metavalue {

using UT = make_unsigned_t<VT>;

#define VALUE_TYPE using value_type = VT;
#define VALUE VT value;
#define CALL_OPC VT operator()()const;
#define IMPLCONV operator VT()const;

#define VALUu    UT value;
#define CALL_OPm VT operator()();
#define CALL_OPu UT operator()()const;
#define IMPLCONm operator VT();
#define ExPLCONV explicit operator VT()const;
#define IMPLCONu operator UT()const;

struct TVCo {VALUE_TYPE VALUE CALL_OPC         }; // Missing conv-op
struct TVoI {VALUE_TYPE VALUE          IMPLCONV}; // Missing call-op
struct ToCI {VALUE_TYPE       CALL_OPC IMPLCONV}; // Missing value
struct oVCI {           VALUE CALL_OPC IMPLCONV}; // Missing value_type

struct TVCI {VALUE_TYPE VALUE CALL_OPC IMPLCONV}; // Full house

struct TuCI {VALUE_TYPE VALUu CALL_OPC IMPLCONV}; // value wrong type
struct TVmI {VALUE_TYPE VALUE CALL_OPm IMPLCONV}; // call-op non-const
struct TVuI {VALUE_TYPE VALUE CALL_OPu IMPLCONV}; // call-op wrong type
struct TVCm {VALUE_TYPE VALUE CALL_OPC IMPLCONm}; // conv-op non-const
struct TVCx {VALUE_TYPE VALUE CALL_OPC ExPLCONV}; // conv-op explicit
struct TVCu {VALUE_TYPE VALUE CALL_OPC IMPLCONu}; // conv-op wrong type

#undef VALUE_TYPE
#undef VALUE
#undef CALL_OPC
#undef IMPLCONV

#undef VALUu
#undef CALL_OPm
#undef CALL_OPu
#undef IMPLCONm
#undef ExPLCONV
#undef IMPLCONu

static_assert( ! METAVALUE(TVCo) ); // Missing conv-op
static_assert( ! METAVALUE(TVoI) ); // Missing call-op
static_assert( ! METAVALUE(ToCI) ); // Missing value
static_assert( ! METAVALUE(oVCI) ); // Missing value_type

static_assert(   METAVALUE(TVCI) ); // Full house

static_assert( ! METAVALUE(TuCI) ); // value wrong type
static_assert( ! METAVALUE(TVmI) ); // call-op non-const
static_assert( ! METAVALUE(TVuI) ); // call-op wrong type
static_assert( ! METAVALUE(TVCm) ); // conv-op non-const
static_assert( ! METAVALUE(TVCx) ); // conv-op explicit
static_assert(   METAVALUE(TVCu) == std::is_convertible_v<TVCu,VT>);

static_assert( ! METASTATIC(TVCI) );
static_assert( ! METACONST (TVCI) );

}; // struct test_metavalue

test_metavalue<int>        metavalue_int;
test_metavalue<int&>       metavalue_intref;
test_metavalue<int const>  metavalue_intc;
test_metavalue<int&&>      metavalue_intrefref;
test_metavalue<int const&> metavalue_intcref;

/**** METASTATIC ******* METACONST ******/
//    metastatic<T,V> &  metaconst<T,V>

template <decltype(auto) v,
          bool ConstExpr = true> struct test_metastatic {

using VT = decltype(v);
//static constexpr decltype(auto) u = make_unsigned_v<v>;

#define VALUE_TYPE using value_type = VT;
#define VALUE inline static VT value = v;
#define CALL_OPC VT operator()()const{return value;}
#define IMPLCONV operator VT()const;
// note implicit conversion is not required to be constexpr (should it?)

#define VALUe VT value = v;

struct TVcI {VALUE_TYPE constexpr VALUE           CALL_OPC IMPLCONV};
struct TvCI {VALUE_TYPE           VALUe constexpr CALL_OPC IMPLCONV};
struct TVCI {VALUE_TYPE constexpr VALUE constexpr CALL_OPC IMPLCONV};

static_assert( ! METASTATIC(TVcI) );
static_assert( ! METASTATIC(TvCI) );
static_assert(   METASTATIC(TVCI) );

static_assert( ! METACONST(TVcI) );
static_assert( ! METACONST(TvCI) );
static_assert(   METACONST(TVCI) == ConstExpr );

}; // struct test_metastatic

#include <utility> // as_const

constexpr int constant{};

[[maybe_unused]] test_metastatic<constant>   metastatic_int;
[[maybe_unused]] test_metastatic<(constant)> metastatic_intcref;

[[maybe_unused]] test_metastatic<(global), false> metastatic_intref;

constexpr void function()noexcept{}

[[maybe_unused]]
#ifndef _MSC_VER
test_metastatic<(function)>
#else
test_metastatic<static_cast<void(&)()noexcept>(function)>
#endif
metastatic_function;


#include "parameta.hpp"

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

// dynameta<void> can be named but not instantiated
using voidmeta = dynameta<void>;

static_assert( ! METAVALUE(void) );

using MSF = decltype(makestatic<function>());

# ifndef _MSC_VER
using SF = staticmeta<(function)>;
#else
using SF = staticmetacast<void(&)()noexcept,function>; // MSVC requires a cast
#endif

static_assert( SAME<SF,MSF> );
static_assert( METACONST( SF, void()noexcept) );

static_assert( METACONST( staticmeta<true>, bool) );
static_assert( METACONST( staticmeta<false>,bool) );

constexpr int k = 42;
static float a[2] = {};
void func(){}

static_assert( ! std::is_reference_v<decltype(makestatic<42>()())> );
static_assert( ! std::is_reference_v<decltype(makestatic<k>()())> );
static_assert( ! std::is_reference_v<decltype(makestatic<(k)>()())> );
static_assert(   std::is_reference_v<decltype(makestatic<a>()())> );
static_assert(   std::is_reference_v<decltype(makestatic<func>()())> );


static_assert( staticmeta<0,1,2,3>::metaget<0>() == 1 );
static_assert( SAME<decltype(staticmeta<0,1,2,3>::metaget())
                             , staticmeta<1,2,3>> );
static_assert( SAME<decltype(staticmeta<0,1,2,3>::metaget<2,1,0>())
                             , staticmeta<3,2,1>> );

auto p0123 = staticmeta<0,1,2,3>{};

static_assert( p0123.metasize() == 3 );
static_assert( p0123 == 0 );
static_assert( p0123.metaget() == 1 );
static_assert( p0123.metaget().metaget() == 2 );
static_assert( p0123.metaget().metaget().metaget() == 3 );
static_assert( p0123.metaget<0>() == 1 &&  p0123.metaget<-3>() == 1 );
static_assert( p0123.metaget<1>() == 2 &&  p0123.metaget<-2>() == 2 );
static_assert( p0123.metaget<2>() == 3 &&  p0123.metaget<-1>() == 3 );

auto d0123 = dynameta<int,1,2,3>{};

static_assert( d0123.metasize() == 3 );
static_assert( d0123.metaget() == 1 );
static_assert( d0123.metaget<0>() == 1 && d0123.metaget<-3>() == 1 );
static_assert( d0123.metaget<1>() == 2 && d0123.metaget<-2>() == 2 );
static_assert( d0123.metaget<2>() == 3 && d0123.metaget<-1>() == 3 );

int main() {}

#if __cpp_concepts
#include <algorithm>
#include <memory>
using std::unique_ptr;
using std::make_unique;
using std::integral;

#ifndef _MSC_VER
# define NUA [[no_unique_address]]
#else
# define NUA [[msvc::no_unique_address]]
#endif

  template <typename Storage, metavalue Extent>
            requires (integral<typename Extent::value_type>
         && requires (Storage a) {a[0];})
  struct ray {
    NUA Storage data;
    NUA Extent extent{};
  };

template <typename T, int N> using array = ray<T[N], staticmeta<N>>;

  array<int,2>  i2 {{4,2}}; // ray<int[2],metastatic<2>>
  array<char,4> c4 {"str"}; // ray<char[4],metastatic<4>>

static_assert( sizeof c4 == 4 && c4.extent == 4 );

template <typename P> using span = ray<P,dynameta<int>>;

  char buffer[4];
  span<char*> eh{buffer,{4}};

  span<unique_ptr<char[]>> up{make_unique<char[]>(4),{4}};

  ray<char(&)[4], staticmeta<4>> sp{buffer};

  ray<staticmetacast<char(&)[4], buffer>, staticmeta<4>> sb{};

static_assert( sizeof eh == 16 );
static_assert( sizeof up == 16 );
static_assert( sizeof sp == 8 );
static_assert( sizeof c4 == 4 );
static_assert( sizeof sb == 1 );

void cp() {
  std::copy_n("1234", eh.extent, &eh.data[0]);
  std::copy_n("1234", up.extent, &up.data[0]);
  std::copy_n("1234", sp.extent, &sp.data[0]);
  std::copy_n("1234", c4.extent, &c4.data[0]);
  std::copy_n("1234", sb.extent, &sb.data[0]);
}
#endif
