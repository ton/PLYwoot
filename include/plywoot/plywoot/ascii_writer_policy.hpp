#ifndef PLYWOOT_ASCII_WRITER_POLICY_HPP
#define PLYWOOT_ASCII_WRITER_POLICY_HPP

#include <ostream>

namespace plywoot { namespace detail {

namespace {

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

}

/// Defines a writer policy that deals with ASCII output streams.
class AsciiWriterPolicy
{
public:
  /// Writes the number `t` of the given type `T` to the given ASCII output
  /// stream.
  template<typename T>
  void writeNumber(std::ostream &os, T t) const
  {
    os << CharToInt<T>{}(t);
  }

  /// Writes a newline separate to the given output stream `os`.
  void writeNewline(std::ostream &os) const { os.put('\n'); }

  /// Writes a token separator to the given output stream `os`.
  void writeTokenSeparator(std::ostream &os) const { os.put(' '); }
};

}}

#endif
