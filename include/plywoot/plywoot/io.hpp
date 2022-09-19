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
