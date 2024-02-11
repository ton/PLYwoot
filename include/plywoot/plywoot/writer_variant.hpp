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

#ifndef PLYWOOT_WRITER_VARIANT_HPP
#define PLYWOOT_WRITER_VARIANT_HPP

#include "ascii_writer_policy.hpp"
#include "binary_writer_policy.hpp"
#include "writer.hpp"

#include <ostream>

namespace plywoot { namespace detail {

/// For lack of `std::variant`, create a cheap tagged union.
class WriterVariant
{
public:
  WriterVariant(std::ostream &os, PlyFormat format) : format_{format}, variant_{os, format} {}

  WriterVariant(const WriterVariant &) = delete;
  WriterVariant &operator=(const WriterVariant &) = delete;

  ~WriterVariant()
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.close();
        variant_.ascii.~Writer<detail::AsciiWriterPolicy>();
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.close();
        variant_.bbe.~Writer<detail::BinaryBigEndianWriterPolicy>();
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.close();
        variant_.ble.~Writer<detail::BinaryLittleEndianWriterPolicy>();
        break;
    }
  }

  void write(const PlyElement &element, const std::uint8_t *src, std::size_t alignment) const
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.write(element, src, alignment);
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.write(element, src, alignment);
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.write(element, src, alignment);
        break;
    }
  }

  template<typename... Ts>
  void write(const PlyElement &element, const reflect::Layout<Ts...> layout) const
  {
    switch (format_)
    {
      case PlyFormat::Ascii:
        variant_.ascii.write(element, layout);
        break;
      case PlyFormat::BinaryBigEndian:
        variant_.bbe.write(element, layout);
        break;
      case PlyFormat::BinaryLittleEndian:
        variant_.ble.write(element, layout);
        break;
    }
  }

private:
  PlyFormat format_;

  union U {
    U(std::ostream &os, PlyFormat format)
    {
      switch (format)
      {
        case PlyFormat::Ascii:
          new (&ascii) detail::Writer<detail::AsciiWriterPolicy>{os};
          break;
        case PlyFormat::BinaryBigEndian:
          new (&bbe) detail::Writer<detail::BinaryBigEndianWriterPolicy>{os};
          break;
        case PlyFormat::BinaryLittleEndian:
          new (&ble) detail::Writer<detail::BinaryLittleEndianWriterPolicy>{os};
          break;
      }
    }

    detail::Writer<detail::AsciiWriterPolicy> ascii;
    detail::Writer<detail::BinaryBigEndianWriterPolicy> bbe;
    detail::Writer<detail::BinaryLittleEndianWriterPolicy> ble;
  } variant_;
};

}}

#endif
