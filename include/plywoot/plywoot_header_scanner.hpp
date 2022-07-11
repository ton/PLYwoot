#ifndef PLYWOOT_HEADER_SCANNER_HPP
#define PLYWOOT_HEADER_SCANNER_HPP

#include "plywoot_std.hpp"

#include <cstdint>
#include <istream>
#include <string>

namespace plywoot
{
  static constexpr const char endHeaderToken[] = "end_header";

  // Lookup table to check whether a character is a token delimiter.
  // The following characters are token delimiters:
  //
  //  1. <space> (32)
  //  2. \t      (9)
  //  3. \n      (10)
  //  4. \r      (13)
  //  5. EOF     (255)
  //
  // clang-format off
  constexpr bool isTokenDelimiter[256] = {
    // 0      1      2      3      4      5      6      7      8      9     10     11     12     13     14     15
    false, false, false, false, false, false, false, false, false, true,  true,  false, false, true,  false, false, // 0   - 15
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 16  - 31
    true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 32  - 47
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 48  - 63
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 64  - 79
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 80  - 95
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 96  - 111
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 112 - 127
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 128 - 143
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 144 - 159
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 160 - 175
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 176 - 191
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 192 - 207
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 208 - 223
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, // 224 - 239
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true , // 240 - 255
  };

  class HeaderScanner
  {
  public:
    HeaderScanner(std::istream &is) : is_{is}
    {
      std::string line;
      while (bool(std::getline(is, line)) && line != endHeaderToken)
      {
        buffer_.append(line);
        buffer_.push_back('\n');
      }

      if (line == endHeaderToken)
      {
        buffer_.append(line);
        buffer_.push_back('\n');
      }
      else
      {
        buffer_.push_back(EOF);
      }

      // Initialize the read head to the start of the buffered data.
      c_ = buffer_.data();
    }

    // Enumeration of all PLY header token types.
    enum class Token
    {
      Unknown = 0,
      Ascii,
      BinaryBigEndian,
      BinaryLittleEndian,
      Char,
      Comment,
      Double,
      Element,
      EndHeader,
      Eof,
      Float,
      FloatingPointNumber,
      Format,
      Identifier,
      Int,
      List,
      MagicNumber,
      Number,
      Property,
      Short,
      UChar,
      UInt,
      UShort,
    };

    /// Skips all input data and puts the read head just after the n-th newline
    /// character it encounters, or at EOF in case no such newline character is
    /// present in the input stream.
    void skipLines(std::size_t n)
    {
      const char *last{buffer_.data() + buffer_.size()};
      while (c_ && n > 0)
      {
        c_ = static_cast<const char *>(std::memchr(c_, '\n', (last - c_)));
        --n;
      }
    }

    /// Returns the next token type in the input stream.
    Token nextToken() noexcept
    {
      // Ignore all whitespace, read upto the first non-whitespace character.
      const char *last = buffer_.data() + buffer_.size();
      while (c_ < last && 0 <= *c_ && *c_ <= 0x20)
      {
        ++c_;
      }

      // Read an identifier. After an identifier is read, the read head is
      // positioned at the start of the next token or whitespace.
      const char *tokenStart = c_;
      while (!isTokenDelimiter[(unsigned char)*c_])
      {
        c_++;
      }
      tokenString_ = pstd::string_view(tokenStart, c_);

      // In case the identifier is one of the reserved keywords, handle it as
      // such. Use first character for quick comparison.
      switch (tokenString_.front())
      {
        case 'a':
          token_ = !tokenString_.compare("ascii") ? Token::Ascii : Token::Identifier;
          break;
        case 'b':
          if (!tokenString_.compare("binary_big_endian")) token_ = Token::BinaryBigEndian;
          else if (!tokenString_.compare("binary_little_endian")) token_ = Token::BinaryLittleEndian;
          else token_ = Token::Identifier;
          break;
        case 'c':
          if (!tokenString_.compare("char")) { token_ = Token::Char; }
          else if (!tokenString_.compare("comment")) { token_ = Token::Comment; readComment(); }
          else { token_ = Token::Identifier; }
          break;
        case 'd':
          token_ = (!tokenString_.compare("double") ? Token::Double : Token::Identifier);
          break;
        case 'e':
          if (!tokenString_.compare("element")) token_ = Token::Element;
          else if (!tokenString_.compare("end_header")) token_ = Token::EndHeader;
          else token_ = Token::Identifier;
          break;
        case 'f':
          if (!tokenString_.compare("format")) token_ = Token::Format;
          else if (!tokenString_.compare("float")) token_ = Token::Float;
          else token_ = Token::Identifier;
          break;
        case 'l':
          token_ = (!tokenString_.compare("list") ? Token::List : Token::Identifier);
          break;
        case 'i':
          token_ = (!tokenString_.compare("int") ? Token::Int : Token::Identifier);
          break;
        case 'p':
          if (!tokenString_.compare("ply")) token_ = Token::MagicNumber;
          else if (!tokenString_.compare("property")) token_ = Token::Property;
          else token_ = Token::Identifier;
          break;
        case 's':
          token_ = (!tokenString_.compare("short") ? Token::Short : Token::Identifier);
          break;
        case 'u':
          if (!tokenString_.compare("uchar")) token_ = Token::UChar;
          else if (!tokenString_.compare("uint")) token_ = Token::UInt;
          else if (!tokenString_.compare("ushort")) token_ = Token::UShort;
          else token_ = Token::Identifier;
          break;
        case '-':
        case '+':
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          token_ = (tokenString_.find('.') != std::string::npos) ? Token::FloatingPointNumber : Token::Number;
          break;
        case EOF:
          token_ = Token::Eof;
          break;
        default:
          token_ = Token::Identifier;
          break;
      }

      return token_;
    }

    // Returns the most recently scanned token.
    Token token() const noexcept { return token_; }
    // Converts the current token string to a number of the given type.
    std::size_t tokenNumber() const noexcept
    {
      return static_cast<std::size_t>(std::strtoull(tokenString_.data(), nullptr, 10));
    }
    // Returns the string representation of the current token.
    std::string tokenString() const noexcept
    {
      return std::string(tokenString_.data(), tokenString_.size());
    }

  private:
    // Reads the remainder of the line as a comment. The comment itself is set
    // as the token string.
    void readComment()
    {
      const std::size_t remainingBytes = buffer_.size() - (c_ - buffer_.data());
      const char *last = static_cast<const char*>(::memchr(c_, '\n', remainingBytes));
      if (last != nullptr)
      {
        tokenString_ = pstd::string_view(c_, last);
      }
    }

    // Buffered data, always a null terminated string.
    std::string buffer_;
    // Character the scanner's read head is currently pointing to. Invariant
    // after construction of the scanner is:
    //
    //       buffer_.data() <= c_ < (buffer_.data() + buffer_.size())
    //
    const char* c_{buffer_.data()};

    // Most recently scanned token.
    Token token_{Token::Unknown};
    // String representation of the current token in case it is not predefined.
    pstd::string_view tokenString_;

    // Reference to the wrapped input stream.
    std::istream &is_;
  };
}

#endif
