#ifndef PLYWOOT_BINARY_PARSER_POLICY_HPP
#define PLYWOOT_BINARY_PARSER_POLICY_HPP

#include "buffered_istream.hpp"
#include "endian.hpp"
#include "type_traits.hpp"

#include <cstdint>
#include <numeric>

namespace plywoot { namespace detail {

/// Defines a parser policy that deals with binary input streams.
template<typename Endianness>
class BinaryParserPolicy
{
public:
  /// Constructs a binary little endian parser policy.
  BinaryParserPolicy(std::istream &is) : is_{is} {}

  /// Skips the given element in the current input stream, assuming the read
  /// head is at the start of that element.
  void skipElement(const PlyElement &e) const
  {
    // In case all element properties are non-list properties, we can calculate
    // the element size in one go.
    const std::vector<PlyProperty> &properties = e.properties();
    if (std::all_of(properties.begin(), properties.end(), [](const PlyProperty &p) { return !p.isList(); }))
    {
      is_.skip(
          e.size() * std::accumulate(
                         properties.begin(), properties.end(), 0,
                         [](std::size_t acc, const PlyProperty &p) { return acc + sizeOf(p.type()); }));
    }
    else
    {
      for (std::size_t i = 0; i < e.size(); ++i)
      {
        for (const PlyProperty &p : e.properties())
        {
          if (!p.isList()) { is_.skip(sizeOf(p.type())); }
          else
          {
            std::size_t size = 0;
            switch (p.sizeType())
            {
              case PlyDataType::Char:
                size = readNumber<char>();
                break;
              case PlyDataType::UChar:
                size = readNumber<unsigned char>();
                break;
              case PlyDataType::Short:
                size = readNumber<short>();
                break;
              case PlyDataType::UShort:
                size = readNumber<unsigned short>();
                break;
              case PlyDataType::Int:
                size = readNumber<int>();
                break;
              case PlyDataType::UInt:
                size = readNumber<unsigned int>();
                break;
              case PlyDataType::Float:
                size = readNumber<float>();
                break;
              case PlyDataType::Double:
                size = readNumber<double>();
                break;
            }

            is_.skip(size * sizeOf(p.type()));
          }
        }
      }
    }
  }

  /// Reads a number of the given type `T` from the input stream.
  /// @{
  template<typename T, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, LittleEndian>::value, T>::type readNumber() const
  {
    // TODO(ton): this assumes the target architecture is little endian.
    return is_.read<T>();
  }

  template<typename T, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, BigEndian>::value, T>::type readNumber() const
  {
    return betoh(is_.read<T>());
  }
  /// @}

  /// Reads `N` numbers of the given type `PlyT` from the input stream, and
  /// stores them contiguously at the given destination in memory as numbers of
  /// type `DestT`. Returns a pointer pointing just after the last number stored
  /// at `dest`.
  template<typename PlyT, typename DestT, std::size_t N>
  std::uint8_t *readNumbers(std::uint8_t *dest) const
  {
    return is_.read<PlyT, DestT, N, Endianness>(dest);
  }

  /// Skips a number of the given type `T` in the input stream.
  template<typename T>
  void skipNumber() const
  {
    is_.skip(sizeof(T));
  }

  /// Skips property data, totaling `n` bytes.
  void skipProperties(std::size_t n) const { is_.skip(n); }

  /// Copies all element data to the given destination buffer `dest`. This
  /// assumes an element maps to a collection of types `Ts...` for which all
  /// types are trivially copyable, and contiguous in memory without any padding
  /// in between.
  /// @{
  template<typename... Ts, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, LittleEndian>::value, void>::type memcpy(
      std::uint8_t *dest,
      const PlyElement &element) const
  {
    is_.memcpy(dest, element.size() * detail::SizeOf<Ts...>::size);
  }

  template<typename... Ts, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, BigEndian>::value, void>::type memcpy(
      std::uint8_t *dest,
      const PlyElement &element) const
  {
    is_.memcpy(dest, element.size() * detail::SizeOf<Ts...>::size);

    for (size_t i = 0; i < element.size(); ++i) { dest = toBigEndian<Ts...>(dest); }
  }
  /// @}

private:
  template<typename T>
  typename std::enable_if<!detail::IsPack<T>::value, std::uint8_t *>::type toBigEndian(
      std::uint8_t *dest) const
  {
    auto ptr = reinterpret_cast<T *>(dest);
    *ptr = htobe(*ptr);
    return dest + sizeof(T);
  }

  template<typename T>
  typename std::enable_if<detail::IsPack<T>::value, std::uint8_t *>::type toBigEndian(
      std::uint8_t *dest) const
  {
    auto ptr = reinterpret_cast<typename T::DestT *>(dest);
    for (std::size_t i = 0; i < T::size; ++i, ++ptr) { *ptr = htobe(*ptr); }

    return dest + detail::SizeOf<T>::size;
  }

  template<typename T, typename U, typename... Ts>
  std::uint8_t *toBigEndian(std::uint8_t *dest) const
  {
    static_assert(
        detail::isPacked<T, U, Ts...>(),
        "converting possibly padded range of little-endian objects to big-endian not implemented yet");

    return toBigEndian<U, Ts...>(toBigEndian<T>(dest));
  }

  mutable detail::BufferedIStream is_;
};

using BinaryLittleEndianParserPolicy = BinaryParserPolicy<LittleEndian>;
using BinaryBigEndianParserPolicy = BinaryParserPolicy<BigEndian>;

}}

#endif
