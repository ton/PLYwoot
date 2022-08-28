#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include "plywoot/exceptions.hpp"
#include "plywoot/header_parser.hpp"
#include "plywoot/header_scanner.hpp"
#include "plywoot/reflect.hpp"
#include "plywoot/std.hpp"
#include "plywoot/types.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace plywoot {

/// Represents an input PLY data stream that can be queried for data.
class IStream
{
public:
  IStream() = default;
  IStream(std::istream &is) : IStream{is, detail::HeaderParser{is}} {}

  // TODO(ton): add support for storing comments

  /// Returns all elements associated with this PLY file.
  std::vector<PlyElement> elements() { return elements_; }
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
    if (format_ == PlyFormat::Ascii) { readAscii<Ts...>(element, layout.data()); }
  }

private:
  /// Constructs a PLY file from the given input stream and header parser.
  IStream(std::istream &is, const detail::HeaderParser &parser)
      : is_{is}, headerOffset_{is_.tellg()}, elements_{parser.elements()}, format_{parser.format()}
  {
  }

  template<typename... Ts>
  std::uint8_t *readAscii(const PlyElement &element, std::uint8_t *dest) const
  {
    if (seekTo(element))
    {
      for (std::size_t i{0}; i < element.size(); ++i)
      {
        dest = readAsciiElement<Ts...>(dest);

        // In case the number of properties in the element exceeds the number of
        // properties to read, ignore the remainder of the element.
        // TODO(ton): maybe we need to make this more explicit; what should the
        // default behavior be? Skipping or adding without a cast?
        if (sizeof...(Ts) < element.properties().size()) { skipLines(1); }
      }
    }

    return dest;
  }

  template<
      typename T,
      typename TypeTag,
      typename std::enable_if<std::is_arithmetic<T>::value, std::size_t>::type = 0>
  std::uint8_t *readAsciiProperty(std::uint8_t *dest, TypeTag) const
  {
    dest = static_cast<std::uint8_t *>(detail::align(dest, alignof(T)));
    *reinterpret_cast<T *>(dest) = readNumber<T>();
    return dest + sizeof(T);
  }

  template<
      typename T,
      typename TypeTag,
      typename std::enable_if<!std::is_arithmetic<T>::value, std::size_t>::type = 0>
  std::uint8_t *readAsciiProperty(std::uint8_t *dest, TypeTag) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(T))) + sizeof(T);
  }

  template<typename, typename T, size_t N>
  std::uint8_t *readAsciiProperty(std::uint8_t *dest, detail::Type<reflect::Array<T, N>>) const
  {
    // TODO(ton): skip the number that defines the list in the PLY data, we
    // expect it to be of length N; throw an exception here in case they do no match?
    skipNumber();
    for (size_t i = 0; i < N; ++i) { dest = readAsciiProperty<T>(dest, detail::Type<T>{}); }
    return dest;
  }

  template<typename, typename T>
  std::uint8_t *readAsciiProperty(std::uint8_t *dest, detail::Type<reflect::Stride<T>>) const
  {
    return static_cast<std::uint8_t *>(detail::align(dest, alignof(T))) + sizeof(T);
  }

  std::uint8_t *readAsciiElement(std::uint8_t *dest) const { return dest; }

  template<typename T>
  std::uint8_t *readAsciiElement(std::uint8_t *dest) const
  {
    return readAsciiProperty<T>(dest, detail::Type<T>{});
  }

  template<typename T, typename U, typename... Ts>
  std::uint8_t *readAsciiElement(std::uint8_t *dest) const
  {
    return readAsciiElement<U, Ts...>(readAsciiProperty<T>(dest, detail::Type<T>{}));
  }

  template<typename Number>
  Number readNumber() const
  {
    // Ignore whitespace and throw in case EOF was found...
    while (0 <= *c_ && *c_ <= 0x20) { readCharacter(); }
    if (*c_ == EOF) { throw UnexpectedEof(); }

    // ..then read number.
    char buf[128];
    char *head{buf};
    while (*c_ > 0x20)
    {
      *head++ = *c_;
      readCharacter();
    }
    *head = '\0';

    return detail::to_number<Number>(buf, buf + sizeof(buf));
  }

  void skipNumber() const
  {
    while (0 <= *c_ && *c_ <= 0x20) readCharacter();
    while (*c_ > 0x20) readCharacter();
  }

  /// Reads the next character in the input stream and advances the read head by
  /// one character.
  void readCharacter() const
  {
    ++c_;
    if (c_ >= buffer_ + bufferedBytes_) { bufferData(); }
  }

  void readBinary(const PlyElement &element, void *data, std::size_t size);

  /// Seeks to the start of the data for the given element. Returns whether
  /// seeking was successful.
  bool seekTo(const PlyElement &element) const
  {
    // Position the read head at the start of the data just after the header.
    // Resets any buffered data.
    bufferedBytes_ = 0;
    c_ = buffer_;

    is_.clear();  // need to clear eofbit() in case it is set, otherwise the
                  // first read after the seek will fail
    is_.seekg(headerOffset_);

    // Skip all element data for elements that come before the given element.
    auto first{elements().begin()};
    const auto last{elements().end()};
    while (first != last && *first != element) { skipLines(first++->size()); }

    return first != last;
  }

  void bufferData() const
  {
    if (!is_.read(buffer_, BufferSize))
    {
      bufferedBytes_ = is_.gcount();
      buffer_[bufferedBytes_] = EOF;
    }
    else { bufferedBytes_ = BufferSize; }
    c_ = buffer_;
  }

  /// Skips `n` lines in the input, places the read head at the first
  /// character after the `n`-th newline character that as found in the input,
  /// or in case that character does not exist, at EOF.
  void skipLines(std::size_t n) const
  {
    if (c_ >= buffer_ + bufferedBytes_) { bufferData(); }
    while (*c_ != EOF && n > 0)
    {
      auto first = static_cast<char *>(std::memchr(c_, '\n', bufferedBytes_ - (c_ - buffer_)));
      if (first)
      {
        c_ = first + 1;
        if (c_ >= buffer_ + bufferedBytes_) { bufferData(); }
        --n;
      }
      else { bufferData(); }
    }
  }

  std::istream &is_;
  std::istream::pos_type headerOffset_;

  std::vector<PlyElement> elements_;
  PlyFormat format_;

  constexpr static size_t BufferSize{8192};
  /// Buffered data, always a null terminated string.
  mutable char buffer_[BufferSize] = {};
  /// Number of bytes in the input buffer retrieved from disk.
  mutable size_t bufferedBytes_{0};
  /// Character the scanner's read head is currently pointing to. Invariant
  /// after construction of the scanner is:
  ///
  ///       buffer_ <= c_ < (buffer_ + sizeof(buffer_) - 1)
  ///
  /// Note that the invariant allows for one character lookahead without the
  /// need to check whether we need to read data from disk.
  mutable char *c_{buffer_ + BufferSize};
  /// Text representation of the current number.
  std::string numberString_;
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
    elementWriteClosures_.emplace_back(
        element, [this, &layout](std::ostream &os, const PlyElement &e) { write(os, e, layout); });
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

  template<typename... Ts>
  void write(std::ostream &os, const PlyElement &element, const reflect::Layout<Ts...> &layout)
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        writeAscii<Ts...>(os, element, layout.data(), layout.size());
        break;
      default:
        break;
    }
  }

  /// TODO
  template<typename T, typename TypeTag, typename std::enable_if<std::is_arithmetic<T>::value, int>::type = 0>
  const std::uint8_t *writeAsciiArithmeticProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag)
  {
    src = static_cast<const std::uint8_t *>(detail::align(src, alignof(T)));

    switch (property.type())
    {
      // Note all data types that 'fit' are just casted to int here, to
      // prevent having issues with 'char' being interpreted as ASCII
      // characters instead of numbers.
      case PlyDataType::Char:
      case PlyDataType::UChar:
      case PlyDataType::Short:
      case PlyDataType::UShort:
      case PlyDataType::Int:
        os << static_cast<const int>(*reinterpret_cast<const T *>(src));
        break;
      case PlyDataType::UInt:
        os << static_cast<const unsigned int>(*reinterpret_cast<const T *>(src));
        break;
      case PlyDataType::Float:
        os << static_cast<const float>(*reinterpret_cast<const T *>(src));
        break;
      case PlyDataType::Double:
        os << static_cast<const double>(*reinterpret_cast<const T *>(src));
        break;
    }

    return src + sizeof(T);
  }

  /// TODO
  template<
      typename T,
      typename TypeTag,
      typename std::enable_if<!std::is_arithmetic<T>::value, int>::type = 0>
  const std::uint8_t *writeAsciiArithmeticProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag)
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(T))) + sizeof(T);
  }

  /// Note; type tag parameter is merely to facility overloading based on the
  /// template type.
  template<typename T, typename TypeTag>
  const std::uint8_t *writeAsciiProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      TypeTag tag)
  {
    return writeAsciiArithmeticProperty<T>(os, src, property, tag);
  }

  /// Specialization for the meta property type `Stride<T>`, this will just skip
  /// over a member variable of type `T` in the source buffer.
  template<typename, typename T>
  const std::uint8_t *writeAsciiProperty(
      std::ostream &,
      const std::uint8_t *src,
      const PlyProperty &property,
      detail::Type<reflect::Stride<T>>)
  {
    return static_cast<const std::uint8_t *>(detail::align(src, alignof(T))) + sizeof(T);
  }

  /// Specialization for the meta property type `Array<T, N>`, this will write a
  /// list of N properties of type T.
  template<typename, typename T, size_t N>
  const std::uint8_t *writeAsciiProperty(
      std::ostream &os,
      const std::uint8_t *src,
      const PlyProperty &property,
      detail::Type<reflect::Array<T, N>>)
  {
    static_assert(N > 0, "invalid array size specified (needs to be larger than zero)");

    // Write the size of the list, followed by all comma separated elements.
    os << int(N) << ' ';
    for (std::size_t i = 0; i < N - 1; ++i)
    {
      src = writeAsciiProperty<T>(os, src, property, detail::Type<T>{});
      os.put(' ');
    }
    src = writeAsciiProperty<T>(os, src, property, detail::Type<T>{});

    return src;
  }

  /// Writes all properties of the given element, iterating over all types
  /// specified in the source type layout, matching them with properties defined
  /// for the element. This is the bottom case and ends the recursion.
  const std::uint8_t *writeAsciiElement(
      std::ostream &,
      const std::uint8_t *src,
      ConstPropertyIterator,
      ConstPropertyIterator)
  {
    return src;
  }

  /// Reads a single object of type `T` from the input buffer `src`. In case a
  /// corresponding element property is defined for it (`first` < `last`),
  /// writes that property to the output stream. Otherwise, it will skip over
  /// the object in the input buffer.
  template<typename T>
  const std::uint8_t *writeAsciiElement(
      std::ostream &os,
      const std::uint8_t *src,
      ConstPropertyIterator first,
      ConstPropertyIterator last)
  {
    return first < last
               ? writeAsciiProperty<T>(os, src, *first, detail::Type<T>{})
               : writeAsciiProperty<reflect::Stride<T>>(os, src, {}, detail::Type<reflect::Stride<T>>{});
  }

  /// Reads an  object of type `T` from the input buffer `src`, and writes it.
  template<typename T, typename U, typename... Ts>
  const std::uint8_t *writeAsciiElement(
      std::ostream &os,
      const std::uint8_t *src,
      ConstPropertyIterator first,
      ConstPropertyIterator last)
  {
    src = writeAsciiElement<T>(os, src, first++, last);
    if (first < last) os.put(' ');
    return writeAsciiElement<U, Ts...>(os, src, first, last);
  }

  template<typename... Ts>
  void writeAscii(std::ostream &os, const PlyElement &element, const std::uint8_t *src, std::size_t n)
  {
    const auto first{element.properties().begin()};
    const auto last{element.properties().end()};

    for (std::size_t i{0}; i < n; ++i)
    {
      src = writeAsciiElement<Ts...>(os, src, first, last);

      // In case the element defines more properties than the source data,
      // append the missing properties with a default value of zero.
      if (sizeof...(Ts) < static_cast<std::size_t>(std::distance(first, last)))
      {
        const std::size_t numExtra{std::distance(first, last) - sizeof...(Ts)};
        for (size_t j = 0; j < numExtra; ++j) os.write(" 0", 2);
      }
      os.put('\n');
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
