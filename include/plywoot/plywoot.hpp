#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include "plywoot/ascii_parser_policy.hpp"
#include "plywoot/binary_little_endian_parser_policy.hpp"
#include "plywoot/buffered_istream.hpp"
#include "plywoot/exceptions.hpp"
#include "plywoot/header_parser.hpp"
#include "plywoot/io.hpp"
#include "plywoot/parser.hpp"
#include "plywoot/reflect.hpp"
#include "plywoot/std.hpp"
#include "plywoot/types.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace plywoot {

// TODO(ton): documentation

/// Represents an input PLY data stream that can be queried for data.
class IStream
{
public:
  IStream(std::istream &is) : IStream{is, detail::HeaderParser{is}} {}

  // TODO(ton): add support for storing comments

  /// Returns all elements associated with this PLY file.
  const std::vector<PlyElement> &elements() const { return elements_; }

  /// Returns a pair where the first element is a copy of the element with the
  /// given name in case it exists. The second element is a boolean that
  /// indicates whether the requested element was found. In case a requested
  /// element was not found in the input data, a default constructed element
  /// is returned.
  std::pair<PlyElement, bool> element(const std::string &name) const
  {
    const auto it = std::find_if(
        elements_.begin(), elements_.end(), [&](const PlyElement &e) { return e.name() == name; });
    return it != elements_.end() ? std::pair<PlyElement, bool>{*it, true}
                                 : std::pair<PlyElement, bool>{{}, false};
  }

  /// Returns the format of the input PLY data stream.
  PlyFormat format() const { return format_; }

  /// Reads the given element from the PLY input data stream, expecting elements
  /// where every property can be statically casted to the given property type
  /// in the template argument list.
  template<typename T, typename Layout>
  std::vector<T> read(const PlyElement &element) const
  {
    std::vector<T> result(element.size());
    read(element, Layout{result});
    return result;
  }

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
    switch (format_)
    {
      case PlyFormat::Ascii: {
        detail::Parser<detail::AsciiParserPolicy> parser{is_, elements_};
        parser.read<Ts...>(element, layout);
      }
      break;
      case PlyFormat::BinaryLittleEndian: {
        detail::Parser<detail::BinaryLittleEndianParserPolicy> parser{is_, elements_};
        parser.read<Ts...>(element, layout);
      }
      break;
      default:
        break;
    }
  }

private:
  /// Constructs a PLY file from the given input stream and header parser.
  IStream(std::istream &is, const detail::HeaderParser &parser)
      : is_{is}, elements_{parser.elements()}, format_{parser.format()}
  {
  }

  mutable detail::BufferedIStream is_;

  std::vector<PlyElement> elements_;
  PlyFormat format_;
};

/// Represents an output PLY data stream that can be used to output data to a
/// PLY format.
class OStream
{
public:
  OStream(PlyFormat format) : format_{format} {}

  /// Queues an element with the associated data for writing. Elements will be
  /// stored in the same order they are queued.
  template<typename... Ts>
  void add(const PlyElement &element, const reflect::Layout<Ts...> &layout)
  {
    elementWriteClosures_.emplace_back(element, [this, layout](std::ostream &os, const PlyElement &e) {
      switch (format_)
      {
        case PlyFormat::Ascii:
          write<PlyFormat::Ascii, Ts...>(os, e, layout.data(), layout.size());
          break;
        case PlyFormat::BinaryLittleEndian:
          write<PlyFormat::BinaryLittleEndian, Ts...>(os, e, layout.data(), layout.size());
        default:
          break;
      }
    });
  }

  /// Writes all data as a PLY file queued through `addElement()` to the given
  /// output stream.
  void write(std::ostream &os) const
  {
    writeHeader(os);

    for (const auto &elementClosurePair : elementWriteClosures_)
    {
      const PlyElement &element{elementClosurePair.first};
      const auto &writeFn{elementClosurePair.second};
      writeFn(os, element);
    }
  }

private:
  void writeHeader(std::ostream &os) const
  {
    os << "ply\n";

    switch (format_)
    {
      case PlyFormat::Ascii:
        os << "format ascii 1.0\n";
        break;
      case PlyFormat::BinaryBigEndian:
        os << "format binary_big_endian 1.0\n";
        break;
      case PlyFormat::BinaryLittleEndian:
        os << "format binary_little_endian 1.0\n";
        break;
    }

    for (const auto &elementClosurePair : elementWriteClosures_)
    {
      const PlyElement &element{elementClosurePair.first};
      os << "element " << element.name() << ' ' << element.size() << '\n';

      for (const PlyProperty &property : element.properties())
      {
        if (property.isList())
        {
          os << "property list " << property.sizeType() << ' ' << property.type() << ' ' << property.name()
             << '\n';
        }
        else { os << "property " << property.type() << ' ' << property.name() << '\n'; }
      }
    }

    os << "end_header\n";
  }

  template<PlyFormat format, typename PlyT, typename SrcT>
  typename std::enable_if<std::is_arithmetic<SrcT>::value, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<SrcT>)
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT)));
    detail::io::writeNumber<format>(os, static_cast<PlyT>(*reinterpret_cast<const SrcT *>(src)));
    return src + sizeof(SrcT);
  }

  template<PlyFormat format, typename PlyT, typename SrcT>
  typename std::enable_if<!std::is_arithmetic<SrcT>::value, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<SrcT>)
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT))) + sizeof(SrcT);
  }

  /// Specialization for the meta property type `Stride<T>`, this will just skip
  /// over a member variable of type `T` in the source buffer.
  template<PlyFormat format, typename SrcT>
  const std::uint8_t *writeProperty(
      std::ostream &,
      const std::uint8_t *src,
      reflect::Type<reflect::Stride<SrcT>>)
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(SrcT))) + sizeof(SrcT);
  }

  /// Specialization for the meta property type `Array<T, N, SizeT>`, this will
  /// write a list of N properties of type T.
  // TODO(ton): reimplement for binary, gets rid of
  // `detail::io::writeTokenSeparator()` and likely improves performance.
  template<PlyFormat format, typename PlyT, typename PlySizeT, typename SrcT, size_t N>
  const std::uint8_t *writeListProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<reflect::Array<SrcT, N>>)
  {
    static_assert(N > 0, "invalid array size specified (needs to be larger than zero)");

    detail::io::writeNumber<format, PlySizeT>(os, N);
    detail::io::writeTokenSeparator<format>(os);
    for (std::size_t i = 0; i < N - 1; ++i)
    {
      src = writeProperty<format, PlyT>(os, src, reflect::Type<SrcT>{});
      detail::io::writeTokenSeparator<format>(os);
    }
    src = writeProperty<format, PlyT>(os, src, reflect::Type<SrcT>{});

    return src;
  }

  /// Specialization for a vector of type `T`.
  template<PlyFormat format, typename PlyT, typename PlySizeT, typename SrcT>
  const std::uint8_t *writeListProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<std::vector<SrcT>>)
  {
    const std::vector<SrcT> &v = *reinterpret_cast<const std::vector<SrcT> *>(src);

    detail::io::writeNumber<format, PlySizeT>(os, v.size());
    if (!v.empty())
    {
      detail::io::writeTokenSeparator<format>(os);
      for (std::size_t i = 0; i < v.size() - 1; ++i)
      {
        detail::io::writeNumber<format, PlyT>(os, v[i]);
        detail::io::writeTokenSeparator<format>(os);
      }
      detail::io::writeNumber<format, PlyT>(os, v.back());
    }

    return static_cast<const std::uint8_t *>(detail::align(src, alignof(std::vector<SrcT>))) +
           sizeof(std::vector<SrcT>);
  }

  template<PlyFormat format, typename PlyT, typename TypeTag>
  const std::uint8_t *writeListProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag)
  {
    switch (property.sizeType())
    {
      case PlyDataType::Char:
        return writeListProperty<format, PlyT, char>(os, src, tag);
      case PlyDataType::UChar:
        return writeListProperty<format, PlyT, unsigned char>(os, src, tag);
      case PlyDataType::Short:
        return writeListProperty<format, PlyT, short>(os, src, tag);
      case PlyDataType::UShort:
        return writeListProperty<format, PlyT, unsigned short>(os, src, tag);
      case PlyDataType::Int:
        return writeListProperty<format, PlyT, int>(os, src, tag);
      case PlyDataType::UInt:
        return writeListProperty<format, PlyT, unsigned int>(os, src, tag);
      case PlyDataType::Float:
        return writeListProperty<format, PlyT, float>(os, src, tag);
      case PlyDataType::Double:
        return writeListProperty<format, PlyT, double>(os, src, tag);
    }

    return src;
  }

  template<PlyFormat format, typename TypeTag>
  typename std::enable_if<TypeTag::isList, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag)
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return writeListProperty<format, char>(os, src, property, tag);
      case PlyDataType::UChar:
        return writeListProperty<format, unsigned char>(os, src, property, tag);
      case PlyDataType::Short:
        return writeListProperty<format, short>(os, src, property, tag);
      case PlyDataType::UShort:
        return writeListProperty<format, unsigned short>(os, src, property, tag);
      case PlyDataType::Int:
        return writeListProperty<format, int>(os, src, property, tag);
      case PlyDataType::UInt:
        return writeListProperty<format, unsigned int>(os, src, property, tag);
      case PlyDataType::Float:
        return writeListProperty<format, float>(os, src, property, tag);
      case PlyDataType::Double:
        return writeListProperty<format, double>(os, src, property, tag);
    }

    return src;
  }

  template<PlyFormat format, typename TypeTag>
  typename std::enable_if<!TypeTag::isList, const std::uint8_t *>::type writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag)
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return writeProperty<format, char>(os, src, tag);
      case PlyDataType::UChar:
        return writeProperty<format, unsigned char>(os, src, tag);
      case PlyDataType::Short:
        return writeProperty<format, short>(os, src, tag);
      case PlyDataType::UShort:
        return writeProperty<format, unsigned short>(os, src, tag);
      case PlyDataType::Int:
        return writeProperty<format, int>(os, src, tag);
      case PlyDataType::UInt:
        return writeProperty<format, unsigned int>(os, src, tag);
      case PlyDataType::Float:
        return writeProperty<format, float>(os, src, tag);
      case PlyDataType::Double:
        return writeProperty<format, double>(os, src, tag);
    }

    return src;
  }

  /// Bottom case of the function recursively defined on a list of input property
  /// types to write, this is simply a no-op.
  template<PlyFormat format>
  const std::uint8_t *writeElement(
      std::ostream &,
      const std::uint8_t *src,
      PropertyConstIterator,
      PropertyConstIterator)
  {
    return src;
  }

  /// Reads a single object of type `T` from the input buffer `src` and in case
  /// a corresponding element property is defined for it (`first` < `last`),
  /// writes that property to the output stream. Otherwise, it will skip over
  /// the object in the input buffer.
  template<PlyFormat format, typename SrcT>
  const std::uint8_t *writeElement(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last)
  {
    return first < last ? writeProperty<format>(os, src, *first, reflect::Type<SrcT>{})
                        : writeProperty<format>(os, src, reflect::Type<reflect::Stride<SrcT>>{});
  }

  /// Reads an  object of type `T` from the input buffer `src`, and writes it.
  // TODO(ton): reimplement for binary, gets rid of
  // `detail::io::writeTokenSeparator()` and likely improves performance.
  template<PlyFormat format, typename T, typename U, typename... Ts>
  const std::uint8_t *writeElement(
      std::ostream &os,
      const std::uint8_t *src,
      PropertyConstIterator first,
      PropertyConstIterator last)
  {
    src = writeElement<format, T>(os, src, first++, last);
    if (first < last) detail::io::writeTokenSeparator<format>(os);
    return writeElement<format, U, Ts...>(os, src, first, last);
  }

  template<PlyFormat format, typename... Ts>
  void write(std::ostream &os, const PlyElement &element, const std::uint8_t *src, std::size_t n)
  {
    const auto first{element.properties().begin()};
    const auto last{element.properties().end()};

    for (std::size_t i{0}; i < n; ++i)
    {
      src = writeElement<format, Ts...>(os, src, first, last);

      // In case the element defines more properties than the source data,
      // append the missing properties with a default value of zero.
      if (sizeof...(Ts) < static_cast<std::size_t>(std::distance(first, last)))
      {
        // TODO(ton): broken in case the PLY property type is a list...
        for (auto it = first + sizeof...(Ts); it < last; ++it)
        {
          detail::io::writeTokenSeparator<format>(os);

          switch (it->type())
          {
            case PlyDataType::Char:
              detail::io::writeNumber<format, char>(os, 0);
              break;
            case PlyDataType::UChar:
              detail::io::writeNumber<format, unsigned char>(os, 0);
              break;
            case PlyDataType::Short:
              detail::io::writeNumber<format, short>(os, 0);
              break;
            case PlyDataType::UShort:
              detail::io::writeNumber<format, unsigned short>(os, 0);
              break;
            case PlyDataType::Int:
              detail::io::writeNumber<format, int>(os, 0);
              break;
            case PlyDataType::UInt:
              detail::io::writeNumber<format, unsigned int>(os, 0);
              break;
            case PlyDataType::Float:
              detail::io::writeNumber<format, float>(os, 0);
              break;
            case PlyDataType::Double:
              detail::io::writeNumber<format, double>(os, 0);
              break;
          }
        }
      }

      detail::io::writeNewline<format>(os);
    }
  }

  using ElementWriteClosure = std::function<void(std::ostream &, const PlyElement &)>;

  /// All queued elements with the associated data.
  std::vector<std::pair<PlyElement, ElementWriteClosure>> elementWriteClosures_;
  /// Format the PLY data should be written in.
  PlyFormat format_;
};
}

#endif
