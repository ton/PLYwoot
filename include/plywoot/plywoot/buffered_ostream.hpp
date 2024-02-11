/*
   This file is part of PLYwoot, a header-only PLY parser.

   Copyright (C) 2023-2024, Ton van den Heuvel

   PLYwoot is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
    *c_++ = c;
    if (c_ == eob_) flush();
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
      if (c_ + n >= eob_) { flush(); }
      std::memcpy(c_, src, n);
      c_ += n;
    }
  }

  /// Writes a number of type `T` to the output stream.
  template<typename T>
  void writeAscii(T t)
  {
    constexpr int MIN_BUFFER_SIZE = 100;
    // TODO(ton): use something like Grisu3 instead of `std::snprintf` to
    // (greatly) improve performance.
    if (c_ + MIN_BUFFER_SIZE >= eob_) { flush(); }
    c_ += std::snprintf(c_, MIN_BUFFER_SIZE, detail::formatStr<T>(), t);
  }

  /// Writes a number of type `T` to the output stream.
  template<typename T>
  void write(T t)
  {
    if (c_ + sizeof(T) >= eob_) { flush(); }
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
  ///       buffer_ <= c_ < (buffer_ + sizeof(buffer_))
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
