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
///   - template<typename T> void writeNumber(std::ostream &os, T t);
///   - template<typename PlySizeT, typename PlyT, typename SrcT>
///     void writeList(std::ostream &os, const SrcT *t, std::size_t n) const;
///   - template<typename PlyT, typename SrcT>
///     void writeNumbers(std::ostream &os, const SrcT *t, std::size_t n) const;
///
///   - void writeNewline(std::ostream &os);
template<typename FormatWriterPolicy>
class Writer : private FormatWriterPolicy
{
public:
  using FormatWriterPolicy::FormatWriterPolicy;

  template<typename... Ts>
  void write(std::ostream &os, const PlyElement &element, const std::uint8_t *src, std::size_t n) const
  {
    const auto first = element.properties().begin();
    const auto last = element.properties().end();

    for (std::size_t i{0}; i < n; ++i)
    {
      src = writeProperties<FormatWriterPolicy, Ts...>(os, src, first, last);

      // In case the element defines more properties than the source data,
      // append the missing properties with a default value of zero.
      if (reflect::numProperties<Ts...>() < static_cast<std::size_t>(std::distance(first, last)))
      {
        this->writeMissingProperties(os, first + sizeof...(Ts), last);
      }

      this->writeNewline(os);
    }
  }

private:
  template<typename PlyT, typename SrcT>
  typename std::enable_if<std::is_arithmetic<SrcT>::value, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<SrcT>) const
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    this->writeNumber(os, static_cast<PlyT>(*reinterpret_cast<const SrcT *>(src)));
    return src + sizeof(SrcT);
  }

  template<typename PlyT, typename SrcT>
  typename std::enable_if<!std::is_arithmetic<SrcT>::value, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<SrcT>) const
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT))) + sizeof(SrcT);
  }

  /// Specialization for the meta property type `Pack<T, N>`, this will write N
  /// numbers of type `T` to the output stream.
  template<typename PlyT, typename SrcT, std::size_t N>
  const std::uint8_t *writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<reflect::Pack<SrcT, N>>) const
  {
    static_assert(N > 0, "invalid pack size specified (needs to be larger than zero)");
    static_assert(std::is_arithmetic<SrcT>::value, "it is not (yet) possible to write packs of non-numeric types");

    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    this->template writeNumbers<PlyT, SrcT>(os, reinterpret_cast<const SrcT *>(src), N);
    return src + N * sizeof(SrcT);
  }

  /// Specialization for the meta property type `Stride<T>`, this will just skip
  /// over a member variable of type `T` in the source buffer.
  template<typename SrcT>
  const std::uint8_t *writeProperty(
      std::ostream &,
      const std::uint8_t *src,
      reflect::Type<reflect::Stride<SrcT>>) const
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT))) + sizeof(SrcT);
  }

  /// Specialization for the meta property type `Array<T, N, SizeT>`, this will
  /// write a list of N properties of type T.
  template<typename PlyT, typename PlySizeT, typename SrcT, std::size_t N>
  const std::uint8_t *writeListProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<reflect::Array<SrcT, N>>) const
  {
    static_assert(N > 0, "invalid array size specified (needs to be larger than zero)");

    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    this->template writeList<PlySizeT, PlyT, SrcT>(os, reinterpret_cast<const SrcT *>(src), N);
    return src + N * sizeof(SrcT);
  }

  /// Specialization for a vector of type `T`.
  template<typename PlyT, typename PlySizeT, typename SrcT>
  const std::uint8_t *writeListProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<std::vector<SrcT>>) const
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(std::vector<SrcT>)));
    const std::vector<SrcT> &v = *reinterpret_cast<const std::vector<SrcT> *>(src);
    this->template writeList<PlySizeT, PlyT, SrcT>(os, v.data(), v.size());
    return src + sizeof(std::vector<SrcT>);
  }

  template<typename PlyT, typename TypeTag>
  const std::uint8_t *writeListProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.sizeType())
    {
      case PlyDataType::Char:
        return writeListProperty<PlyT, char>(os, src, tag);
      case PlyDataType::UChar:
        return writeListProperty<PlyT, unsigned char>(os, src, tag);
      case PlyDataType::Short:
        return writeListProperty<PlyT, short>(os, src, tag);
      case PlyDataType::UShort:
        return writeListProperty<PlyT, unsigned short>(os, src, tag);
      case PlyDataType::Int:
        return writeListProperty<PlyT, int>(os, src, tag);
      case PlyDataType::UInt:
        return writeListProperty<PlyT, unsigned int>(os, src, tag);
      case PlyDataType::Float:
        return writeListProperty<PlyT, float>(os, src, tag);
      case PlyDataType::Double:
        return writeListProperty<PlyT, double>(os, src, tag);
    }

    return src;
  }

  template<typename TypeTag>
  typename std::enable_if<TypeTag::isList, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return writeListProperty<char>(os, src, property, tag);
      case PlyDataType::UChar:
        return writeListProperty<unsigned char>(os, src, property, tag);
      case PlyDataType::Short:
        return writeListProperty<short>(os, src, property, tag);
      case PlyDataType::UShort:
        return writeListProperty<unsigned short>(os, src, property, tag);
      case PlyDataType::Int:
        return writeListProperty<int>(os, src, property, tag);
      case PlyDataType::UInt:
        return writeListProperty<unsigned int>(os, src, property, tag);
      case PlyDataType::Float:
        return writeListProperty<float>(os, src, property, tag);
      case PlyDataType::Double:
        return writeListProperty<double>(os, src, property, tag);
    }

    return src;
  }

  template<typename TypeTag>
  typename std::enable_if<!TypeTag::isList, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return writeProperty<char>(os, src, tag);
      case PlyDataType::UChar:
        return writeProperty<unsigned char>(os, src, tag);
      case PlyDataType::Short:
        return writeProperty<short>(os, src, tag);
      case PlyDataType::UShort:
        return writeProperty<unsigned short>(os, src, tag);
      case PlyDataType::Int:
        return writeProperty<int>(os, src, tag);
      case PlyDataType::UInt:
        return writeProperty<unsigned int>(os, src, tag);
      case PlyDataType::Float:
        return writeProperty<float>(os, src, tag);
      case PlyDataType::Double:
        return writeProperty<double>(os, src, tag);
    }

    return src;
  }

  /// Bottom case of the function recursively defined on a list of input property
  /// types to write, this is simply a no-op.
  const std::uint8_t *writeElement(
      std::ostream &,
      const std::uint8_t *src,
      PropertyConstIterator,
      PropertyConstIterator) const
  {
    return src;
  }

  /// Writes a single object of type `SrcT` read from the input buffer `src` to
  /// the given output stream `os` in case a corresponding element property is
  /// defined for it (`first` < `last`). Otherwise, it will skip over the object
  /// in the input buffer.
  template<typename SrcT>
  const std::uint8_t *writeElement(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return first < last ? writeProperty(os, src, *first, reflect::Type<SrcT>{})
                        : writeProperty(os, src, reflect::Type<reflect::Stride<SrcT>>{});
  }

  /// Reads a list of objects of type `T`, `U`, and `Ts...` from the input
  /// buffer `src` and writes them to the given output stream `os`. The objects
  /// should have a corresponding property defined in the input buffer, the list
  /// of properties is defined by the property range (`first`, `last`].
  /// @{
  template<typename Policy>
  const std::uint8_t *writeProperties(
      std::ostream &,
      const std::uint8_t *src,
      PropertyConstIterator,
      PropertyConstIterator) const
  {
    return src;
  }

  template<typename Policy, typename T>
  const std::uint8_t *writeProperties(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return writeElement<T>(os, src, first, last);
  }

  template<typename Policy, typename T, typename U, typename... Ts>
  typename std::enable_if<
      std::is_same<Policy, AsciiWriterPolicy>::value && !reflect::IsPack<T>::value,
      const std::uint8_t *>::type
  writeProperties(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    src = writeElement<T>(os, src, first++, last);
    if (first < last) os.put(' ');
    return writeProperties<Policy, U, Ts...>(os, src, first, last);
  }

  template<typename Policy, typename T, typename U, typename... Ts>
  typename std::enable_if<
      std::is_same<Policy, AsciiWriterPolicy>::value && reflect::IsPack<T>::value,
      const std::uint8_t *>::type
  writeProperties(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    src = writeElement<T>(os, src, first, last);
    first += T::size;
    if (first < last) os.put(' ');
    return writeProperties<Policy, U, Ts...>(os, src, first, last);
  }

  template<typename Policy, typename T, typename U, typename... Ts>
  typename std::enable_if<
      !std::is_same<Policy, AsciiWriterPolicy>::value && !reflect::IsPack<T>::value,
      const std::uint8_t *>::type
  writeProperties(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return writeProperties<Policy, U, Ts...>(os, writeElement<T>(os, src, first, last), first + 1, last);
  }

  template<typename Policy, typename T, typename U, typename... Ts>
  typename std::enable_if<
      !std::is_same<Policy, AsciiWriterPolicy>::value && reflect::IsPack<T>::value,
      const std::uint8_t *>::type
  writeProperties(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return writeProperties<Policy, U, Ts...>(
        os, writeElement<T>(os, src, first, last), first + T::size, last);
  }
  /// @}
};

}}

#endif
