/*
   This file is part of PLYwoot, a header-only PLY parser.

   Copyright (C) 2023-2024, Ton van den Heuvel

   PLYwoot is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLYWOOT_TYPE_TRAITS_HPP
#define PLYWOOT_TYPE_TRAITS_HPP

#include "reflect.hpp"
#include "std.hpp"
#include "types.hpp"

#include <algorithm>
#include <type_traits>

namespace plywoot { namespace detail {

/// Returns whether the given type `T` is considered to be a list.
/// @{
template<typename T>
struct IsList
{
  static constexpr bool value = false;
};

template<typename T, std::size_t N>
struct IsList<reflect::Array<T, N>>
{
  static constexpr bool value = true;
};

template<typename T>
struct IsList<std::vector<T>>
{
  static constexpr bool value = true;
};

template<typename T>
struct IsList<reflect::Type<T>>
{
  static constexpr bool value = IsList<T>::value;
};
/// @}

/// Returns whether the given reflection type represents a list.
template<typename T>
constexpr bool isList()
{
  return IsList<T>::value;
}

/// Given a reflect type, stores the number of properties spanned by the
/// reflection type. By default, every reflection type spans one property,
/// except for `plywoot::reflect::Pack`, which spans multiple properties by
/// definition.
template<typename... Ts>
struct NumProperties
{
  static constexpr std::size_t size = 0;
};

template<typename T, typename... Ts>
struct NumProperties<T, Ts...>
{
  static constexpr std::size_t size = NumProperties<T>::size + NumProperties<Ts...>::size;
};

template<typename T, std::size_t N>
struct NumProperties<reflect::Array<T, N>>
{
  static constexpr std::size_t size = 1;
};

template<typename T, std::size_t N>
struct NumProperties<reflect::Pack<T, N>>
{
  static constexpr std::size_t size = N;
};

template<typename T>
struct NumProperties<T>
{
  static constexpr std::size_t size = 1;
};

/// Returns the number of properties spanned by the given list of reflection
/// types. By default, every reflection type spans one property, except for
/// `plywoot::reflect::Pack`, which spans multiple properties by definition.
template<typename... Ts>
std::size_t numProperties()
{
  return NumProperties<Ts...>::size;
}

/// Returns whether an object of type `T` represents that same object as an
/// object of the given PLY data type `type`.
template<typename T>
bool isSame(PlyDataType type)
{
  switch (type)
  {
    case PlyDataType::Char:
      return std::is_same<T, char>::value;
    case PlyDataType::UChar:
      return std::is_same<T, unsigned char>::value;
    case PlyDataType::Short:
      return std::is_same<T, short>::value;
    case PlyDataType::UShort:
      return std::is_same<T, unsigned short>::value;
    case PlyDataType::Int:
      return std::is_same<T, int>::value;
    case PlyDataType::UInt:
      return std::is_same<T, unsigned int>::value;
    case PlyDataType::Float:
      return std::is_same<T, float>::value;
    case PlyDataType::Double:
      return std::is_same<T, double>::value;
  }

  return false;
}

/// Type function that returns whether a given type `T` is an instance of
/// `Pack<>`.
/// @{
template<typename T>
struct IsPack
{
  static constexpr bool value = false;
};

template<typename T, std::size_t N>
struct IsPack<reflect::Pack<T, N>>
{
  static constexpr bool value = true;
};
/// @}

/// Type function that returns the size of some type T, effectively implementing
/// sizeof(T), where it overrides sizeof() for types of instance Pack<>. For
/// pack types, the computed size is the sum of `SizeOf` for all types in the
/// pack, whereas for arrays it is the `SizeOf` of the element type in the array
/// times the number of elements in the array.
/// @{
template<typename T, typename... Ts>
struct SizeOf
{
  static constexpr auto size = SizeOf<T>::size + SizeOf<Ts...>::size;
};

template<typename T>
struct SizeOf<T>
{
  static constexpr auto size = sizeof(T);
};

template<typename T, std::size_t N>
struct SizeOf<reflect::Array<T, N>>
{
  static constexpr auto size = N * SizeOf<T>::size;
};

template<typename T, std::size_t N>
struct SizeOf<reflect::Pack<T, N>>
{
  static constexpr auto size = N * SizeOf<T>::size;
};
/// @}

/// Returns the size of some type T, effectively implementing sizeof(T), where
/// it overrides sizeof() for types of instance Pack<> and Array<>. For pack
/// types, the computed size is the sum of `SizeOf` for all types in the pack,
/// whereas for arrays it is the `SizeOf` of the element type in the array times
/// the number of elements in the array.
template<typename... Ts>
constexpr std::size_t sizeOf()
{
  return SizeOf<Ts...>::size;
}

/// Returns the size in bytes of the given PLY data type.
inline std::size_t sizeOf(PlyDataType type)
{
  switch (type)
  {
    case PlyDataType::Char:
    case PlyDataType::UChar:
      return 1;
    case PlyDataType::Short:
    case PlyDataType::UShort:
      return 2;
    case PlyDataType::Int:
    case PlyDataType::UInt:
    case PlyDataType::Float:
      return 4;
    case PlyDataType::Double:
      return 8;
  }

  return 0;
}

/// Aligns the given input pointer according to the size of the given PLY data
/// type.
template<typename Ptr>
Ptr align(Ptr ptr, PlyDataType type)
{
  switch (type)
  {
    case PlyDataType::Char:
      return detail::align(ptr, alignof(char));
    case PlyDataType::UChar:
      return detail::align(ptr, alignof(unsigned char));
    case PlyDataType::Short:
      return detail::align(ptr, alignof(short));
    case PlyDataType::UShort:
      return detail::align(ptr, alignof(unsigned short));
    case PlyDataType::Int:
      return detail::align(ptr, alignof(int));
    case PlyDataType::UInt:
      return detail::align(ptr, alignof(unsigned int));
    case PlyDataType::Float:
      return detail::align(ptr, alignof(float));
    case PlyDataType::Double:
      return detail::align(ptr, alignof(double));
  }

  return ptr;
}

/// Returns the alignment of the given PLY data type.
// TODO(ton): make `constexpr` when moving to C++17.
inline std::size_t alignof_(PlyDataType type)
{
  switch (type)
  {
    case PlyDataType::Char:
      return alignof(char);
    case PlyDataType::UChar:
      return alignof(unsigned char);
    case PlyDataType::Short:
      return alignof(short);
    case PlyDataType::UShort:
      return alignof(unsigned short);
    case PlyDataType::Int:
      return alignof(int);
    case PlyDataType::UInt:
      return alignof(unsigned int);
    case PlyDataType::Float:
      return alignof(float);
    case PlyDataType::Double:
      return alignof(double);
  }

  return 0;
}

/// Type function that returns whether a list of types is consecutively aligned
/// in memory, without any padding.
/// @{
template<typename T>
constexpr bool isPacked(uintptr_t offset = 0)
{
  return ((offset + alignof(T)) % alignof(T)) == 0;
}

template<typename T, typename U, typename... Ts>
constexpr bool isPacked(uintptr_t offset = 0)
{
  return ((offset + alignof(T)) % alignof(T)) == 0 && isPacked<U, Ts...>(offset + sizeOf<T>());
}
/// @}

/// Type function that returns whether all types in the given list of types are
/// trivially copyable.
/// @{
template<typename T>
constexpr bool isTriviallyCopyable()
{
  return std::is_trivially_copyable<T>::value;
}

template<typename T, typename U, typename... Ts>
constexpr bool isTriviallyCopyable()
{
  return std::is_trivially_copyable<T>::value && isTriviallyCopyable<U, Ts...>();
}
/// @}

/// Type that provides a function operator that returns whether a range of
/// properties in [`first`, `last`) represents PLY properties that can be
/// trivially copied to the given destination type `T`.
/// @{
template<typename T>
struct IsMemcpyable
{
  bool operator()(const PlyPropertyConstIterator first, const PlyPropertyConstIterator last) const
  {
    return first < last && isSame<T>(first->type());
  }
};

template<typename T, std::size_t N>
struct IsMemcpyable<reflect::Array<T, N>>
{
  bool operator()(const PlyPropertyConstIterator, const PlyPropertyConstIterator) const { return false; }
};
/// @}

template<typename T, std::size_t N>
struct IsMemcpyable<reflect::Pack<T, N>>
{
  bool operator()(const PlyPropertyConstIterator first, const PlyPropertyConstIterator last) const
  {
    return first + N <= last &&
           std::all_of(first, first + N, [](const PlyProperty &p) { return isSame<T>(p.type()); });
  }
};

/// Returns whether the range of properties in [`first`, `last`) represents PLY
/// properties that have the same type as the given type range. Note that a
/// single `reflect::Pack` type is compared with same number of properties as
/// are in the pack.
/// @{
template<typename T>
bool isMemcpyable(const PlyPropertyConstIterator first, const PlyPropertyConstIterator last)
{
  return first + detail::numProperties<T>() == last && IsMemcpyable<T>{}(first, last);
}

template<typename T, typename U, typename... Ts>
bool isMemcpyable(const PlyPropertyConstIterator first, const PlyPropertyConstIterator last)
{
  return IsMemcpyable<T>{}(first, last) && isMemcpyable<U, Ts...>(first + detail::numProperties<T>(), last);
}
/// @}

/// Returns the format string for the given number type.
template<typename T>
constexpr typename std::enable_if<std::is_floating_point<T>::value, const char *>::type formatStr()
{
  return "%g";
}

template<typename T>
constexpr typename std::enable_if<
    !std::is_floating_point<T>::value && std::is_signed<T>::value && sizeof(T) <= 4,
    const char *>::type
formatStr()
{
  return "%d";
}

template<typename T>
constexpr typename std::enable_if<
    !std::is_floating_point<T>::value && !std::is_signed<T>::value && sizeof(T) <= 4,
    const char *>::type
formatStr()
{
  return "%u";
}

template<typename T>
constexpr typename std::enable_if<
    !std::is_floating_point<T>::value && std::is_signed<T>::value && (sizeof(T) > 4),
    const char *>::type
formatStr()
{
  return "%ld";
}

template<typename T>
constexpr typename std::enable_if<
    !std::is_floating_point<T>::value && !std::is_signed<T>::value && (sizeof(T) > 4),
    const char *>::type
formatStr()
{
  return "%lu";
}
/// @}

}}

#endif
