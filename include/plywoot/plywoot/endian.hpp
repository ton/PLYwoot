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

#ifndef PLYWOOT_ENDIAN_HPP
#define PLYWOOT_ENDIAN_HPP

#include <cstdint>
#include <utility>

#include <endian.h>

namespace plywoot::detail {

/// Type tag to indicate little endian behavior is required for the
/// parser/writer policies.
struct LittleEndian
{
};

/// Type tag to indicate big endian behavior is required for the parser/writer
/// policies.
struct BigEndian
{
};

/// Naive byte swap functions for floating point numbers.
template<typename T>
T byte_swap(T t)
{
  unsigned char *bytes = reinterpret_cast<unsigned char *>(&t);

  if constexpr (sizeof(T) == 4)
  {
    std::swap(bytes[0], bytes[3]);
    std::swap(bytes[1], bytes[2]);
  }
  else if constexpr (sizeof(T) == 8)
  {
    std::swap(bytes[0], bytes[7]);
    std::swap(bytes[1], bytes[6]);
    std::swap(bytes[2], bytes[5]);
    std::swap(bytes[3], bytes[4]);
  }

  return t;
}

/// Wrappers around the glibc htobe* and be*toh functions that pick the correct
/// call depending on the size of the argument type.
template<typename T>
T htobe(T t)
{
  if constexpr (sizeof(T) == 1) { return t; }
  else if constexpr (std::is_integral_v<T> && sizeof(T) == 2) { return static_cast<T>(htobe16(t)); }
  else if constexpr (std::is_integral_v<T> && sizeof(T) == 4) { return static_cast<T>(htobe32(t)); }
  else if constexpr (std::is_integral_v<T> && sizeof(T) == 8) { return static_cast<T>(htobe64(t)); }
  else if constexpr (std::is_floating_point_v<T>) { return byte_swap(t); }
}

template<typename T>
T betoh(T t)
{
  if constexpr (sizeof(T) == 1) { return t; }
  else if constexpr (std::is_integral_v<T> && sizeof(T) == 2) { return static_cast<T>(be16toh(t)); }
  else if constexpr (std::is_integral_v<T> && sizeof(T) == 4) { return static_cast<T>(be32toh(t)); }
  else if constexpr (std::is_integral_v<T> && sizeof(T) == 8) { return static_cast<T>(be64toh(t)); }
  else if constexpr (std::is_floating_point_v<T>) { return byte_swap(t); }
}

}

#endif
