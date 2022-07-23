#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include <algorithm>
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

    template<typename T, typename... PropertyTypes>
    std::vector<T> read(const PlyElement &element) const
    {
      const auto first = elements().begin();
      const auto last = elements().end();
      if (std::find(first, last, element) != last)
      {
        if (format_ == PlyFormat::Ascii) { return readAscii<T, PropertyTypes...>(element); }
      }

      return {};
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

    /// Reads from ASCII without casting any of the properties.
    template<typename T>
    std::vector<T> readAscii(const PlyElement &element) const
    {
      is_.seekg(headerOffset_);
      auto first{elements().begin()};
      const auto last{elements().end()};
      while (first != last && *first != element) skipLines(first++->size);

      std::vector<T> result(element.size);
      for (std::size_t i{0}; i < element.size; ++i)
      {
        T &t{result[i]};

        char dest[sizeof(T)];
        std::size_t offset{0};

        for (const PlyProperty &property : element.properties)
        {
          switch (property.type)
          {
            case PlyDataType::Char:
              offset = writeAsciiNumber(readNumber<std::int8_t>(), dest, offset);
              break;
            case PlyDataType::UChar:
              offset = writeAsciiNumber(readNumber<std::uint8_t>(), dest, offset);
              break;
            case PlyDataType::Short:
              offset = writeAsciiNumber(readNumber<std::int16_t>(), dest, offset);
              break;
            case PlyDataType::UShort:
              offset = writeAsciiNumber(readNumber<std::uint16_t>(), dest, offset);
              break;
            case PlyDataType::Int:
              offset = writeAsciiNumber(readNumber<std::int32_t>(), dest, offset);
              break;
            case PlyDataType::UInt:
              offset = writeAsciiNumber(readNumber<std::uint32_t>(), dest, offset);
              break;
            case PlyDataType::Float:
              offset = writeAsciiNumber(readNumber<float>(), dest, offset);
              break;
            case PlyDataType::Double:
              offset = writeAsciiNumber(readNumber<double>(), dest, offset);
              break;
          }
        }

        std::memcpy(&t, dest, sizeof(T));
      }

      return result;
    }

    template<typename T, typename Cast, typename... Casts>
    std::vector<T> readAscii(const PlyElement &element) const
    {
      is_.seekg(headerOffset_);
      auto first{elements().begin()};
      const auto last{elements().end()};
      while (first != last && *first != element) { skipLines(first++->size); }

      std::vector<T> result(element.size);
      for (std::size_t i{0}; i < element.size; ++i)
      {
        char dest[sizeof(T)];
        readAsciiCastedProperties<Cast, Casts...>(dest, std::size_t{0});

        std::memcpy(&result[i], dest, sizeof(T));
      }

      return result;
    }

    template<typename T>
    void readAsciiCastedProperties(char *dest, std::size_t offset) const
    {
      *reinterpret_cast<T *>(dest + offset) = readNumber<T>();
    }

    template<typename T, typename U, typename... Ts>
    void readAsciiCastedProperties(char *dest, std::size_t offset) const
    {
      *reinterpret_cast<T *>(dest + offset) = readNumber<T>();
      readAsciiCastedProperties<U, Ts...>(dest, pstd::roundup(offset + sizeof(T), alignof(T)));
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
    std::size_t writeAsciiNumber(Number number, char *data, std::size_t offset) const
    {
      const std::size_t alignedOffset{pstd::roundup(offset, alignof(Number))};
      *reinterpret_cast<Number *>(data + alignedOffset) = number;
      return alignedOffset + sizeof(Number);
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
    /// stored in the same order they are queued.
    template<typename T>
    void add(const PlyElement &element, const std::vector<T> &values)
    {
      elementWriteClosures_.emplace_back(
          element,
          [this, &values](std::ostream &os, const PlyElement &e) { write(os, e, values); });
    }

    /// Queues an element with the associated data for writing. Elements will be
    /// stored in the same order they are queued.
    template<typename T, typename PropertyType, typename... PropertyTypes>
    void add(const PlyElement &element, const std::vector<T> &values)
    {
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
        const char *src{reinterpret_cast<const char *>(&value)};
        std::size_t offset{0};

        for (std::size_t i{0}; i < element.properties.size(); ++i)
        {
          const PlyProperty &property{element.properties[i]};

          switch (property.type)
          {
            case PlyDataType::Char:
              offset = writeAsciiNumber<std::int8_t>(os, src, offset);
              break;
            case PlyDataType::UChar:
              offset = writeAsciiNumber<std::uint8_t>(os, src, offset);
              break;
            case PlyDataType::Short:
              offset = writeAsciiNumber<std::int16_t>(os, src, offset);
              break;
            case PlyDataType::UShort:
              offset = writeAsciiNumber<std::uint16_t>(os, src, offset);
              break;
            case PlyDataType::Int:
              offset = writeAsciiNumber<std::int32_t>(os, src, offset);
              break;
            case PlyDataType::UInt:
              offset = writeAsciiNumber<std::uint32_t>(os, src, offset);
              break;
            case PlyDataType::Float:
              offset = writeAsciiNumber<float>(os, src, offset);
              break;
            case PlyDataType::Double:
              offset = writeAsciiNumber<double>(os, src, offset);
              break;
            default:
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
        const std::uint8_t *src{reinterpret_cast<const std::uint8_t *>(&value)};
        writeAsciiCastedProperty<PropertyType, PropertyTypes...>(os, src, 0, element, 0),
            os.put('\n');
      }
    }

    template<typename T>
    void writeAsciiCastedProperty(
        std::ostream &os,
        const std::uint8_t *src,
        std::size_t offset,
        const PlyElement &element,
        std::size_t propertyIndex)
    {
      switch (element.properties[propertyIndex].type)
      {
        case PlyDataType::Char:
          os << static_cast<const char>(*reinterpret_cast<const T *>(src + offset));
          break;
        case PlyDataType::UChar:
          os << static_cast<const unsigned char>(*reinterpret_cast<const T *>(src + offset));
          break;
        case PlyDataType::Float:
          os << static_cast<const float>(*reinterpret_cast<const T *>(src + offset));
          break;
        default:
          break;
      }
    }

    template<typename T, typename U, typename... PropertyTypes>
    void writeAsciiCastedProperty(
        std::ostream &os,
        const std::uint8_t *src,
        const std::size_t offset,
        const PlyElement &element,
        const std::size_t propertyIndex)
    {
      switch (element.properties[propertyIndex].type)
      {
        case PlyDataType::Char:
          os << static_cast<const char>(*reinterpret_cast<const T *>(src + offset));
          break;
        case PlyDataType::UChar:
          os << static_cast<const unsigned char>(*reinterpret_cast<const T *>(src + offset));
          break;
        case PlyDataType::Float:
          os << static_cast<const float>(*reinterpret_cast<const T *>(src + offset));
          break;
        default:
          break;
      }

      os.put(' ');

      writeAsciiCastedProperty<U, PropertyTypes...>(os, src, offset, element, propertyIndex + 1);
    }

    template<typename Number>
    std::size_t writeAsciiNumber(std::ostream &os, const char *data, std::size_t offset) const
    {
      const std::size_t alignedOffset = pstd::roundup(offset, alignof(Number));
      os << pstd::CharToInt<Number>{}(*reinterpret_cast<const Number *>(data + alignedOffset));
      return alignedOffset + sizeof(Number);
    }

    using ElementWriteClosure = std::function<void(std::ostream &, const PlyElement &)>;

    /// All queued elements with the associated data.
    std::vector<std::pair<PlyElement, ElementWriteClosure>> elementWriteClosures_;
    /// Format the PLY data should be written in.
    PlyFormat format_;
  };
}

#endif
