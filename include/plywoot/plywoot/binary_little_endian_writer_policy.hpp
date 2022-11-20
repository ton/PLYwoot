#ifndef PLYWOOT_BINARY_LITTLE_ENDIAN_WRITER_POLICY_HPP
#define PLYWOOT_BINARY_LITTLE_ENDIAN_WRITER_POLICY_HPP

#include <ostream>

namespace plywoot { namespace detail {

/// Defines a writer policy that deals with binary output streams.
class BinaryLittleEndianWriterPolicy
{
public:
  /// Writes the number `t` of the given type `T` to the given binary output
  /// stream in little endian format.
  template<typename T>
  void writeNumber(std::ostream &os, T t) const
  {
    os.write(reinterpret_cast<const char *>(&t), sizeof(T));
  }

  /// Writes a newline, ignored for binary output formats.
  void writeNewline(std::ostream &) const {}

  /// Writes a token separator, ignored for binary output formats.
  void writeTokenSeparator(std::ostream &) const {}
};

}}

#endif
