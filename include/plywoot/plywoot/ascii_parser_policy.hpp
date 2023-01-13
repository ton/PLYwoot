#ifndef PLYWOOT_ASCII_PARSER_POLICY_HPP
#define PLYWOOT_ASCII_PARSER_POLICY_HPP

#include "buffered_istream.hpp"
#include "exceptions.hpp"
#include "reflect.hpp"
#include "std.hpp"
#include "types.hpp"

#include <cstdint>
#include <string>

namespace plywoot {

// Base class for all parser exceptions.
struct ParserException : Exception
{
  ParserException(const std::string &message) : Exception("parser error: " + message) {}
};

// Unexpected end-of-file exception.
struct UnexpectedEof : ParserException
{
  UnexpectedEof() : ParserException("unexpected end of file") {}
};

namespace detail {

/// Defines a parser policy that deals with ASCII input streams.
class AsciiParserPolicy
{
public:
  AsciiParserPolicy(std::istream &is) : is_{is} {}

  /// Skips the given element in the current input stream, assuming the read
  /// head is at the start of that element.
  void skipElement(const PlyElement &e) const
  {
    is_.skipLines(e.size());
  }

  /// Reads a number of the given type `T` from the input stream.
  template<typename T>
  T readNumber() const
  {
    is_.skipWhitespace();
    if (is_.eof()) { throw UnexpectedEof(); }

    is_.buffer(256);
    return detail::to_number<T>(is_.data(), is_.data() + 256, &is_.data());
  }

  /// Reads `N` numbers of the given type `PlyT` from the input stream, and
  /// stores them contiguously at the given destination in memory as numbers of
  /// type `DestT`. Returns a pointer pointing just after the last number stored
  /// at `dest`.
  template<typename PlyT, typename DestT, std::size_t N>
  std::uint8_t *readNumbers(std::uint8_t *dest) const
  {
    // TODO(ton): needs to be specialized for improved performance.
    for (std::size_t i = 0; i < N; ++i, dest += sizeof(DestT)) { *reinterpret_cast<DestT *>(dest) = readNumber<PlyT>(); }
    return dest;
  }

  /// Skips a number of the given type `T` in the input stream.
  template<typename T>
  void skipNumber() const
  {
    is_.skipWhitespace();
    is_.skipNonWhitespace();
  }

  /// Skips property data, totaling `n` bytes.
  void skipProperties(size_t n) const
  {
    if (n > 0) is_.skipLines(1);
  }

private:
  mutable detail::BufferedIStream is_;
};

}
}

#endif
