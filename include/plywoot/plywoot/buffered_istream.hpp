#ifndef PLYWOOT_BUFFERED_ISTREAM_HPP
#define PLYWOOT_BUFFERED_ISTREAM_HPP

#include <algorithm>
#include <cassert>
#include <cstring>
#include <istream>

namespace {

/// Default buffer size; may need tweaking.
constexpr std::size_t BufferSize{1024 * 1024};

}

namespace plywoot { namespace detail {

/// Wrapper around some input stream that provides buffered input functionality.
/// This will always buffer some compile-time given size of bytes up front, and
/// data is read from this buffer until the buffer is exhausted, at which point
/// it is refilled again with the next block of data from the wrapped input
/// stream. This improves reading from file-backed input streams considerably.
class BufferedIStream
{
public:
  /// Constructs a buffered input stream wrapper around the given input stream.
  explicit BufferedIStream(std::istream &is) : is_{is}, initialOffset_{is_.tellg()} {}

  /// No copy semantics allowed.
  BufferedIStream(const BufferedIStream &) = delete;
  BufferedIStream &operator=(const BufferedIStream &) = delete;

  /// Returns whether the read head is at the end of the stream.
  bool eof() const { return *c_ == EOF; }

  /// Returns a raw character pointer representing the current read head in the
  /// stream.
  /// @{
  const char *data() const { return c_; }
  const char *&data() { return c_; }
  /// @}

  /// Reads an object of the given type from the input data stream.
  template<typename T>
  inline T read()
  {
    // Note; buffer a bit more than strictly necessary, so that we can move the
    // read head unconditionally after reading the object of type T.
    if (remaining_ < sizeof(T)) buffer(sizeof(T));
    const char *t = c_;
    remaining_ -= sizeof(T);
    c_ += sizeof(T);
    return *reinterpret_cast<const T *>(t);
  }

  /// Reads the next character in the input stream and advances the read head by
  /// one character.
  inline void readCharacter()
  {
    ++c_;
    --remaining_;
    if (remaining_ == 0) { buffer(); }
  }

  /// Skips the given number of bytes in the input stream.
  void skip(std::size_t n)
  {
    if (remaining_ > n)
    {
      c_ += n;
      remaining_ -= n;
    }
    else
    {
      is_.seekg(n - remaining_, std::ios_base::cur);
      buffer();
    }
  }

  /// Skips `n` lines in the input, places the read head at the first
  /// character after the `n`-th newline character that as found in the input,
  /// or in case that character does not exist, at EOF.
  void skipLines(std::size_t n)
  {
    while (*c_ != EOF && n > 0)
    {
      c_ = static_cast<const char *>(std::memchr(c_, '\n', remaining_));
      if (c_)
      {
        remaining_ = eob_ - c_;
        readCharacter();
        --n;
      }
      else { buffer(); }
    }
  }

  /// Skips whitespace in the input stream, and positions the read head on the
  /// first non-whitespace character in the input stream relative from the
  /// current read head.
  inline void skipWhitespace()
  {
    while (0 <= *c_ && *c_ <= 0x20) { readCharacter(); }
  }

  /// Skips non-whitespace in the input stream, and positions the read head on
  /// the first whitespace character in the input stream relative from the
  /// current read head.
  inline void skipNonWhitespace()
  {
    while (*c_ > 0x20) readCharacter();
  }

  // Positions the read head at the start of the data just after the header.
  // Resets any buffered data.
  void seekToBegin()
  {
    // Need to clear eofbit() in case it is set,
    // otherwise the first read after the seek will fail.
    is_.clear();
    is_.seekg(initialOffset_);

    buffer();
  }

  /// Ensures that the buffer contains at least the given number of characters.
  /// In case it already does, this does nothing, otherwise, it will shift the
  /// data remaining in the buffer to the front, then refill the remaining part
  /// of the buffer.
  void buffer(std::size_t minimum)
  {
    assert(minimum < BufferSize / 2);
    if (remaining_ < minimum)
    {
      std::memcpy(buffer_, c_, remaining_);
      if (!is_.read(buffer_ + remaining_, BufferSize - remaining_))
      {
        // In case the buffer is only partially filled, fill the remainder with
        // EOF characters.
        remaining_ = is_.gcount() + remaining_;
        std::fill_n(buffer_ + remaining_, BufferSize - remaining_, EOF);
      }
      else
      {
        remaining_ = BufferSize;
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
      remaining_ = is_.gcount();
      std::fill_n(buffer_ + remaining_, BufferSize - remaining_, EOF);
    }
    else
    {
      remaining_ = BufferSize;
    }

    c_ = buffer_;
  }

  /// Character the scanner's read head is currently pointing to. Invariant
  /// after construction of the scanner is:
  ///
  ///       buffer_ <= c_ < (buffer_ + sizeof(buffer_) - 1)
  ///
  /// Note that the invariant allows for one character lookahead without the
  /// need to check whether we need to read data from disk.
  const char *c_{buffer_ + BufferSize};
  /// End-of-buffer pointer.
  const char *eob_{c_};
  /// Number of bytes remaining in the buffer.
  std::size_t remaining_{0};

  /// Reference to the wrapper standard input stream.
  std::istream &is_;
  /// Initial offset in the input stream at the time of construction of this
  /// buffered stream.
  const std::istream::pos_type initialOffset_;

  /// Buffered data, always a null terminated string.
  alignas(64) char buffer_[BufferSize] = {};
};

}}

#endif
