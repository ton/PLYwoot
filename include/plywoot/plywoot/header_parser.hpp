#ifndef PLYWOOT_HEADER_PARSER_HPP
#define PLYWOOT_HEADER_PARSER_HPP

#include "exceptions.hpp"
#include "header_scanner.hpp"
#include "header_scanner_ios.hpp"
#include "std.hpp"
#include "types.hpp"

#include <cassert>
#include <cstdint>
#include <istream>
#include <string>
#include <utility>
#include <vector>

namespace plywoot {

/// Base class for all header parser exceptions.
struct HeaderParserException : Exception
{
  HeaderParserException(const std::string &message) : Exception("parser error: " + message) {}
};

/// Exception thrown in case some invalid format specification is found in the
/// input.
struct InvalidFormat : HeaderParserException
{
  InvalidFormat(const std::string &format) : HeaderParserException("invalid format found: " + format) {}
};

/// Exception thrown in case the input contains an unexpected token.
struct UnexpectedToken : HeaderParserException
{
  UnexpectedToken(
      detail::HeaderScanner::Token expected,
      detail::HeaderScanner::Token found,
      const std::string &tokenString)
      : HeaderParserException(
            "unexpected token '" + detail::to_string(found) + "' found, expected '" +
            detail::to_string(expected) + "' (=" + tokenString + ") instead"),
        expected_{expected},
        found_{found}
  {
  }

  UnexpectedToken(detail::HeaderScanner::Token found, const std::string &tokenString)
      : UnexpectedToken{detail::HeaderScanner::Token::Eof, found, tokenString}
  {
  }

  std::string expected() const { return detail::to_string(expected_); }
  std::string found() const { return detail::to_string(found_); }

private:
  detail::HeaderScanner::Token expected_;
  detail::HeaderScanner::Token found_;
};

}

namespace plywoot { namespace detail {

/// Parser implementation for a PLY header. Results in a list of PLY element
/// specifications.
class HeaderParser
{
  using Token = detail::HeaderScanner::Token;

public:
  HeaderParser(std::istream &is) : scanner_{is}
  {
    assert(is.good());

    accept(Token::MagicNumber);

    // Parse the format section.
    accept(Token::Format);
    switch (scanner_.nextToken())
    {
      case Token::Ascii:
        format_ = PlyFormat::Ascii;
        break;
      case Token::BinaryLittleEndian:
        format_ = PlyFormat::BinaryLittleEndian;
        break;
      case Token::BinaryBigEndian:
        format_ = PlyFormat::BinaryBigEndian;
        break;
      default:
        throw InvalidFormat(scanner_.tokenString());
    }
    scanner_.nextToken();  // ignore the format version

    // Ignore comment section for now.
    while (scanner_.nextToken() == Token::Comment)
    {
      comments_.push_back(scanner_.comment());
    }

    // Parse elements.
    do {
      switch (scanner_.token())
      {
        case Token::EndHeader:
          break;
        case Token::Element:
          elements_.push_back(parseElement());
          break;
        case Token::Comment:
          comments_.push_back(scanner_.comment());
          scanner_.nextToken();
          break;
        default:
          throw UnexpectedToken(scanner_.token(), scanner_.tokenString());
          break;
      }
    } while (scanner_.token() != Token::EndHeader);
  }

  const std::vector<Comment> &comments() const { return comments_; }
  const std::vector<PlyElement> &elements() const { return elements_; }
  PlyFormat format() const { return format_; }

private:
  /// Asks the scanner for the next token, and verifies that it matches the
  /// given expected token. In case it fails, throws `UnexpectedToken`.
  void accept(Token expected)
  {
    if (scanner_.nextToken() != expected)
    {
      throw UnexpectedToken(expected, scanner_.token(), scanner_.tokenString());
    }
  }

  /// Converts a scanner token type to a data type, in case the token
  /// represents a data type. Otherwise, this throws `UnexpectedToken`.
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
        // TODO(ton): expected is a range of tokens...
        throw UnexpectedToken(Token::Char, t, scanner_.tokenString());
    }
  }

  /// Parses an element definition together with its associated properties.
  PlyElement parseElement()
  {
    accept(Token::Identifier);  // name of the elements
    std::string name{scanner_.tokenString()};

    accept(Token::Number);
    std::size_t size{scanner_.tokenNumber()};

    PlyElement result{std::move(name), size};

    // Parse properties.
    while (scanner_.nextToken() == Token::Property || scanner_.token() == Token::Comment)
    {
      if (scanner_.token() == Token::Property)
      {
        PlyDataType type;
        PlyDataType sizeType;

        // TODO(ton): probably reserved keywords may be used as names as well,
        // just accept every token here, even numbers?
        switch (scanner_.nextToken())
        {
          case Token::List:
            sizeType = tokenToDataType(scanner_.nextToken());
            type = tokenToDataType(scanner_.nextToken());
            accept(Token::Identifier);
            result.addProperty(scanner_.tokenString(), type, sizeType);
            break;
          default:
            type = tokenToDataType(scanner_.token());
            accept(Token::Identifier);
            result.addProperty(scanner_.tokenString(), type);
            break;
        }
      }
      else
      {
        assert(scanner_.token() == Token::Comment);
        comments_.push_back(scanner_.comment());
      }
    }

    return result;
  }

  /// Comment line in the header.
  std::vector<Comment> comments_;
  /// Format the data is stored in.
  PlyFormat format_;
  /// PLY elements defined in the header.
  std::vector<PlyElement> elements_;
  /// Reference to the wrapped input stream.
  detail::HeaderScanner scanner_;
};

}}

#endif
