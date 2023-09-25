#ifndef PLYWOOT_BUFFERED_OSTREAM_HPP
#define PLYWOOT_BUFFERED_OSTREAM_HPP

#include "type_traits.hpp"

#include <cstdio>
#include <cstring>
#include <ostream>
#include <type_traits>

namespace {

/// Default buffer size; may need tweaking.
constexpr std::size_t OStreamBufferSize{1024 * 1024};

}

namespace plywoot { namespace detail {

/// Wrapper around some ASCII output stream that provides buffered output
/// functionality. This will always buffer some compile-time given size of bytes
/// up front, and data is written to this buffer first until either the buffer
/// fills up or an instance of this object goes out of scope, at which point all
/// buffered data it is written to the wrapped output stream. This improves
/// writing to file-backed output streams considerably.
class BufferedOStream
{
public:
  /// Constructs a buffered ASCII output stream wrapper around the given output stream.
  explicit BufferedOStream(std::ostream &os) : os_{os} {}

  /// No copy semantics allowed.
  BufferedOStream(const BufferedOStream &) = delete;
  BufferedOStream &operator=(const BufferedOStream &) = delete;

  /// Closes this output stream.
  // TODO(ton): this should be in a destructor; but because this is part of a
  // union, there is a strict requirement for having a trivial destructor.
  // `std::variant` removes this limitation. The cheap variant implementation
  // needs to be revised to clean this mess up.
  void close() { flush(); }

  /// Writes a single character `c` to the output stream.
  void put(char c)
  {
    if (c_ == eob_) flush();
    *c_++ = c;
  }

  /// Writes the given number of characters `n` read from `src` to the output
  /// stream.
  void write(const char *src, std::size_t n)
  {
    if (n >= OStreamBufferSize)
    {
      flush();
      os_.write(src, n);
    }
    else
    {
      if (c_ + n > eob_) { flush(); }
      std::memcpy(c_, src, n);
      c_ += n;
    }
  }

  /// Writes a number of type `T` to the output stream.
  template<typename T>
  void writeAscii(T t)
  {
    if (c_ >= eob_)
    {
      flush();
      c_ += std::snprintf(c_, OStreamBufferSize, detail::formatStr<T>(), t);
    }
    else
    {
      int n = std::snprintf(c_, eob_ - c_, detail::formatStr<T>(), t);
      if (n < 0)
      {
        flush();
        n = std::snprintf(c_, OStreamBufferSize, detail::formatStr<T>(), t);
      }
      c_ += n;
    }
  }

  /// Writes a number of type `T` to the output stream.
  template<typename T>
  void write(T t)
  {
    if (c_ + sizeof(T) > eob_) { flush(); }
    std::memcpy(c_, reinterpret_cast<const char *>(&t), sizeof(T));
    c_ += sizeof(T);
  }

private:
  /// Flushes the output buffer to the underlying output stream.
  void flush()
  {
    os_.write(buffer_, c_ - buffer_);
    c_ = buffer_;
  }

  /// Character the scanner's write head is currently pointing to. Invariant:
  ///
  ///       buffer_ <= c_ < (buffer_ + sizeof(buffer_) - 1)
  ///
  char *c_{buffer_};
  /// Number of bytes remaining in the buffer.
  const char *eob_{buffer_ + OStreamBufferSize};

  /// Reference to the wrapped standard output stream.
  std::ostream &os_;

  /// Buffered data, always a null terminated string.
  alignas(64) char buffer_[OStreamBufferSize] = {};
};

}}

#endif
