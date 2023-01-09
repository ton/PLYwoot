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

  /// Writes a list of numbers of type `SrcT` to the given ASCII output stream,
  /// which includes the size of that list.
  /// The first template argument represents the type of the size of the list in
  /// the output PLY file, which can be ignored for ASCII PLY formats, as is the
  /// type of the PLY number type.
  template<typename PlySizeT, typename PlyT, typename SrcT>
  void writeList(std::ostream &os, const SrcT *t, std::size_t n) const
  {
    os << n << ' ';
    writeNumbers<PlyT, SrcT>(os, t, n);
  }

  /// Writes a list of numbers of type `SrcT` to the given ASCII output stream.
  /// The first template argument represents the type of the size of the list in
  /// the output PLY file, which can be ignored for ASCII PLY formats, as is the
  /// type of the PLY number type.
  template<typename PlyT, typename SrcT>
  void writeNumbers(std::ostream &os, const SrcT *t, std::size_t n) const
  {
    for (std::size_t i = 0; i < n - 1; ++i) { os << *t++ << ' '; }
    os << *t;
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
};

}}

#endif
