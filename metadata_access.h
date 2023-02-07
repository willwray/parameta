#ifndef METADATA_ACCESS_OFF /*

  metadata_access.h
  =================
  This is not a 'header' file. Header guards aren't needed or wanted.
  It is #include'd directly in-class to expand as 'vertical' codegen.
  To disable codegen either remove this file or #define METAGET empty.
  Assumes a value pack x... is defined.

  Common accessors for metadata x... in staticmeta, dynameta, typemeta:

  * metasize() -> sizeof...(x)

  * metaget() -> staticmeta<x...>
    metaget<I>() -> staticmeta<xI>
    metaget<I...>() -> staticmeta<xI...>
                    where xI is the Ith x... argument
*/

static CONSTEVAL decltype(auto) metasize() noexcept
{
  return sizeof...(x);
}

template <int...I>
static CONSTEVAL decltype(auto) metaget() noexcept
{
  constexpr int size{sizeof...(x)};
  static_assert(size != 0, "metaget on type with no metadata");

  if constexpr (sizeof...(I) == 0)
    return staticmeta<x...>{};
  else if constexpr (sizeof...(I) > 1)
    return staticmeta<metaget<I>()()...>{};
  else {
    constexpr int K {(I,...)};
    constexpr int i {K<0 ? K+size : K};

    static_assert( i >= 0 && i < size, "metaget index out of bounds");

    if constexpr (i == 0)
      return staticmeta<staticmeta<x...>{}()>{};
    else
      return staticmeta<x...>::template metaget<i - 1>();
  }
}

#endif
