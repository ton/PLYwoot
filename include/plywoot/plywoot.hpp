#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include "plywoot/buffered_istream.hpp"
#include "plywoot/exceptions.hpp"
#include "plywoot/header_parser.hpp"
#include "plywoot/header_scanner.hpp"
#include "plywoot/io.hpp"
#include "plywoot/reflect.hpp"
#include "plywoot/std.hpp"
#include "plywoot/types.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace plywoot {

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
      case PlyFormat::Ascii:
        read<PlyFormat::Ascii, Ts...>(layout.data(), element);
        break;
      case PlyFormat::BinaryLittleEndian:
        read<PlyFormat::BinaryLittleEndian, Ts...>(layout.data(), element);
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

  template<PlyFormat format, typename... Ts>
  void read(std::uint8_t *dest, const PlyElement &element) const
  {
    if (seekTo<format>(element))
    {
      const PropertyConstIterator last = element.properties().end();

      for (std::size_t i{0}; i < element.size(); ++i)
      {
        dest = readElement<format, Ts...>(dest, element.properties().begin(), last);

        // In case the number of properties in the PLY element exceeds the
        // number of properties to map to, ignore the remainder of the PLY
        // element properties...
        if (element.properties().size() > sizeof...(Ts))
        {
          detail::io::skipProperties<format>(is_, element.properties().begin() + sizeof...(Ts), last);
        }
      }
    }
  }

  template<PlyFormat format, typename PlyT, typename DestT>
  typename std::enable_if<std::is_arithmetic<DestT>::value, std::uint8_t *>::type readProperty(
      std::uint8_t *dest,
      reflect::Type<DestT>) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT)));
    *reinterpret_cast<DestT *>(dest) = detail::io::readNumber<format, PlyT>(is_);
    return dest + sizeof(DestT);
  }

  template<PlyFormat, typename PlyT, typename DestT>
  typename std::enable_if<!std::is_arithmetic<DestT>::value, std::uint8_t *>::type readProperty(
      std::uint8_t *dest,
      reflect::Type<DestT>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT))) + sizeof(DestT);
  }

  template<PlyFormat format, typename PlyT, typename DestT, size_t N, typename SizeT>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Array<DestT, N, SizeT>>) const
  {
    // TODO(ton): skip the number that defines the list in the PLY data, we
    // expect it to be of length N; throw an exception here in case they do no match?
    detail::io::skipNumber<format, SizeT>(is_);
    for (size_t i = 0; i < N; ++i) { dest = readProperty<format, PlyT>(dest, reflect::Type<DestT>{}); }
    return dest;
  }

  template<PlyFormat, typename PlyT, typename DestT>
  std::uint8_t *readProperty(std::uint8_t *dest, reflect::Type<reflect::Stride<DestT>>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(DestT))) + sizeof(DestT);
  }

  template<PlyFormat format, typename TypeTag>
  std::uint8_t *readProperty(std::uint8_t *dest, const PlyProperty &property, TypeTag tag) const
  {
    switch (property.type())
    {
      case PlyDataType::Char:
        return readProperty<format, char>(dest, tag);
      case PlyDataType::UChar:
        return readProperty<format, unsigned char>(dest, tag);
      case PlyDataType::Short:
        return readProperty<format, short>(dest, tag);
      case PlyDataType::UShort:
        return readProperty<format, unsigned short>(dest, tag);
      case PlyDataType::Int:
        return readProperty<format, int>(dest, tag);
      case PlyDataType::UInt:
        return readProperty<format, unsigned int>(dest, tag);
      case PlyDataType::Float:
        return readProperty<format, float>(dest, tag);
      case PlyDataType::Double:
        return readProperty<format, double>(dest, tag);
    }

    return dest;
  }

  /// Skips the properties of the types given in the variadic template parameter
  /// list of these function in the *destination* data pointed to by `dest`,
  /// taking into account default alignment rules.
  /// @{
  template<typename T>
  std::uint8_t *skipProperties(std::uint8_t *dest) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(T)));
    return dest + sizeof(T);
  }

  template<typename T, typename U, typename... Ts>
  std::uint8_t *skipProperties(std::uint8_t *dest) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(T)));
    return skipProperties<U, Ts...>(dest + sizeof(T));
  }
  /// @}

  template<PlyFormat>
  std::uint8_t *readElement(std::uint8_t *dest, PlyPropertyConstIterator, PlyPropertyConstIterator) const
  {
    return dest;
  }

  template<PlyFormat format, typename T>
  std::uint8_t *readElement(std::uint8_t *dest, PlyPropertyConstIterator first, PlyPropertyConstIterator last)
      const
  {
    return first < last ? readProperty<format>(dest, *first, reflect::Type<T>{}) : skipProperties<T>(dest);
  }

  template<PlyFormat format, typename T, typename U, typename... Ts>
  std::uint8_t *readElement(std::uint8_t *dest, PropertyConstIterator first, PropertyConstIterator last) const
  {
    return first < last ? readElement<format, U, Ts...>(
                              readProperty<format>(dest, *first, reflect::Type<T>{}), first + 1, last)
                        : skipProperties<T, U, Ts...>(dest);
  }

  /// Seeks to the start of the data for the given element. Returns whether
  /// seeking was successful.
  /// @{
  template<PlyFormat format>
  typename std::enable_if<format == PlyFormat::Ascii, bool>::type seekTo(const PlyElement &element) const
  {
    std::size_t numLines{0};
    auto first{elements().begin()};
    const auto last{elements().end()};
    while (first != last && *first != element) { numLines += first++->size(); }

    if (first != last && *first == element)
    {
      is_.seekToBegin();
      is_.skipLines(numLines);
    }

    return first != last;
  }

  template<PlyFormat format>
  typename std::enable_if<format != PlyFormat::Ascii, bool>::type seekTo(const PlyElement &element) const
  {
    std::size_t numBytes{0};
    auto first{elements().begin()};
    const auto last{elements().end()};
    while (first != last && *first != element) { numBytes += elementSizeInBytes(*first++); }

    if (first != last && *first == element)
    {
      is_.seekToBegin();
      is_.skip(numBytes);
    }

    return first != last;
  }
  /// @}

  size_t elementSizeInBytes(const PlyElement &element) const
  {
    auto it = elementSize_.lower_bound(element.name());
    if (it == elementSize_.end() || it->first != element.name())
    {
      std::size_t numBytes{0};
      for (const PlyProperty &p : element.properties())
      {
        if (!p.isList()) { numBytes += sizeOf(p.type()); }
        else
        {
          // TODO(ton): implement this for both variable and fixed size
          // lists...it is annoying we need to have a separate size hint for
          // this most likely :(
        }
      }
      it = elementSize_.insert(it, std::make_pair(element.name(), element.size() * numBytes));
    }
    return it->second;
  }

  mutable detail::BufferedIStream is_;
  mutable std::map<std::string, std::ptrdiff_t> elementSize_;

  std::vector<PlyElement> elements_;
  PlyFormat format_;
};

// TODO(ton): documentation
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
    // TODO(ton): test what happens if you pass in an rvalue layout; likely this
    // would result in a crash? Better to copy data/size into the closure to
    // prevent this.
    elementWriteClosures_.emplace_back(element, [this, &layout](std::ostream &os, const PlyElement &e) {
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
    // TODO(ton): add support for comments
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

  /// TODO
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

  /// TODO
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
  template<PlyFormat format, typename PlyT, typename SrcT, size_t N, typename SizeT>
  const std::uint8_t *writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      reflect::Type<reflect::Array<SrcT, N, SizeT>>)
  {
    static_assert(N > 0, "invalid array size specified (needs to be larger than zero)");

    detail::io::writeNumber<format>(os, *reinterpret_cast<const SizeT *>(src));
    detail::io::writeTokenSeparator<format>(os);
    for (std::size_t i = 0; i < N - 1; ++i)
    {
      src = writeProperty<format, PlyT>(os, src, reflect::Type<SrcT>{});
      detail::io::writeTokenSeparator<format>(os);
    }
    src = writeProperty<format, PlyT>(os, src, reflect::Type<SrcT>{});

    return src;
  }

  template<PlyFormat format, typename TypeTag>
  const std::uint8_t *writeProperty(
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
      // TODO(ton): ASCII specific code
      if (sizeof...(Ts) < static_cast<std::size_t>(std::distance(first, last)))
      {
        const std::size_t numExtra{std::distance(first, last) - sizeof...(Ts)};
        for (size_t j = 0; j < numExtra; ++j) os.write(" 0", 2);
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
