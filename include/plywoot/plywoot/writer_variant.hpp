#ifndef PLYWOOT_WRITER_VARIANT_HPP
#define PLYWOOT_WRITER_VARIANT_HPP

#include "ascii_writer_policy.hpp"
#include "binary_writer_policy.hpp"
#include "writer.hpp"

#include <ostream>

namespace plywoot { namespace detail {

/// For lack of `std::variant`, create a cheap tagged union.
class WriterVariant
{
public:
  WriterVariant(std::ostream &os, PlyFormat format) : format_{format}, variant_{os, format} {}

  WriterVariant(const WriterVariant &) = delete;
  WriterVariant &operator=(const WriterVariant &) = delete;

  ~WriterVariant()
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.close();
        variant_.ascii.~Writer<detail::AsciiWriterPolicy>();
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.close();
        variant_.bbe.~Writer<detail::BinaryBigEndianWriterPolicy>();
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.close();
        variant_.ble.~Writer<detail::BinaryLittleEndianWriterPolicy>();
        break;
    }
  }

  template<typename... Ts>
  void write(const PlyElement &element, const std::uint8_t *src, std::size_t n) const
  {
    const auto first = element.properties().begin();
    const auto last = element.properties().end();

    switch (format_)
    {
      case PlyFormat::Ascii:
        for (std::size_t i = 0; i < n; ++i) { src = variant_.ascii.writeElement<Ts...>(src, first, last); }
        break;
      case PlyFormat::BinaryBigEndian:
        for (std::size_t i = 0; i < n; ++i) { src = variant_.bbe.writeElement<Ts...>(src, first, last); }
        break;
      case PlyFormat::BinaryLittleEndian:
        for (std::size_t i = 0; i < n; ++i) { src = variant_.ble.writeElement<Ts...>(src, first, last); }
        break;
    }
  }

private:
  PlyFormat format_;

  union U {
    U(std::ostream &os, PlyFormat format)
    {
      switch (format)
      {
        case PlyFormat::Ascii:
          new (&ascii) detail::Writer<detail::AsciiWriterPolicy>{os};
          break;
        case PlyFormat::BinaryBigEndian:
          new (&bbe) detail::Writer<detail::BinaryBigEndianWriterPolicy>{os};
          break;
        case PlyFormat::BinaryLittleEndian:
          new (&ble) detail::Writer<detail::BinaryLittleEndianWriterPolicy>{os};
          break;
      }
    }

    detail::Writer<detail::AsciiWriterPolicy> ascii;
    detail::Writer<detail::BinaryBigEndianWriterPolicy> bbe;
    detail::Writer<detail::BinaryLittleEndianWriterPolicy> ble;
  } variant_;
};

}}

#endif
