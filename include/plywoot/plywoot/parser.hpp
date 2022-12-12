#ifndef PLYWOOT_PARSER_HPP
#define PLYWOOT_PARSER_HPP

#include "reflect.hpp"
#include "std.hpp"
#include "types.hpp"

#include <cstdint>
#include <numeric>

namespace plywoot { namespace detail {

/// Represents a generic PLY parser that is parameterized with format specific
/// functionality through the FormatParserPolicy type, which should adhere to the
/// following model requirements:
///
///   - bool seekTo(const PlyElement &element);
///
///   - template<typename T> T readNumber();
///   - template<typename T> std::uint8_t *readNumbers(std::uint8_t *dest, std::size_t n);
///   - template<typename T> T skipNumber();
///
///   - void skipProperties(size_t numBytes);
template<typename FormatParserPolicy>
class Parser : private FormatParserPolicy
{
public:
  using FormatParserPolicy::FormatParserPolicy;

  /// Reads the given element from the PLY input data stream, storing data in
  /// the given destination buffer `dest` using the types given as
  /// `PropertyType`'s in the template argument list. In case the number of
  /// properties for the element exceeds the number of properties in the
  /// template argument list, the remaining properties are directly stored
  /// using the property type as defined in the PLY header. This assumes that
  /// the output buffer can hold the required amount of data; failing to
  /// satisfy this precondition results in undefined behavior.
  // TODO(ton): probably better to add another parameter 'size' to guard
  // against overwriting the input buffer.
  template<typename... Ts>
  void read(const PlyElement &element, reflect::Layout<Ts...> layout) const
  {
    readElements<Ts...>(layout.data(), element);
  }

private:
  template<typename... Ts>
  void readElements(std::uint8_t *dest, const PlyElement &element) const
  {
    if (this->seekTo(element))
    {
      const PropertyConstIterator first = element.properties().begin();
      const PropertyConstIterator last = element.properties().end();
      const PropertyConstIterator firstToSkip = first + reflect::numProperties<Ts...>();

      const std::size_t numBytesToSkip =
          firstToSkip < last ? std::accumulate(
                                   firstToSkip, last, 0ul,
                                   [](std::size_t acc, const PlyProperty &p) {
                                     return acc + sizeOf(p.isList() ? p.sizeType() : p.type());
                                   })
                             : 0;

      for (std::size_t i{0}; i < element.size(); ++i)
      {
        dest = readElement<Ts...>(dest, first, last);
        this->skipProperties(numBytesToSkip);
      }
    }
  }

  template<typename T>
  std::uint8_t *readElement(std::uint8_t *dest, PlyPropertyConstIterator first, PlyPropertyConstIterator last)
      const
  {
    return first < last ? readProperty(dest, *first, reflect::Type<T>{})
                        : readProperty(dest, reflect::Type<reflect::Stride<T>>{});
  }

  template<typename T, typename U, typename... Ts>
  typename std::enable_if<!reflect::IsPack<T>::value, std::uint8_t *>::type readElement(
      std::uint8_t *dest,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return readElement<U, Ts...>(readElement<T>(dest, first, last), first + 1, last);
  }

  template<typename T, typename U, typename... Ts>
  typename std::enable_if<reflect::IsPack<T>::value, std::uint8_t *>::type readElement(
      std::uint8_t *dest,
      PropertyConstIterator first,
      PropertyConstIterator last) const
  {
    return readElement<U, Ts...>(readElement<T>(dest, first, last), first + T::size, last);
  }

  template<typename PlyT, typename PlySizeT, typename DestT>
  std::uint8_t *readListProperty(std::uint8_t *dest, reflect::Type<std::vector<DestT>>) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(std::vector<DestT>)));
    std::vector<DestT> &v = *reinterpret_cast<std::vector<DestT> *>(dest);

    const PlySizeT size = this->template readNumber<PlySizeT>();
    v.reserve(size);
    for (PlySizeT i = 0; i < size; ++i) { v.push_back(this->template readNumber<PlyT>()); }

    return dest + sizeof(std::vector<DestT>);
  }

  template<typename PlyT, typename PlySizeT, typename DestT, std::size_t N>
  typename std::enable_if<!std::is_same<PlyT, DestT>::value, std::uint8_t *>::type readListProperty(
      std::uint8_t *dest,
      reflect::Type<reflect::Array<DestT, N>>) const
  {
    // TODO(ton): skip the number that defines the list in the PLY data, we
    // expect it to be of length N; throw an exception here in case they do no match?
    this->template skipNumber<PlySizeT>();
    for (std::size_t i = 0; i < N; ++i) { dest = readProperty<PlyT>(dest, reflect::Type<DestT>{}); }
    return dest;
  }

  template<typename PlyT, typename PlySizeT, typename DestT, std::size_t N>
  typename std::enable_if<std::is_same<PlyT, DestT>::value, std::uint8_t *>::type readListProperty(
      std::uint8_t *dest,
      reflect::Type<reflect::Array<DestT, N>>) const
  {
    static_assert(std::is_arithmetic<PlyT>::value, "unexpected PLY data type");

    // TODO(ton): skip the number that defines the list in the PLY data, we
    // expect it to be of length N; throw an exception here in case they do no match?
    this->template skipNumber<PlySizeT>();
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT)));
    return this->template readNumbers<DestT>(dest, N);
  }

  template<typename PlyT, typename TypeTag>
  std::uint8_t *readListProperty(std::uint8_t *dest, const PlyProperty &property, TypeTag tag) const
  {
    switch (property.sizeType())
    {
      case PlyDataType::Char:
        return readListProperty<PlyT, char>(dest, tag);
      case PlyDataType::UChar:
        return readListProperty<PlyT, unsigned char>(dest, tag);
      case PlyDataType::Short:
        return readListProperty<PlyT, short>(dest, tag);
      case PlyDataType::UShort:
        return readListProperty<PlyT, unsigned short>(dest, tag);
      case PlyDataType::Int:
        return readListProperty<PlyT, int>(dest, tag);
      case PlyDataType::UInt:
        return readListProperty<PlyT, unsigned int>(dest, tag);
      case PlyDataType::Float:
        return readListProperty<PlyT, float>(dest, tag);
      case PlyDataType::Double:
        return readListProperty<PlyT, double>(dest, tag);
    }

    return dest;
  }

  template<typename DestT>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Stride<DestT>>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT))) + sizeof(DestT);
  }

  template<typename PlyT, typename DestT>
  typename std::enable_if<std::is_arithmetic<DestT>::value, std::uint8_t *>::type readProperty(
      std::uint8_t *dest,
      reflect::Type<DestT>) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT)));
    *reinterpret_cast<DestT *>(dest) = this->template readNumber<PlyT>();
    return dest + sizeof(DestT);
  }

  template<typename PlyT, typename DestT>
  typename std::enable_if<!std::is_arithmetic<DestT>::value, std::uint8_t *>::type readProperty(
      std::uint8_t *dest,
      reflect::Type<DestT>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT))) + sizeof(DestT);
  }

  template<typename PlyT, typename DestT, size_t N>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Pack<DestT, N>>) const
  {
    static_assert(std::is_arithmetic<PlyT>::value, "unexpected PLY data type");
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT)));
    return this->template readNumbers<DestT>(dest, N);
  }

  template<typename TypeTag>
  typename std::enable_if<TypeTag::isList, std::uint8_t *>::type readProperty(
      std::uint8_t *dest,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return readListProperty<char>(dest, property, tag);
      case PlyDataType::UChar:
        return readListProperty<unsigned char>(dest, property, tag);
      case PlyDataType::Short:
        return readListProperty<short>(dest, property, tag);
      case PlyDataType::UShort:
        return readListProperty<unsigned short>(dest, property, tag);
      case PlyDataType::Int:
        return readListProperty<int>(dest, property, tag);
      case PlyDataType::UInt:
        return readListProperty<unsigned int>(dest, property, tag);
      case PlyDataType::Float:
        return readListProperty<float>(dest, property, tag);
      case PlyDataType::Double:
        return readListProperty<double>(dest, property, tag);
    }

    return dest;
  }

  template<typename TypeTag>
  typename std::enable_if<!TypeTag::isList, std::uint8_t *>::type readProperty(
      std::uint8_t *dest,
      const PlyProperty &property,
      TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return readProperty<char>(dest, tag);
      case PlyDataType::UChar:
        return readProperty<unsigned char>(dest, tag);
      case PlyDataType::Short:
        return readProperty<short>(dest, tag);
      case PlyDataType::UShort:
        return readProperty<unsigned short>(dest, tag);
      case PlyDataType::Int:
        return readProperty<int>(dest, tag);
      case PlyDataType::UInt:
        return readProperty<unsigned int>(dest, tag);
      case PlyDataType::Float:
        return readProperty<float>(dest, tag);
      case PlyDataType::Double:
        return readProperty<double>(dest, tag);
    }

    return dest;
  }
};

}}

#endif
