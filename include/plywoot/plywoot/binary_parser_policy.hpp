#ifndef PLYWOOT_BINARY_PARSER_POLICY_HPP
#define PLYWOOT_BINARY_PARSER_POLICY_HPP

#include "buffered_istream.hpp"
#include "endian.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace plywoot { namespace detail {

/// Defines a parser policy that deals with binary input streams.
template<typename Endianness>
class BinaryParserPolicy
{
public:
  /// Constructs a binary little endian parser policy.
  BinaryParserPolicy(std::istream &is, std::vector<PlyElement> elements)
      : is_{is}, elements_{std::move(elements)}
  {
  }

  /// Seeks to the start of the data for the given element. Returns whether
  /// seeking was successful.
  // TODO(ton): skipping elements is inefficient; find a better solution.
  bool seekTo(const PlyElement &element) const
  {
    std::size_t numBytes{0};
    auto first = elements_.begin();
    const auto last = elements_.end();
    while (first != last && *first != element) { numBytes += elementSizeInBytes(*first++); }

    if (first != last && *first == element)
    {
      is_.seekTo(numBytes);
    }

    return first != last;
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

  /// Reads `n` numbers of the given type `T` from the input stream.
  /// @{
  template<typename T, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, LittleEndian>::value, std::uint8_t *>::type
  readNumbers(std::uint8_t *dest, std::size_t n) const
  {
    return is_.read<T>(dest, n);
  }

  template<typename T, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, BigEndian>::value, std::uint8_t *>::type
  readNumbers(std::uint8_t *dest, std::size_t n) const
  {
    is_.read<T>(dest, n);
    for (std::size_t i = 0; i < n; ++i, dest += sizeof(T))
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
  void skipProperties(std::size_t n) const
  {
    is_.skip(n);
  }

private:
  /// Calculates and returns the size in bytes of the given PLY element. Uses
  /// memoization; the size of every unique element is only calculated once.
  std::size_t elementSizeInBytes(const PlyElement &element) const
  {
    auto it = elementSize_.lower_bound(element.name());
    if (it == elementSize_.end() || it->first != element.name())
    {
      std::size_t numBytes{0};
      for (const PlyProperty &p : element.properties())
      {
        if (!p.isList()) { numBytes += element.size() * sizeOf(p.type()); }
        else
        {
          is_.seekToBegin();
          is_.skip(numBytes);

          std::size_t sizeSum = 0;
          for (std::size_t i = 0; i < element.size(); ++i)
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

            sizeSum += size;
            is_.skip(size * sizeOf(p.type()));
          }

          numBytes += element.size() * sizeOf(p.sizeType()) + sizeSum * sizeOf(p.type());
        }
      }
      it = elementSize_.emplace_hint(it, element.name(), numBytes);
    }
    return it->second;
  }

  mutable detail::BufferedIStream is_;
  std::vector<PlyElement> elements_;
  mutable std::map<std::string, std::ptrdiff_t> elementSize_;
};

using BinaryLittleEndianParserPolicy = BinaryParserPolicy<LittleEndian>;
using BinaryBigEndianParserPolicy = BinaryParserPolicy<BigEndian>;

}}

#endif
