#ifndef PLYWOOT_ISTREAM_ASCII_HPP
#define PLYWOOT_ISTREAM_ASCII_HPP

#include "buffered_istream.hpp"
#include "exceptions.hpp"
#include "reflect.hpp"
#include "std.hpp"
#include "types.hpp"

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

// TODO(ton): documentation

/// Represents an input PLY data stream that can be queried for data.
class AsciiPolicy
{
public:
  AsciiPolicy(BufferedIStream &is, const std::vector<PlyElement> &elements) : is_{is}, elements_{elements} {}

  /// Seeks to the start of the data for the given element. Returns whether
  /// seeking was successful.
  bool seekTo(const PlyElement &element) const
  {
    std::size_t numLines{0};
    auto first{elements_.begin()};
    const auto last{elements_.end()};
    while (first != last && *first != element) { numLines += first++->size(); }

    if (first != last && *first == element)
    {
      is_.seekToBegin();
      is_.skipLines(numLines);
    }

    return first != last;
  }

  template<typename T>
  T readNumber() const
  {
    is_.skipWhitespace();
    if (is_.eof()) { throw UnexpectedEof(); }

    is_.buffer(256);
    return detail::to_number<T>(is_.data(), is_.data() + 256, &is_.data());
  }

  template<typename T>
  void skipNumber() const
  {
    is_.skipWhitespace();
    is_.skipNonWhitespace();
  }

  void skipProperties(PlyPropertyConstIterator, PlyPropertyConstIterator) const { is_.skipLines(1); }

private:
  mutable detail::BufferedIStream is_;
  const std::vector<PlyElement> &elements_;
};

}
}

#endif
