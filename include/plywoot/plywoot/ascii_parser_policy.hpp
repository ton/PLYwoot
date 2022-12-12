#ifndef PLYWOOT_ASCII_PARSER_POLICY_HPP
#define PLYWOOT_ASCII_PARSER_POLICY_HPP

#include "buffered_istream.hpp"
#include "exceptions.hpp"
#include "reflect.hpp"
#include "std.hpp"
#include "types.hpp"

#include <cstdint>
#include <string>
#include <vector>

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
  AsciiParserPolicy(std::istream &is, std::vector<PlyElement> elements)
      : is_{is}, elements_{std::move(elements)}
  {
  }

  /// Seeks to the start of the data for the given element. Returns whether
  /// seeking was successful.
  bool seekTo(const PlyElement &element) const
  {
    std::size_t numLines{0};
    auto first = elements_.begin();
    const auto last = elements_.end();
    while (first != last && *first != element) { numLines += first++->size(); }

    if (first != last && *first == element)
    {
      is_.seekToBegin();
      is_.skipLines(numLines);
    }

    return first != last;
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

  /// Reads `n` numbers of the given type `T` from the input stream.
  template<typename T>
  std::uint8_t *readNumbers(std::uint8_t *dest, std::size_t n) const
  {
    // TODO(ton): needs to be specialized for improved performance.
    for (std::size_t i = 0; i < n; ++i, dest += sizeof(T)) { *reinterpret_cast<T *>(dest) = readNumber<T>(); }
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
  void skipProperties(size_t n) const { if (n > 0) is_.skipLines(1); }

private:
  mutable detail::BufferedIStream is_;
  std::vector<PlyElement> elements_;
};

}
}

#endif
