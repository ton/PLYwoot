#ifndef PLYWOOT_TYPE_TRAITS_HPP
#define PLYWOOT_TYPE_TRAITS_HPP

#include "reflect.hpp"
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
/// pack.
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

// TODO(ton): add convenience function sizeOf().

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
  return ((offset + alignof(T)) % alignof(T)) == 0 && isPacked<U, Ts...>(offset + SizeOf<T>::size);
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
