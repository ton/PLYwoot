#ifndef PLYWOOT_HEADER_PARSER_HPP
#define PLYWOOT_HEADER_PARSER_HPP

#include "plywoot_exceptions.hpp"
#include "plywoot_header_scanner.hpp"
#include "plywoot_header_scanner_ios.hpp"
#include "plywoot_std.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace plywoot
{
  // Base class for all parser exception.
  struct ParserException : Exception
  {
    ParserException(const std::string &message) : Exception("parser error: " + message) {}
  };

  // Exception thrown in case some invalid format specification is found in the
  // input.
  struct InvalidFormat : ParserException
  {
    InvalidFormat(const std::string &format) : ParserException("invalid format found: " + format) {}
  };

  // Exception thrown in case some valid but unsupported format specification is
  // found in the input.
  struct UnsupportedFormat : ParserException
  {
    UnsupportedFormat(const std::string &format)
        : ParserException("unsupported format definition: " + format)
    {
    }
  };

  // Exception thrown in case the input contains an unexpected token.
  struct UnexpectedToken : ParserException
  {
    UnexpectedToken(HeaderScanner::Token expected, HeaderScanner::Token found)
        : ParserException(
              "unexpected token '" + pstd::to_string(found) + "' found, expected '" +
              pstd::to_string(expected) + "' instead"),
          expected_{expected},
          found_{found}
    {
    }

    UnexpectedToken(HeaderScanner::Token expected)
        : UnexpectedToken{expected, HeaderScanner::Token::Eof}
    {
    }

    HeaderScanner::Token expected() const { return expected_; }
    HeaderScanner::Token found() const { return found_; }

  private:
    HeaderScanner::Token expected_;
    HeaderScanner::Token found_;
  };

  // Parser implementation for a PLY header. Results in a list of PLY element
  // specifications.
  class HeaderParser
  {
    using Token = HeaderScanner::Token;

  public:
    HeaderParser(std::istream &is) : scanner_{is}
    {
      accept(Token::MagicNumber);

      // Parse the format section.
      accept(Token::Format);
      switch (scanner_.nextToken())
      {
        case Token::Ascii:
          format_ = PlyFormat::Ascii;
          break;
        case Token::BinaryLittleEndian:
        case Token::BinaryBigEndian:
          throw UnsupportedFormat(scanner_.tokenString());
        default:
          throw InvalidFormat(scanner_.tokenString());
      }
      scanner_.nextToken();  // ignore the format version

      // Ignore comment section for now.
      // TODO(ton): better to store the comments so that we can recreate the
      // original input when writing is implemented.
      while (scanner_.nextToken() == Token::Comment) { scanner_.skipLines(1); }

      // Parse elements.
      do {
        switch (scanner_.token())
        {
          case Token::EndHeader:
            break;
          case Token::Element:
            elements_.push_back(parseElement());
            break;
          default:
            throw UnexpectedToken(scanner_.token());
            break;
        }
      } while (scanner_.token() != Token::EndHeader);
    }

    const std::vector<PlyElement> &elements() const { return elements_; }
    PlyFormat format() const { return format_; }

  private:
    // Asks the scanner for the next token, and verifies that it matches the
    // given expected token. In case it fails, throws `UnexpectedToken`.
    void accept(Token expected)
    {
      if (scanner_.nextToken() != expected) { throw UnexpectedToken(expected, scanner_.token()); }
    }

    // Converts a scanner token type to a data type, in case the token
    // represents a data type. Otherwise, this throws `UnexpectedToken`.
    PlyDataType tokenToDataType(Token t) const
    {
      switch (t)
      {
        case Token::Char:
          return PlyDataType::Char;
        case Token::UChar:
          return PlyDataType::UChar;
        case Token::Short:
          return PlyDataType::Short;
        case Token::UShort:
          return PlyDataType::UShort;
        case Token::Int:
          return PlyDataType::Int;
        case Token::UInt:
          return PlyDataType::UInt;
        case Token::Float:
          return PlyDataType::Float;
        case Token::Double:
          return PlyDataType::Double;
        default:
          throw UnexpectedToken(t);
      }
    }

    // Parses an element definition together with its associated properties.
    PlyElement parseElement()
    {
      accept(Token::Identifier);  // name of the elements
      std::string name{scanner_.tokenString()};

      accept(Token::Number);
      std::size_t size{scanner_.tokenNumber()};

      PlyElement result{std::move(name), size, {}};
      while (scanner_.nextToken() == Token::Property)
      {
        PlyProperty property;

        switch (scanner_.nextToken())
        {
          case Token::List:
            property.isList = true;
            property.sizeType = tokenToDataType(scanner_.nextToken());
            property.type = tokenToDataType(scanner_.nextToken());
            break;
          default:
            property.type = tokenToDataType(scanner_.token());
            break;
        }

        // TODO(ton): probably reserved keywords may be used as names as well,
        // just accept every token here, even numbers?
        accept(Token::Identifier);
        property.name = scanner_.tokenString();

        result.properties.push_back(std::move(property));
      }

      return result;
    }

    // Format the data is stored in.
    PlyFormat format_;
    // PLY elements defined in the header.
    std::vector<PlyElement> elements_;
    // Reference to the wrapped input stream.
    HeaderScanner scanner_;
  };
}

#endif
