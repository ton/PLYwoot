#ifndef PLYWOOT_IO_HPP
#define PLYWOOT_IO_HPP

#include "buffered_istream.hpp"

#include <ostream>

namespace plywoot { namespace detail { namespace io {

template<typename T>
struct CharToInt
{
  template<typename U>
  U operator()(U &&u) const
  {
    return u;
  }

  int operator()(char c) const { return static_cast<int>(c); }
  int operator()(signed char c) const { return static_cast<int>(c); }
  unsigned operator()(unsigned char c) const { return static_cast<unsigned>(c); }
};

template<PlyFormat format, typename T>
typename std::enable_if<format == PlyFormat::Ascii, T>::type readNumber(BufferedIStream &is)
{
  is.skipWhitespace();
  if (is.eof()) { throw UnexpectedEof(); }

  is.buffer(256);
  return detail::to_number<T>(is.data(), is.data() + 256, &is.data());
}

template<PlyFormat format, typename T>
typename std::enable_if<format != PlyFormat::Ascii, T>::type readNumber(BufferedIStream &is)
{
  is.buffer(sizeof(T));
  T t = *reinterpret_cast<const T *>(is.data());
  is.skip(sizeof(T));
  return t;
}

/// TODO
template<PlyFormat format, typename T>
typename std::enable_if<format == PlyFormat::Ascii, void>::type skipNumber(BufferedIStream &is)
{
  is.skipWhitespace();
  is.skipNonWhitespace();
}

/// TODO
template<PlyFormat format, typename T>
typename std::enable_if<format != PlyFormat::Ascii, void>::type skipNumber(BufferedIStream &is)
{
  is.skip(sizeof(T));
}

template<PlyFormat format>
typename std::enable_if<format != PlyFormat::Ascii, void>::type skipProperty(
    BufferedIStream &is,
    const PlyProperty &property)
{
  if (!property.isList())
  {
    is.skip(sizeOf(property.type()));
  }
}

/// Skips the remaining properties of an element, where the range of properties
/// to skip is given by the [first, last) PlyProperty  iterators.
/// @{
template<PlyFormat format, typename It>
typename std::enable_if<format == PlyFormat::Ascii, void>::type skipProperties(BufferedIStream &is, It, It)
{
  // Skipping remaining properties for an ASCII input file boils down to
  // skipping the remainder of the input line.
  is.skipLines(1);
}

template<PlyFormat format, typename It>
typename std::enable_if<format != PlyFormat::Ascii, void>::type skipProperties(
    BufferedIStream &is,
    It first,
    It last)
{
  while (first != last) { skipProperty<format>(is, *first++); }
}
/// @}

template<PlyFormat format, typename T>
typename std::enable_if<format == PlyFormat::Ascii, void>::type skipElement(
    BufferedIStream &is,
    std::size_t size)
{
  is.skipLines(size);
}

template<PlyFormat format, typename T>
typename std::enable_if<format != PlyFormat::Ascii, void>::type skipElement(
    BufferedIStream &is,
    std::size_t size)
{
  is.skip(size);
}

template<PlyFormat format, typename T>
typename std::enable_if<format == PlyFormat::Ascii, void>::type writeNumber(std::ostream &os, T t)
{
  os << CharToInt<T>{}(t);
}

template<PlyFormat format, typename T>
typename std::enable_if<format == PlyFormat::BinaryLittleEndian, void>::type writeNumber(
    std::ostream &os,
    T t)
{
  os.write(reinterpret_cast<const char *>(&t), sizeof(T));
}

/// Writes a token separator, only required for an ASCII format.
template<PlyFormat format>
typename std::enable_if<format == PlyFormat::Ascii, void>::type writeTokenSeparator(std::ostream &os)
{
  os.put(' ');
}

template<PlyFormat format>
typename std::enable_if<format != PlyFormat::Ascii, void>::type writeTokenSeparator(std::ostream &)
{
}

/// Writes a newline to the given output stream \a os for an ASCII format.
template<PlyFormat format>
typename std::enable_if<format == PlyFormat::Ascii, void>::type writeNewline(std::ostream &os)
{
  os.put('\n');
}

/// Newlines are not written for binary formats, hence writing a newline for a
/// non-ASCII format is not implemented.
template<PlyFormat format>
typename std::enable_if<format != PlyFormat::Ascii, void>::type writeNewline(std::ostream &)
{
}

}}}

#endif
