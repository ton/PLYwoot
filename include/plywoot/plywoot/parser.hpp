/*
   This file is part of PLYwoot, a header-only PLY parser.

   Copyright (C) 2023-2024, Ton van den Heuvel

   PLYwoot is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef PLYWOOT_PARSER_HPP
#define PLYWOOT_PARSER_HPP

#include "reflect.hpp"
#include "std.hpp"
#include "type_traits.hpp"
#include "types.hpp"

#include <cstdint>
#include <numeric>

namespace plywoot { namespace detail {

/// Represents a generic PLY parser that is parameterized with format specific
/// functionality through the FormatParserPolicy type, which should adhere to the
/// following model requirements:
///
///   - template<typename T> T readNumber();
///   - template<typename PlyT, typename DestT, std::size_t N> std::uint8_t *readNumbers(std::uint8_t *dest);
///   - template<typename T> T skipNumber();
///
///   - void skipElement(const PlyElement &e);
///   - void skipProperty(const PlyProperty &p);
///   - void skipProperties(size_t numBytes);
///
///  Only for the binary policies, the following function needs to be
///  implemented as well:
///
///   - template<typename ...Ts> void memcpy(std::uint8_t *dest, const PLyElement &e);
///
template<typename FormatParserPolicy>
class Parser : private FormatParserPolicy
{
private:
  template<typename... Ts>
  struct MaybeMemcpyable
  {
    static constexpr bool value = !std::is_same<FormatParserPolicy, AsciiParserPolicy>::value &&
                                  detail::isPacked<Ts...>() && detail::isTriviallyCopyable<Ts...>();
  };

public:
  using FormatParserPolicy::FormatParserPolicy;

  /// Reads the given element from the PLY input data stream, returning all data
  /// as a memory block wrapped by an instance of `PlyElementData`. For
  /// representation of the data in the memory block, see `PlyElementData` for
  /// more details.
  ///
  /// Note: this is not optimized for performance in any way, but is meant to
  /// provide a way to read all data from a PLY file without the need for up
  /// front knowledge about the data a PLY file may contain. For now, it is only
  /// used by `plywoot::convert()` to be able to convert PLY data between
  /// different formats.
  PlyElementData read(const PlyElement &element) const
  {
    PlyElementData result(element);

    std::uint8_t *dest = result.data();
    for (size_t i = 0; i < element.size(); ++i)
    {
      for (const PlyProperty &property : element.properties())
      {
        // In case of a list property, allocate a vector of the right type, and
        // read the variable length list.
        if (property.isList())
        {
          switch (property.type())
          {
            case PlyDataType::Char:
              dest = readProperty(dest, property, reflect::Type<std::vector<char>>{});
              break;
            case PlyDataType::UChar:
              dest = readProperty(dest, property, reflect::Type<std::vector<unsigned char>>{});
              break;
            case PlyDataType::Short:
              dest = readProperty(dest, property, reflect::Type<std::vector<short>>{});
              break;
            case PlyDataType::UShort:
              dest = readProperty(dest, property, reflect::Type<std::vector<unsigned short>>{});
              break;
            case PlyDataType::Int:
              dest = readProperty(dest, property, reflect::Type<std::vector<int>>{});
              break;
            case PlyDataType::UInt:
              dest = readProperty(dest, property, reflect::Type<std::vector<unsigned int>>{});
              break;
            case PlyDataType::Float:
              dest = readProperty(dest, property, reflect::Type<std::vector<float>>{});
              break;
            case PlyDataType::Double:
              dest = readProperty(dest, property, reflect::Type<std::vector<double>>{});
              break;
          }
        }
        else
        {
          switch (property.type())
          {
            case PlyDataType::Char:
              dest = readProperty(dest, property, reflect::Type<char>{});
              break;
            case PlyDataType::UChar:
              dest = readProperty(dest, property, reflect::Type<unsigned char>{});
              break;
            case PlyDataType::Short:
              dest = readProperty(dest, property, reflect::Type<short>{});
              break;
            case PlyDataType::UShort:
              dest = readProperty(dest, property, reflect::Type<unsigned short>{});
              break;
            case PlyDataType::Int:
              dest = readProperty(dest, property, reflect::Type<int>{});
              break;
            case PlyDataType::UInt:
              dest = readProperty(dest, property, reflect::Type<unsigned int>{});
              break;
            case PlyDataType::Float:
              dest = readProperty(dest, property, reflect::Type<float>{});
              break;
            case PlyDataType::Double:
              dest = readProperty(dest, property, reflect::Type<double>{});
              break;
          }
        }
      }

      dest = detail::align(dest, result.alignment());
    }

    return result;
  }

  /// Reads the given element from the PLY input data stream, storing data in
  /// the given destination buffer associated with the layout descriptor using
  /// the types given associated with template argument list of the layout
  /// descriptor.
  ///
  /// In case the number of properties for the element exceeds the
  /// number of properties in the template argument list, the remaining
  /// properties are directly stored using the property type as defined in the
  /// PLY header. This assumes that the output buffer can hold the required
  /// amount of data; failing to satisfy this precondition results in undefined
  /// behavior.
  /// @{
  // TODO(ton): probably better to add another parameter 'size' to guard
  // against overwriting the input buffer.
  template<typename... Ts>
  typename std::enable_if<MaybeMemcpyable<Ts...>::value, void>::type read(
      const PlyElement &element,
      reflect::Layout<Ts...> layout) const
  {
    const PropertyConstIterator first = element.properties().begin();
    const PropertyConstIterator last = element.properties().end();
    if (detail::isMemcpyable<Ts...>(first, last)) { this->template memcpy<Ts...>(layout.data(), element); }
    else { readElements<Ts...>(element, layout); }
  }

  template<typename... Ts>
  typename std::enable_if<!MaybeMemcpyable<Ts...>::value, void>::type read(
      const PlyElement &element,
      reflect::Layout<Ts...> layout) const
  {
    readElements<Ts...>(element, layout);
  }
  /// @}

  void skip(const PlyElement &element) const { this->skipElement(element); }

private:
  template<typename... Ts>
  void readElements(const PlyElement &element, reflect::Layout<Ts...> layout) const
  {
    const PropertyConstIterator first = element.properties().begin();
    const PropertyConstIterator last = element.properties().end();
    const PropertyConstIterator firstToSkip = first + detail::numProperties<Ts...>();

    std::uint8_t *dest = layout.data();

    if (firstToSkip < last)
    {
      // Note; even though the following may seem specific for binary parsing
      // only, it is still useful in terms of an ASCII parser. That is, in case
      // any bytes need to be skipped, the ASCII parser will just ignore the
      // remainder of the line to read, and as such skip to the next element.
      const std::size_t numBytesToSkip =
          std::accumulate(firstToSkip, last, 0ul, [](std::size_t acc, const PlyProperty &p) {
            return acc + sizeOf(p.isList() ? p.sizeType() : p.type());
          });

      for (std::size_t i{0}; i < element.size(); ++i)
      {
        dest = detail::align(readElement<Ts...>(dest, first, last), layout.alignment());
        this->skipProperties(numBytesToSkip);
      }
    }
    else
    {
      for (std::size_t i{0}; i < element.size(); ++i)
      {
        dest = detail::align(readElement<Ts...>(dest, first, last), layout.alignment());
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
  std::uint8_t *readElement(std::uint8_t *dest, PropertyConstIterator first, PropertyConstIterator last) const
  {
    return readElement<U, Ts...>(readElement<T>(dest, first, last), first + detail::numProperties<T>(), last);
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
  std::uint8_t *readListProperty(std::uint8_t *dest, reflect::Type<reflect::Array<DestT, N>>) const
  {
    static_assert(std::is_arithmetic<PlyT>::value, "unexpected PLY data type");

    // TODO(ton): skip the number that defines the list in the PLY data, we
    // expect it to be of length N; throw an exception here in case they do no match?
    this->template skipNumber<PlySizeT>();
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT)));
    return this->template readNumbers<PlyT, DestT, N>(dest);
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

  template<typename DestT>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Skip>) const
  {
    return dest;
  }

  template<typename DestT>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Stride<DestT>>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT))) + sizeof(DestT);
  }

  template<typename PlyT, typename DestT, size_t N>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Pack<DestT, N>>) const
  {
    static_assert(std::is_arithmetic<PlyT>::value, "unexpected PLY data type");
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT)));
    return this->template readNumbers<PlyT, DestT, N>(dest);
  }

  template<typename TypeTag>
  typename std::enable_if<detail::isList<TypeTag>(), std::uint8_t *>::type readProperty(
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
  typename std::enable_if<
      !detail::isList<TypeTag>() && !std::is_same<typename TypeTag::DestT, reflect::Skip>::value,
      std::uint8_t *>::type
  readProperty(std::uint8_t *dest, const PlyProperty &property, TypeTag tag) const
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

  template<typename TypeTag>
  typename std::enable_if<std::is_same<typename TypeTag::DestT, reflect::Skip>::value, std::uint8_t *>::type
  readProperty(std::uint8_t *dest, const PlyProperty &property, TypeTag tag) const
  {
    this->skipProperty(property);
    return dest;
  }
};

}}

#endif
