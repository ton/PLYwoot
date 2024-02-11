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

#ifndef PLYWOOT_PARSER_VARIANT_HPP
#define PLYWOOT_PARSER_VARIANT_HPP

#include "ascii_parser_policy.hpp"
#include "binary_parser_policy.hpp"
#include "parser.hpp"

#include <istream>

namespace plywoot { namespace detail {

/// For lack of `std::variant`, create a cheap tagged union.
class ParserVariant
{
public:
  ParserVariant(std::istream &is, PlyFormat format) : format_{format}, variant_{is, format} {}

  ParserVariant(const ParserVariant &) = delete;
  ParserVariant &operator=(const ParserVariant &) = delete;

  ~ParserVariant()
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.~Parser<detail::AsciiParserPolicy>();
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.~Parser<detail::BinaryBigEndianParserPolicy>();
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.~Parser<detail::BinaryLittleEndianParserPolicy>();
        break;
    }
  }

  PlyElementData read(const PlyElement &element) const
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        return variant_.ascii.read(element);
        break;
      case PlyFormat::BinaryBigEndian:
        return variant_.bbe.read(element);
        break;
      case PlyFormat::BinaryLittleEndian:
        return variant_.ble.read(element);
        break;
    }

    return {};
  }

  template<typename... Ts>
  void read(const PlyElement &element, reflect::Layout<Ts...> layout) const
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.read(element, std::move(layout));
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.read(element, std::move(layout));
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.read(element, std::move(layout));
        break;
    }
  }

  void skip(const PlyElement &element) const
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.skip(element);
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.skip(element);
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.skip(element);
        break;
    }
  }

private:
  PlyFormat format_;

  union U {
    U(std::istream &is, PlyFormat format)
    {
      switch (format)
      {
        case PlyFormat::Ascii:
          new (&ascii) detail::Parser<detail::AsciiParserPolicy>{is};
          break;
        case PlyFormat::BinaryBigEndian:
          new (&bbe) detail::Parser<detail::BinaryBigEndianParserPolicy>{is};
          break;
        case PlyFormat::BinaryLittleEndian:
          new (&ble) detail::Parser<detail::BinaryLittleEndianParserPolicy>{is};
          break;
      }
    }

    detail::Parser<detail::AsciiParserPolicy> ascii;
    detail::Parser<detail::BinaryBigEndianParserPolicy> bbe;
    detail::Parser<detail::BinaryLittleEndianParserPolicy> ble;
  } variant_;
};

}}

#endif
