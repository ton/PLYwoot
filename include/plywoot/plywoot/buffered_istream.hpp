#ifndef PLYWOOT_BUFFERED_ISTREAM_HPP
#define PLYWOOT_BUFFERED_ISTREAM_HPP

#include <algorithm>
#include <cassert>
#include <cstring>
#include <istream>

namespace plywoot { namespace detail {

/// TODO
class BufferedIStream
{
public:
  explicit BufferedIStream(std::istream &is) : is_{is}, headerOffset_{is_.tellg()} {}

  bool eof() const { return *c_ == EOF; }

  const char *data() const { return c_; }
  const char *&data() { return c_; }

  void seekToBegin()
  {
    // Position the read head at the start of the data just after the header.
    // Resets any buffered data.
    c_ = buffer_;

    is_.clear();  // need to clear eofbit() in case it is set, otherwise the
                  // first read after the seek will fail
    is_.seekg(headerOffset_);
  }

  void skip(std::size_t n)
  {
    const std::size_t remaining = (buffer_ + BufferSize) - c_;
    if (remaining <= n)
    {
      buffer();
      n -= remaining;
    }

    // TODO(ton): this can be implemented much more efficiently by just seeking
    // the underlying stream and buffer from the right offset.
    while (n >= BufferSize)
    {
      buffer();
      n -= BufferSize;
    }

    c_ += n;
  }

  /// Skips `n` lines in the input, places the read head at the first
  /// character after the `n`-th newline character that as found in the input,
  /// or in case that character does not exist, at EOF.
  void skipLines(std::size_t n)
  {
    while (*c_ != EOF && n > 0)
    {
      c_ = static_cast<const char *>(std::memchr(c_, '\n', (buffer_ + BufferSize) - c_));
      if (c_)
      {
        readCharacter();
        --n;
      }
      else { buffer(); }
    }
  }

  inline void skipWhitespace()
  {
    while (0 <= *c_ && *c_ <= 0x20) { readCharacter(); }
  }

  inline void skipNonWhitespace()
  {
    while (*c_ > 0x20) readCharacter();
  }

  /// Reads the next character in the input stream and advances the read head by
  /// one character.
  inline void readCharacter()
  {
    ++c_;
    if (c_ >= buffer_ + BufferSize) { buffer(); }
  }

  /// Ensures that the buffer contains at least the given number of characters.
  /// In case it already does, this does nothing, otherwise, it will shift the
  /// data remaining in the buffer to the front, then refill the remaining part
  /// of the buffer.
  void bufferData(size_t minimum)
  {
    assert(minimum < BufferSize / 2);
    const size_t remaining = (buffer_ + BufferSize - c_);
    if (remaining >= minimum) { return; }
    else
    {
      std::memcpy(buffer_, c_, remaining);
      if (!is_.read(buffer_ + remaining, BufferSize - remaining))
      {
        // In case the buffer is only partially filled, fill the remainder with
        // EOF characters.
        auto bufferedBytes = is_.gcount() + remaining;
        std::fill_n(buffer_ + bufferedBytes, BufferSize - bufferedBytes, EOF);
      }

      c_ = buffer_;
    }
  }

private:
  /// Unconditionally buffers data from the input stream.
  void buffer()
  {
    if (!is_.read(buffer_, BufferSize))
    {
      // In case the buffer is only partially filled, fill the remainder with
      // EOF characters.
      auto bufferedBytes = is_.gcount();
      std::fill_n(buffer_ + bufferedBytes, BufferSize - bufferedBytes, EOF);
    }

    c_ = buffer_;
  }

  std::istream &is_;
  std::istream::pos_type headerOffset_;

  constexpr static size_t BufferSize{8192};
  /// Buffered data, always a null terminated string.
  char buffer_[BufferSize] = {};
  /// Character the scanner's read head is currently pointing to. Invariant
  /// after construction of the scanner is:
  ///
  ///       buffer_ <= c_ < (buffer_ + sizeof(buffer_) - 1)
  ///
  /// Note that the invariant allows for one character lookahead without the
  /// need to check whether we need to read data from disk.
  const char *c_{buffer_ + BufferSize};
};

}}

#endif