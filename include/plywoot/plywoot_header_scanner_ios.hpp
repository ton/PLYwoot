#ifndef PLYWOOT_HEADER_SCANNER_IOS_HPP
#define PLYWOOT_HEADER_SCANNER_IOS_HPP

#include <ostream>

namespace plywoot
{
  inline std::ostream &operator<<(std::ostream &os, HeaderScanner::Token t)
  {
    switch (t)
    {
      case HeaderScanner::Token::Unknown:
        return os << "<unknown>";
      case HeaderScanner::Token::Ascii:
        return os << "ascii";
      case HeaderScanner::Token::BinaryBigEndian:
        return os << "binary_big_endian";
      case HeaderScanner::Token::BinaryLittleEndian:
        return os << "binary_little_endian";
      case HeaderScanner::Token::Char:
        return os << "char";
      case HeaderScanner::Token::Comment:
        return os << "comment";
      case HeaderScanner::Token::Double:
        return os << "double";
      case HeaderScanner::Token::Element:
        return os << "element";
      case HeaderScanner::Token::EndHeader:
        return os << "end_header";
      case HeaderScanner::Token::Eof:
        return os << "<eof>";
      case HeaderScanner::Token::Float:
        return os << "float";
      case HeaderScanner::Token::FloatingPointNumber:
        return os << "<floating point number>";
      case HeaderScanner::Token::Format:
        return os << "format";
      case HeaderScanner::Token::Identifier:
        return os << "<identifier>";
      case HeaderScanner::Token::Int:
        return os << "int";
      case HeaderScanner::Token::List:
        return os << "list";
      case HeaderScanner::Token::Number:
        return os << "<number>";
      case HeaderScanner::Token::MagicNumber:
        return os << "ply";
      case HeaderScanner::Token::Property:
        return os << "property";
      case HeaderScanner::Token::Short:
        return os << "short";
      case HeaderScanner::Token::UChar:
        return os << "uchar";
      case HeaderScanner::Token::UInt:
        return os << "uint";
      case HeaderScanner::Token::UShort:
        return os << "ushort";
    }

    return os;
  }
}

#endif
