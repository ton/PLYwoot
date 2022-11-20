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

  /// Outputs empty data for the range of properties [`first`, `last`). Note
  /// that a property that is undefined is always stored as a zero character in
  /// ASCII mode; regardless whether the property is a list of a single element,
  /// since in case of a list we store a zero-element list.
  void writeMissingProperties(std::ostream &os, PlyPropertyConstIterator first, PlyPropertyConstIterator last)
      const
  {
    while (first++ != last) { os.write(" 0", 2); }
  }

  /// Writes a newline separate to the given output stream `os`.
  void writeNewline(std::ostream &os) const { os.put('\n'); }

  /// Writes a token separator to the given output stream `os`.
  void writeTokenSeparator(std::ostream &os) const { os.put(' '); }
};

}}

#endif
