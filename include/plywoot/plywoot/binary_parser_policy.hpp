#ifndef PLYWOOT_BINARY_PARSER_POLICY_HPP
#define PLYWOOT_BINARY_PARSER_POLICY_HPP

#include "buffered_istream.hpp"
#include "endian.hpp"

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

  /// Reads `N` numbers of the given type `T` from the input stream.
  /// @{
  template<typename T, std::size_t N, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, LittleEndian>::value, std::uint8_t *>::type
  readNumbers(std::uint8_t *dest) const
  {
    return is_.read<T, N>(dest);
  }

  template<typename T, std::size_t N, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, BigEndian>::value, std::uint8_t *>::type
  readNumbers(std::uint8_t *dest) const
  {
    is_.read<T, N>(dest);
    for (std::size_t i = 0; i < N; ++i, dest += sizeof(T))
    {
      T &t = *reinterpret_cast<T *>(dest);
      t = betoh(t);
    }
    return dest;
  }
  /// @}

  /// Skips a number of the given type `T` in the input stream.
  template<typename T>
  void skipNumber() const
  {
    is_.skip(sizeof(T));
  }

  /// Skips property data, totaling `n` bytes.
  void skipProperties(std::size_t n) const { is_.skip(n); }

private:
  mutable detail::BufferedIStream is_;
};

using BinaryLittleEndianParserPolicy = BinaryParserPolicy<LittleEndian>;
using BinaryBigEndianParserPolicy = BinaryParserPolicy<BigEndian>;

}}

#endif
