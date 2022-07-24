#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

namespace plywoot
{
  enum class PlyDataType
  {
    Char,
    UChar,
    Short,
    UShort,
    Int,
    UInt,
    Float,
    Double
  };

  inline std::ostream &operator<<(std::ostream &os, PlyDataType dataType)
  {
    switch (dataType)
    {
      case PlyDataType::Char:
        return os << "char";
      case PlyDataType::UChar:
        return os << "uchar";
      case PlyDataType::Short:
        return os << "short";
      case PlyDataType::UShort:
        return os << "ushort";
      case PlyDataType::Int:
        return os << "int";
      case PlyDataType::UInt:
        return os << "uint";
      case PlyDataType::Float:
        return os << "float";
      case PlyDataType::Double:
        return os << "double";
    }

    return os;
  }

  enum class PlyFormat
  {
    Ascii,
    BinaryBigEndian,
    BinaryLittleEndian,
  };

  struct PlyProperty
  {
    std::string name;
    PlyDataType type;

    bool isList{false};
    PlyDataType sizeType{PlyDataType::Char};
    std::size_t sizeHint{0};

    size_t numBytes() const
    {
      switch (type)
      {
        case PlyDataType::Char:
        case PlyDataType::UChar:
          return 1;
        case PlyDataType::Short:
        case PlyDataType::UShort:
          return 2;
        case PlyDataType::Int:
        case PlyDataType::UInt:
        case PlyDataType::Float:
          return 4;
        case PlyDataType::Double:
          return 8;
      }
    }

    inline friend bool operator==(const PlyProperty &x, const PlyProperty &y)
    {
      // Note; the size hint is not part of the identity of a property.
      return x.type == y.type && x.isList == y.isList && x.sizeType == y.sizeType &&
             x.name == y.name;
    }

    inline friend bool operator!=(const PlyProperty &x, const PlyProperty &y) { return !(x == y); }
  };

  struct PlyElement
  {
    std::string name;
    std::size_t size;
    std::vector<PlyProperty> properties;

    /// Returns a pair where the first element is a copy of the property with
    /// the given name in case it exists. The second element is a Boolean that
    /// indicates whether the requested property was found. In case a requested
    /// property was not found for this element, a default constructed property
    /// is returned.
    std::pair<PlyProperty, bool> property(const std::string &propertyName) const
    {
      const auto it = std::find_if(
          properties.begin(), properties.end(),
          [&](const PlyProperty &p) { return p.name == propertyName; });
      return it != properties.end() ? std::pair<PlyProperty, bool>{*it, true}
                                    : std::pair<PlyProperty, bool>{{}, false};
    }

    inline friend bool operator==(const PlyElement &x, const PlyElement &y)
    {
      return x.size == y.size && x.name == y.name && x.properties == y.properties;
    }
    inline friend bool operator!=(const PlyElement &x, const PlyElement &y) { return !(x == y); }
  };
}

#include "plywoot_exceptions.hpp"
#include "plywoot_header_parser.hpp"
#include "plywoot_header_scanner.hpp"
#include "plywoot_std.hpp"

#include <iostream>

namespace plywoot
{
  // Unexpected end-of-file exception.
  struct UnexpectedEof : ParserException
  {
    UnexpectedEof() : ParserException("unexpected end of file") {}
  };

  /// Missing size hint exception is thrown in case the user request to write
  /// elements with lists that have no size hint size. For now, it is not
  /// possible to write dynamic lists.
  struct MissingSizeHint : Exception
  {
    MissingSizeHint(const std::string &propertyName)
        : Exception(std::string{"missing size hint for property '"} + propertyName + "'")
    {
    }
  };

  class IStream
  {
  public:
    IStream() = default;
    IStream(std::istream &is) : IStream{is, HeaderParser{is}} {}

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
          elements_.begin(), elements_.end(), [&](const PlyElement &e) { return e.name == name; });
      return it != elements_.end() ? std::pair<PlyElement, bool>{*it, true}
                                   : std::pair<PlyElement, bool>{{}, false};
    }

    /// Returns the format of the input PLY data stream.
    PlyFormat format() const { return format_; }

    /// Reads the given element from the PLY input data stream, expecting
    /// elements where every property has exactly the same type as the
    /// corresponding properties of objects of type T.
    template<typename T>
    std::vector<T> read(const PlyElement &element) const
    {
      std::vector<T> result(element.size);
      read(result.data(), element);
      return result;
    }

    /// Reads the given element from the PLY input data stream, expecting
    /// elements where every property can be casted to the given property type
    /// in the template argument list.
    // TODO(ton): test what happens in case the number of property types given
    // does not match the number of properties in the element; the remaining
    // properties will be read 'uncasted'.
    template<typename T, typename PropertyType, typename... PropertyTypes>
    std::vector<T> read(const PlyElement &element) const
    {
      std::vector<T> result(element.size);
      read<T, PropertyType, PropertyTypes...>(result.data(), element);
      return result;
    }

    /// Reads the given element from the PLY input data stream, storing data in
    /// the given destination buffer `dest` using the types defined by the
    /// element. This assumes that the output buffer can hold the required
    /// amount of data; failing to satisfy this precondition results in
    /// undefined behavior.
    // TODO(ton): probably better to add another parameter 'size' to guard
    // against overwriting the input buffer.
    // TODO(ton): test what happens in case the number of property types given
    // in the argument list *exceeds* the number of properties associated with
    // the element.
    void read(void *dest, const PlyElement &element) const
    {
      const auto first = elements().begin();
      const auto last = elements().end();
      if (std::find(first, last, element) != last)
      {
        if (format_ == PlyFormat::Ascii) { readAscii(static_cast<std::uint8_t *>(dest), element); }
      }
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
    template<typename T, typename PropertyType, typename... PropertyTypes>
    void read(void *dest, const PlyElement &element) const
    {
      const auto first = elements().begin();
      const auto last = elements().end();
      if (std::find(first, last, element) != last)
      {
        if (format_ == PlyFormat::Ascii)
        {
          readAscii<PropertyType, PropertyTypes...>(static_cast<std::uint8_t *>(dest), element);
        }
      }
    }

  private:
    /// Constructs a PLY file from the given input stream and header parser.
    IStream(std::istream &is, const HeaderParser &parser)
        : is_{is},
          headerOffset_{is_.tellg()},
          elements_{parser.elements()},
          format_{parser.format()}
    {
    }

    template<typename... Casts>
    std::uint8_t *readAscii(std::uint8_t *dest, const PlyElement &element) const
    {
      is_.seekg(headerOffset_);
      auto first{elements().begin()};
      const auto last{elements().end()};
      while (first != last && *first != element) { skipLines(first++->size); }

      for (std::size_t i{0}; i < element.size; ++i)
      {
        dest = readAsciiCastedProperties<Casts...>(dest);
      }

      return dest;
    }

    /// Reads from ASCII without casting any of the properties.
    void readAscii(std::uint8_t *dest, const PlyElement &element) const
    {
      is_.seekg(headerOffset_);
      auto first{elements().begin()};
      const auto last{elements().end()};
      while (first != last && *first != element) skipLines(first++->size);

      std::size_t offset{0};
      for (std::size_t i{0}; i < element.size; ++i)
      {
        for (const PlyProperty &property : element.properties)
        {
          const size_t n{property.isList ? readNumber<std::size_t>() : 1};

          switch (property.type)
          {
            case PlyDataType::Char:
              offset = storeNumbers<std::int8_t>(n, dest, offset);
              break;
            case PlyDataType::UChar:
              offset = storeNumbers<std::uint8_t>(n, dest, offset);
              break;
            case PlyDataType::Short:
              offset = storeNumbers<std::int16_t>(n, dest, offset);
              break;
            case PlyDataType::UShort:
              offset = storeNumbers<std::uint16_t>(n, dest, offset);
              break;
            case PlyDataType::Int:
              offset = storeNumbers<std::int32_t>(n, dest, offset);
              break;
            case PlyDataType::UInt:
              offset = storeNumbers<std::uint32_t>(n, dest, offset);
              break;
            case PlyDataType::Float:
              offset = storeNumbers<float>(n, dest, offset);
              break;
            case PlyDataType::Double:
              offset = storeNumbers<double>(n, dest, offset);
              break;
          }
        }
      }
    }

    template<typename T>
    std::uint8_t *readAsciiCastedProperties(std::uint8_t *dest) const
    {
      dest = static_cast<std::uint8_t *>(pstd::align(dest, alignof(T)));
      *reinterpret_cast<T *>(dest) = readNumber<T>();
      return dest + sizeof(T);
    }

    template<typename T, typename U, typename... Ts>
    std::uint8_t *readAsciiCastedProperties(std::uint8_t *dest) const
    {
      dest = static_cast<std::uint8_t *>(pstd::align(dest, alignof(T)));
      *reinterpret_cast<T *>(dest) = readNumber<T>();
      return readAsciiCastedProperties<U, Ts...>(dest + sizeof(T));
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

      return pstd::to_number<Number>(buf);
    }

    template<typename Number>
    std::size_t storeNumbers(std::size_t n, std::uint8_t *data, std::size_t offset) const
    {
      for (std::size_t i{0}; i < n; ++i)
      {
        const std::size_t alignedOffset{pstd::roundup(offset, alignof(Number))};
        *reinterpret_cast<Number *>(data + alignedOffset) = readNumber<Number>();
        offset = alignedOffset + sizeof(Number);
      }
      return offset;
    }

    /// Reads the next character in the input stream and advances the read head by
    /// one character.
    void readCharacter() const
    {
      ++c_;
      if (c_ >= buffer_ + bufferedBytes_) { bufferData(); }
    }

    void readBinary(const PlyElement &element, void *data, std::size_t size);

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
        if (first == nullptr || first == (buffer_ + bufferedBytes_ - 1)) { bufferData(); }
        else { c_ = first + 1; }
        --n;
      }
    }

    std::istream &is_;
    std::istream::pos_type headerOffset_;

    std::vector<PlyElement> elements_;
    PlyFormat format_;

    constexpr static size_t BufferSize{8192};
    /// Buffered data, always a null terminated string.
    mutable char buffer_[BufferSize];
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
  class OStream
  {
  public:
    OStream(PlyFormat format) : format_{format} {}

    /// Queues an element with the associated data for writing. Elements will be
    /// stored in the same order they are queued. List properties require a size
    /// hint for now.
    ///
    /// \throw MissingSizeHint in case a property is present without a size hint
    template<typename T>
    void add(const PlyElement &element, const std::vector<T> &values)
    {
      for (const PlyProperty &p : element.properties)
      {
        if (p.isList && p.sizeHint == 0) { throw MissingSizeHint(p.name); }
      }

      elementWriteClosures_.emplace_back(
          element,
          [this, &values](std::ostream &os, const PlyElement &e) { write(os, e, values); });
    }

    /// Queues an element with the associated data for writing. Elements will be
    /// stored in the same order they are queued. List properties require a size
    /// hint for now.
    ///
    /// \throw MissingSizeHint in case a property is present without a size hint
    template<typename T, typename PropertyType, typename... PropertyTypes>
    void add(const PlyElement &element, const std::vector<T> &values)
    {
      for (const PlyProperty &p : element.properties)
      {
        if (p.isList && p.sizeHint == 0) { throw MissingSizeHint(p.name); }
      }

      elementWriteClosures_.emplace_back(
          element, [this, &values](std::ostream &os, const PlyElement &e)
          { write<T, PropertyType, PropertyTypes...>(os, e, values); });
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
        os << "element " << element.name << ' ' << element.size << '\n';

        for (const PlyProperty &property : element.properties)
        {
          if (property.isList)
          {
            os << "property list " << property.sizeType << ' ' << property.type << ' '
               << property.name << '\n';
          }
          else { os << "property " << property.type << ' ' << property.name << '\n'; }
        }
      }

      os << "end_header\n";
    }

    template<typename T, typename... PropertyTypes>
    void write(std::ostream &os, const PlyElement &element, const std::vector<T> &values)
    {
      switch (format_)
      {
        case PlyFormat::Ascii:
          writeAscii<T, PropertyTypes...>(os, element, values);
          break;
        default:
          break;
      }
    }

    template<typename T>
    void writeAscii(std::ostream &os, const PlyElement &element, const std::vector<T> &values)
    {
      for (const T &value : values)
      {
        const std::uint8_t *src{reinterpret_cast<const std::uint8_t *>(&value)};

        for (std::size_t i{0}; i < element.properties.size(); ++i)
        {
          const PlyProperty &property{element.properties[i]};

          size_t n{1};
          if (property.isList)
          {
            assert(property.sizeHint > 0);
            os << int(property.sizeHint) << ' ';
            n = property.sizeHint;
          }

          switch (property.type)
          {
            case PlyDataType::Char:
              src = streamNumbers<std::int8_t>(os, n, src);
              break;
            case PlyDataType::UChar:
              src = streamNumbers<std::uint8_t>(os, n, src);
              break;
            case PlyDataType::Short:
              src = streamNumbers<std::int16_t>(os, n, src);
              break;
            case PlyDataType::UShort:
              src = streamNumbers<std::uint16_t>(os, n, src);
              break;
            case PlyDataType::Int:
              src = streamNumbers<std::int32_t>(os, n, src);
              break;
            case PlyDataType::UInt:
              src = streamNumbers<std::uint32_t>(os, n, src);
              break;
            case PlyDataType::Float:
              src = streamNumbers<float>(os, n, src);
              break;
            case PlyDataType::Double:
              src = streamNumbers<double>(os, n, src);
              break;
          }

          if (i < element.properties.size() - 1) { os.put(' '); }
        }

        os.put('\n');
      }
    }

    template<typename T, typename PropertyType, typename... PropertyTypes>
    void writeAscii(std::ostream &os, const PlyElement &element, const std::vector<T> &values)
    {
      for (const T &value : values)
      {
        const auto *src{reinterpret_cast<const std::uint8_t *>(&value)};
        writeAsciiCastedProperty<PropertyType, PropertyTypes...>(os, src, element, 0);
        os.put('\n');
      }
    }

    template<typename T>
    const std::uint8_t *writeAsciiCastedProperty(
        std::ostream &os,
        const std::uint8_t *src,
        const PlyElement &element,
        std::size_t propertyIndex)
    {
      src = static_cast<const std::uint8_t *>(pstd::align(src, alignof(T)));
      switch (element.properties[propertyIndex].type)
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

    template<typename T, typename U, typename... PropertyTypes>
    const std::uint8_t *writeAsciiCastedProperty(
        std::ostream &os,
        const std::uint8_t *src,
        const PlyElement &element,
        const std::size_t propertyIndex)
    {
      src = writeAsciiCastedProperty<T>(os, src, element, propertyIndex);
      os.put(' ');
      return writeAsciiCastedProperty<U, PropertyTypes...>(os, src, element, propertyIndex + 1);
    }

    template<typename Number>
    const std::uint8_t *streamNumbers(std::ostream &os, size_t n, const std::uint8_t *data) const
    {
      for (size_t i{0}; i < n; ++i)
      {
        data = static_cast<const std::uint8_t *>(pstd::align(data, alignof(Number)));
        os << pstd::CharToInt<Number>{}(*reinterpret_cast<const Number *>(data));
        data += sizeof(Number);

        if (i < n - 1) { os.put(' '); }
      }

      return data;
    }

    using ElementWriteClosure = std::function<void(std::ostream &, const PlyElement &)>;

    /// All queued elements with the associated data.
    std::vector<std::pair<PlyElement, ElementWriteClosure>> elementWriteClosures_;
    /// Format the PLY data should be written in.
    PlyFormat format_;
  };
}

#endif
