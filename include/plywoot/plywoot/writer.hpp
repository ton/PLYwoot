#ifndef PLYWOOT_WRITER_HPP
#define PLYWOOT_WRITER_HPP

#include "reflect.hpp"
#include "std.hpp"
#include "types.hpp"

#include <cstdint>
#include <type_traits>

namespace plywoot { namespace detail {

/// Represents a generic PLY writer that is parameterized with format specific
/// functionality through the FormatWriterPolicy type, which should adhere to
/// the following model requirements:
///
///   - void close();
///
///   - template<typename T> void writeNumber(T t);
///   - template<typename PlySizeT, typename PlyT, typename SrcT>
///     void writeList(const SrcT *t, std::size_t n) const;
///   - template<typename PlyT, typename SrcT>
///     void writeNumbers(const SrcT *t, std::size_t n) const;
///
///   - void writeNewline();
///   - void writeTokenSeparator();
template<typename FormatWriterPolicy>
class Writer : private FormatWriterPolicy
{
public:
  using FormatWriterPolicy::FormatWriterPolicy;

  void close() { FormatWriterPolicy::close(); }

  template<typename... Ts>
  const std::uint8_t *writeElement(
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    src = this->writeProperties<FormatWriterPolicy, Ts...>(src, first, last);

    // In case the element defines more properties than the source data,
    // append the missing properties with a default value of zero.
    if (reflect::numProperties<Ts...>() < static_cast<std::size_t>(std::distance(first, last)))
    {
      this->writeMissingProperties(first + reflect::numProperties<Ts...>(), last);
    }

    this->writeNewline();

    return src;
  }

private:
  template<typename PlyT, typename SrcT>
  typename std::enable_if<std::is_arithmetic<SrcT>::value, const std::uint8_t *>::type writeProperty(
      const std::uint8_t *src,
      reflect::Type<SrcT>) const
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    this->writeNumber(static_cast<PlyT>(*reinterpret_cast<const SrcT *>(src)));
    return src + sizeof(SrcT);
  }

  template<typename PlyT, typename SrcT>
  typename std::enable_if<!std::is_arithmetic<SrcT>::value, const std::uint8_t *>::type writeProperty(
      const std::uint8_t *src,
      reflect::Type<SrcT>) const
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT))) + sizeof(SrcT);
  }

  /// Specialization for the meta property type `Pack<T, N>`, this will write N
  /// numbers of type `T` to the output stream.
  template<typename PlyT, typename SrcT, std::size_t N>
  const std::uint8_t *writeProperty(const std::uint8_t *src, reflect::Type<reflect::Pack<SrcT, N>>) const
  {
    static_assert(N > 0, "invalid pack size specified (needs to be larger than zero)");
    static_assert(
        std::is_arithmetic<SrcT>::value, "it is not (yet) possible to write packs of non-numeric types");

    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    this->template writeNumbers<PlyT, SrcT>(reinterpret_cast<const SrcT *>(src), N);
    return src + N * sizeof(SrcT);
  }

  /// Specialization for the meta property type `Stride<T>`, this will just skip
  /// over a member variable of type `T` in the source buffer.
  template<typename SrcT>
  const std::uint8_t *writeProperty(const std::uint8_t *src, reflect::Type<reflect::Stride<SrcT>>) const
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT))) + sizeof(SrcT);
  }

  /// Specialization for the meta property type `Array<T, N, SizeT>`, this will
  /// write a list of N properties of type T.
  template<typename PlyT, typename PlySizeT, typename SrcT, std::size_t N>
  const std::uint8_t *writeListProperty(const std::uint8_t *src, reflect::Type<reflect::Array<SrcT, N>>) const
  {
    static_assert(N > 0, "invalid array size specified (needs to be larger than zero)");

    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    this->template writeList<PlySizeT, PlyT, SrcT>(reinterpret_cast<const SrcT *>(src), N);
    return src + N * sizeof(SrcT);
  }

  /// Specialization for a vector of type `T`.
  template<typename PlyT, typename PlySizeT, typename SrcT>
  const std::uint8_t *writeListProperty(const std::uint8_t *src, reflect::Type<std::vector<SrcT>>) const
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(std::vector<SrcT>)));
    const std::vector<SrcT> &v = *reinterpret_cast<const std::vector<SrcT> *>(src);
    this->template writeList<PlySizeT, PlyT, SrcT>(v.data(), v.size());
    return src + sizeof(std::vector<SrcT>);
  }

  template<typename PlyT, typename TypeTag>
  const std::uint8_t *writeListProperty(const std::uint8_t *src, const PlyProperty &property, TypeTag tag)
      const
  {
    switch (property.sizeType())
    {
      case PlyDataType::Char:
        return writeListProperty<PlyT, char>(src, tag);
      case PlyDataType::UChar:
        return writeListProperty<PlyT, unsigned char>(src, tag);
      case PlyDataType::Short:
        return writeListProperty<PlyT, short>(src, tag);
      case PlyDataType::UShort:
        return writeListProperty<PlyT, unsigned short>(src, tag);
      case PlyDataType::Int:
        return writeListProperty<PlyT, int>(src, tag);
      case PlyDataType::UInt:
        return writeListProperty<PlyT, unsigned int>(src, tag);
      case PlyDataType::Float:
        return writeListProperty<PlyT, float>(src, tag);
      case PlyDataType::Double:
        return writeListProperty<PlyT, double>(src, tag);
    }

    return src;
  }

  template<typename TypeTag>
  typename std::enable_if<TypeTag::isList, const std::uint8_t *>::type writeProperty(
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return writeListProperty<char>(src, property, tag);
      case PlyDataType::UChar:
        return writeListProperty<unsigned char>(src, property, tag);
      case PlyDataType::Short:
        return writeListProperty<short>(src, property, tag);
      case PlyDataType::UShort:
        return writeListProperty<unsigned short>(src, property, tag);
      case PlyDataType::Int:
        return writeListProperty<int>(src, property, tag);
      case PlyDataType::UInt:
        return writeListProperty<unsigned int>(src, property, tag);
      case PlyDataType::Float:
        return writeListProperty<float>(src, property, tag);
      case PlyDataType::Double:
        return writeListProperty<double>(src, property, tag);
    }

    return src;
  }

  template<typename TypeTag>
  typename std::enable_if<!TypeTag::isList, const std::uint8_t *>::type writeProperty(
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return writeProperty<char>(src, tag);
      case PlyDataType::UChar:
        return writeProperty<unsigned char>(src, tag);
      case PlyDataType::Short:
        return writeProperty<short>(src, tag);
      case PlyDataType::UShort:
        return writeProperty<unsigned short>(src, tag);
      case PlyDataType::Int:
        return writeProperty<int>(src, tag);
      case PlyDataType::UInt:
        return writeProperty<unsigned int>(src, tag);
      case PlyDataType::Float:
        return writeProperty<float>(src, tag);
      case PlyDataType::Double:
        return writeProperty<double>(src, tag);
    }

    return src;
  }

  /// Writes a single object of type `SrcT` read from the input buffer `src` to
  /// the given output stream `os` in case a corresponding element property is
  /// defined for it (`first` < `last`). Otherwise, it will skip over the object
  /// in the input buffer.
  template<typename SrcT>
  const std::uint8_t *writeProperty(
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return first < last ? writeProperty(src, *first, reflect::Type<SrcT>{})
                        : writeProperty(src, reflect::Type<reflect::Stride<SrcT>>{});
  }

  /// Reads a list of objects of type `T`, `U`, and `Ts...` from the input
  /// buffer `src` and writes them to the given output stream `os`. The objects
  /// should have a corresponding property defined in the input buffer, the list
  /// of properties is defined by the property range (`first`, `last`].
  /// @{
  template<typename Policy, typename T>
  const std::uint8_t *writeProperties(
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return writeProperty<T>(src, first, last);
  }

  template<typename Policy, typename T, typename U, typename... Ts>
  typename std::enable_if<std::is_same<Policy, AsciiWriterPolicy>::value, const std::uint8_t *>::type
  writeProperties(const std::uint8_t *src, PropertyConstIterator first, PropertyConstIterator last) const
  {
    src = writeProperty<T>(src, first, last);
    first += reflect::numProperties<T>();
    if (first < last) { this->writeTokenSeparator(); }
    return writeProperties<Policy, U, Ts...>(src, first, last);
  }

  template<typename Policy, typename T, typename U, typename... Ts>
  typename std::enable_if<!std::is_same<Policy, AsciiWriterPolicy>::value, const std::uint8_t *>::type
  writeProperties(const std::uint8_t *src, PropertyConstIterator first, PropertyConstIterator last) const
  {
    return writeProperties<Policy, U, Ts...>(
        writeProperty<T>(src, first, last), first + reflect::numProperties<T>(), last);
  }
};

}}

#endif
