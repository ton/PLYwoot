#ifndef PLYWOOT_ENDIAN_HPP
#define PLYWOOT_ENDIAN_HPP

#include <utility>

#include <endian.h>

namespace plywoot { namespace detail {

/// Type tag to indicate little endian behavior is required for the
/// parser/writer policies.
struct LittleEndian
{
};

/// Type tag to indicate big endian behavior is required for the parser/writer
/// policies.
struct BigEndian
{
};

/// Naive byte swap functions for floating point numbers.
/// @{
template<typename T>
typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 4, T>::type byte_swap(T t)
{
  unsigned char *bytes = reinterpret_cast<unsigned char*>(&t);
  std::swap(bytes[0], bytes[3]);
  std::swap(bytes[1], bytes[2]);
  return t;
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 8, T>::type byte_swap(T t)
{
  unsigned char *bytes = reinterpret_cast<unsigned char*>(&t);
  std::swap(bytes[0], bytes[7]);
  std::swap(bytes[1], bytes[6]);
  std::swap(bytes[2], bytes[5]);
  std::swap(bytes[3], bytes[4]);
  return t;
}
/// @}

/// Wrappers around the glibc htobe* and be*toh functions that pick the correct
/// call depending on the size of the argument type.
/// @{
template<typename T>
typename std::enable_if<sizeof(T) == 1, T>::type htobe(T t)
{
  return t;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 2, T>::type htobe(T t)
{
  return static_cast<T>(htobe16(static_cast<std::uint16_t>(t)));
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 4, T>::type htobe(T t)
{
  return static_cast<T>(htobe32(static_cast<std::uint32_t>(t)));
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 8, T>::type htobe(T t)
{
  return static_cast<T>(htobe64(static_cast<std::uint64_t>(t)));
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 4, T>::type htobe(T t)
{
  return byte_swap(t);
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 8, T>::type htobe(T t)
{
  return byte_swap(t);
}

template<typename T>
typename std::enable_if<sizeof(T) == 1, T>::type betoh(T t)
{
  return t;
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 2, T>::type betoh(T t)
{
  return static_cast<T>(be16toh(static_cast<std::uint16_t>(t)));
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 4, T>::type betoh(T t)
{
  return static_cast<T>(be32toh(static_cast<std::uint32_t>(t)));
}

template<typename T>
typename std::enable_if<std::is_integral<T>::value && sizeof(T) == 8, T>::type betoh(T t)
{
  return static_cast<T>(be64toh(static_cast<std::uint64_t>(t)));
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 4, T>::type betoh(T t)
{
  return byte_swap(t);
}

template<typename T>
typename std::enable_if<std::is_floating_point<T>::value && sizeof(T) == 8, T>::type betoh(T t)
{
  return byte_swap(t);
}
/// @}

}}

#endif
