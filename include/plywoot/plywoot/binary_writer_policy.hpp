#ifndef PLYWOOT_BINARY_WRITER_POLICY_HPP
#define PLYWOOT_BINARY_WRITER_POLICY_HPP

#include "endian.hpp"

#include <ostream>

namespace plywoot { namespace detail {

/// Defines a writer policy that deals with binary output streams.
template<typename Endianness>
class BinaryWriterPolicy
{
public:
  /// Writes the number `t` of the given type `T` to the given binary output
  /// stream in little endian format.
  /// @{
  template<typename T, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, LittleEndian>::value, void>::type writeNumber(
      std::ostream &os,
      T t) const
  {
    // TODO(ton): this assumes the target architecture is little endian.
    os.write(reinterpret_cast<const char *>(&t), sizeof(T));
  }

  template<typename T, typename EndiannessDependent = Endianness>
  typename std::enable_if<std::is_same<EndiannessDependent, BigEndian>::value, void>::type writeNumber(
      std::ostream &os,
      T t) const
  {
    const auto be = htobe(t);
    os.write(reinterpret_cast<const char *>(&be), sizeof(T));
  }
  /// @}

  /// Outputs empty data for the range of properties [`first`, `last`). Note
  /// that a property that is undefined is always stored as a zero number, where
  /// the type of the number depends on the underlying property; in case of a
  /// list property the size type determines the number type, otherwise the
  /// regular property type is used.
  void writeMissingProperties(std::ostream &os, PlyPropertyConstIterator first, PlyPropertyConstIterator last)
      const
  {
    while (first != last)
    {
      switch (first->isList() ? first->sizeType() : first->type())
      {
        case PlyDataType::Char:
          writeNumber<char>(os, 0);
          break;
        case PlyDataType::UChar:
          writeNumber<unsigned char>(os, 0);
          break;
        case PlyDataType::Short:
          writeNumber<short>(os, 0);
          break;
        case PlyDataType::UShort:
          writeNumber<unsigned short>(os, 0);
          break;
        case PlyDataType::Int:
          writeNumber<int>(os, 0);
          break;
        case PlyDataType::UInt:
          writeNumber<unsigned int>(os, 0);
          break;
        case PlyDataType::Float:
          writeNumber<float>(os, 0);
          break;
        case PlyDataType::Double:
          writeNumber<double>(os, 0);
          break;
      }
      ++first;
    }
  }

  /// Writes a newline, ignored for binary output formats.
  void writeNewline(std::ostream &) const {}

  /// Writes a token separator, ignored for binary output formats.
  void writeTokenSeparator(std::ostream &) const {}
};

using BinaryLittleEndianWriterPolicy = BinaryWriterPolicy<LittleEndian>;
using BinaryBigEndianWriterPolicy = BinaryWriterPolicy<BigEndian>;

}}

#endif
