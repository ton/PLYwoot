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

  /// Reads the given element from the PLY input data stream, expecting
  /// elements where every property can be casted to the given property type
  /// in the template argument list.
  // TODO(ton): test what happens in case the number of property types given
  // does not match the number of properties in the element; the remaining
  // properties will be read 'uncasted'.
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
  // TODO(ton): test what happens in case the number of property types given
  // in the argument list *exceeds* the number of properties associated with
  // the element.
  template<typename... Ts>
  void read(const PlyElement &element, reflect::Layout<Ts...> layout) const
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        read<PlyFormat::Ascii, Ts...>(element, layout.data());
        break;
      case PlyFormat::BinaryLittleEndian:
        read<PlyFormat::BinaryLittleEndian, Ts...>(element, layout.data());
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
  std::uint8_t *read(const PlyElement &element, std::uint8_t *dest) const
  {
    if (seekTo<format>(element))
    {
      const std::uint8_t *start = dest;
      for (std::size_t i{0}; i < element.size(); ++i)
      {
        dest = readElement<format, Ts...>(dest);

        // In case the number of properties in the element exceeds the number of
        // properties to read, ignore the remainder of the element.
        // TODO(ton): maybe we need to make this more explicit; what should the
        // default behavior be? Skipping or adding without a cast?
        if (sizeof...(Ts) < element.properties().size())
        {
          detail::io::skipProperties<format>(
              is_, element.properties().begin() + sizeof...(Ts), element.properties().end());
        }
      }

      elementSize_.emplace(element.name(), dest - start);
    }

    return dest;
  }

  template<
      PlyFormat format,
      typename T,
      typename TypeTag,
      typename std::enable_if<std::is_arithmetic<T>::value, std::size_t>::type = 0>
  std::uint8_t *readProperty(std::uint8_t *dest, TypeTag) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(T)));
    *reinterpret_cast<T *>(dest) = detail::io::readNumber<format, T>(is_);
    return dest + sizeof(T);
  }

  template<
      PlyFormat,
      typename T,
      typename TypeTag,
      typename std::enable_if<!std::is_arithmetic<T>::value, std::size_t>::type = 0>
  std::uint8_t *readProperty(std::uint8_t *dest, TypeTag) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(T))) + sizeof(T);
  }

  template<PlyFormat format, typename, typename T, size_t N, typename SizeT>
  std::uint8_t *readProperty(std::uint8_t *dest, detail::Type<reflect::Array<T, N, SizeT>>) const
  {
    // TODO(ton): skip the number that defines the list in the PLY data, we
    // expect it to be of length N; throw an exception here in case they do no match?
    detail::io::skipNumber<format, SizeT>(is_);
    for (size_t i = 0; i < N; ++i) { dest = readProperty<format, T>(dest, detail::Type<T>{}); }
    return dest;
  }

  template<PlyFormat, typename, typename T>
  std::uint8_t *readProperty(std::uint8_t *dest, detail::Type<reflect::Stride<T>>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(T))) + sizeof(T);
  }

  template<PlyFormat>
  std::uint8_t *readElement(std::uint8_t *dest) const
  {
    return dest;
  }

  template<PlyFormat format, typename T>
  std::uint8_t *readElement(std::uint8_t *dest) const
  {
    return readProperty<format, T>(dest, detail::Type<T>{});
  }

  template<PlyFormat format, typename T, typename U, typename... Ts>
  std::uint8_t *readElement(std::uint8_t *dest) const
  {
    return readElement<format, U, Ts...>(readProperty<format, T>(dest, detail::Type<T>{}));
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

    is_.seekToBegin();
    if (first != last) is_.skipLines(numLines);

    return first != last;
  }

  template<PlyFormat format>
  typename std::enable_if<format != PlyFormat::Ascii, bool>::type seekTo(const PlyElement &element) const
  {
    is_.seekToBegin();

    auto first{elements().begin()};
    const auto last{elements().end()};
    while (first != last && *first != element)
    {
      // TODO(ton): skipping previously parsed elements is not supported; this
      // implies that out-of-order reading of elements for binary inputs is not
      // supported...:( We need to add `setLayout()` or something for this to
      // work properly, but it is quite ugly.
      auto it = elementSize_.find(first->name());
      if (it != elementSize_.end()) { is_.skip(it->second); }
      ++first;
    }

    return first != last;
  }
  /// @}

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
  template<
      PlyFormat format,
      typename T,
      typename TypeTag,
      typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
  const std::uint8_t *writeArithmeticProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag)
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(T)));
    detail::io::writeNumber<format>(os, *reinterpret_cast<const T *>(src));
    return src + sizeof(T);
  }

  /// TODO
  template<
      PlyFormat format,
      typename T,
      typename TypeTag,
      typename std::enable_if<!std::is_arithmetic<T>::value, int>::type = 0>
  const std::uint8_t *writeArithmeticProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag)
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(T))) + sizeof(T);
  }

  /// Note; type tag parameter is merely to facility overloading based on the
  /// template type.
  template<PlyFormat format, typename T, typename TypeTag>
  const std::uint8_t *writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag)
  {
    return writeArithmeticProperty<format, T>(os, src, property, tag);
  }

  /// Specialization for the meta property type `Stride<T>`, this will just skip
  /// over a member variable of type `T` in the source buffer.
  template<PlyFormat format, typename, typename T>
  const std::uint8_t *writeProperty(
      std::ostream &,
      const std::uint8_t *src,
      const PlyProperty &property,
      detail::Type<reflect::Stride<T>>)
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(T))) + sizeof(T);
  }

  /// Specialization for the meta property type `Array<T, N, SizeT>`, this will
  /// write a list of N properties of type T.
  // TODO(ton): reimplement for binary, gets rid of
  // `detail::io::writeTokenSeparator()` and likely improves performance.
  template<PlyFormat format, typename, typename T, size_t N, typename SizeT>
  const std::uint8_t *writeProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      detail::Type<reflect::Array<T, N, SizeT>>)
  {
    static_assert(N > 0, "invalid array size specified (needs to be larger than zero)");

    detail::io::writeNumber<format>(os, *reinterpret_cast<const SizeT *>(src));
    detail::io::writeTokenSeparator<format>(os);
    for (std::size_t i = 0; i < N - 1; ++i)
    {
      src = writeProperty<format, T>(os, src, property, detail::Type<T>{});
      detail::io::writeTokenSeparator<format>(os);
    }
    src = writeProperty<format, T>(os, src, property, detail::Type<T>{});

    return src;
  }

  /// Bottom case of the function recursively defined on a list of input property
  /// types to write, this is simply a no-op.
  template<PlyFormat format>
  const std::uint8_t *writeElement(
      std::ostream &,
      const std::uint8_t *src,
      ConstPropertyIterator,
      ConstPropertyIterator)
  {
    return src;
  }

  /// Reads a single object of type `T` from the input buffer `src` and in case
  /// a corresponding element property is defined for it (`first` < `last`),
  /// writes that property to the output stream. Otherwise, it will skip over
  /// the object in the input buffer.
  template<PlyFormat format, typename T>
  const std::uint8_t *writeElement(
      std::ostream &os,
      const std::uint8_t *src,
      ConstPropertyIterator first,
      ConstPropertyIterator last)
  {
    return first < last
               ? writeProperty<format, T>(os, src, *first, detail::Type<T>{})
               : writeProperty<format, reflect::Stride<T>>(os, src, {}, detail::Type<reflect::Stride<T>>{});
  }

  /// Reads an  object of type `T` from the input buffer `src`, and writes it.
  // TODO(ton): reimplement for binary, gets rid of
  // `detail::io::writeTokenSeparator()` and likely improves performance.
  template<PlyFormat format, typename T, typename U, typename... Ts>
  const std::uint8_t *writeElement(
      std::ostream &os,
      const std::uint8_t *src,
      ConstPropertyIterator first,
      ConstPropertyIterator last)
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
