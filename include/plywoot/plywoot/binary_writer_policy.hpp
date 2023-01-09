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

  /// Writes a list of numbers of type `PlyT` to the given binary output stream,
  /// either in little or big endian format, reading `n` numbers starting at
  /// address `src`, of number type `SrcT`. This also writes the size of the
  /// list to the output stream using number type `PlySizeT`.
  template<typename PlySizeT, typename PlyT, typename SrcT, typename EndiannessDependent = Endianness>
  void writeList(std::ostream &os, const SrcT *t, std::size_t n) const
  {
    writeNumber<PlySizeT>(os, n);
    writeNumbers<PlyT, SrcT>(os, t, n);
  }
  /// @}

  /// Writes `n` numbers of type `SrcT` to the given binary output stream, as
  /// numbers of type `PlyT`, either in little or big endian format.
  /// @{

  // Little endian; source number type is equal to the destination number type.
  template<typename PlyT, typename SrcT, typename EndiannessDependent = Endianness>
  typename std::enable_if<
      std::is_same<PlyT, SrcT>::value && std::is_same<EndiannessDependent, LittleEndian>::value,
      void>::type
  writeNumbers(std::ostream &os, const SrcT *t, std::size_t n) const
  {
    os.write(reinterpret_cast<const char *>(t), sizeof(SrcT) * n);
  }

  // Other cases (not optimized); that is, source number type is not equal to
  // the destination number type, or we have to write big endian numbers.
  template<typename PlyT, typename SrcT, typename EndiannessDependent = Endianness>
  typename std::enable_if<
      !std::is_same<PlyT, SrcT>::value || !std::is_same<EndiannessDependent, LittleEndian>::value,
      void>::type
  writeNumbers(std::ostream &os, const SrcT *t, std::size_t n) const
  {
    for (std::size_t i = 0; i < n; ++i) { writeNumber<PlyT>(os, *t++); }
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
};

using BinaryLittleEndianWriterPolicy = BinaryWriterPolicy<LittleEndian>;
using BinaryBigEndianWriterPolicy = BinaryWriterPolicy<BigEndian>;

}}

#endif
