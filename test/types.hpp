/*
   This file is part of PLYwoot, a header-only PLY parser.

   Copyright (C) 2023-2025, Ton van den Heuvel

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

#ifndef PLYWOOT_TEST_TYPES_HPP
#define PLYWOOT_TEST_TYPES_HPP

#include <ostream>

template<typename T>
struct VertexT
{
  T x, y, z;

  bool operator==(const VertexT &v) const { return x == v.x && y == v.y && z == v.z; }
  bool operator!=(const VertexT &v) const { return !(*this == v); }
};

using FloatVertex = VertexT<float>;
using DoubleVertex = VertexT<double>;

struct Triangle
{
  int a, b, c;

  bool operator==(const Triangle &f) const { return a == f.a && b == f.b && c == f.c; }
};

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const VertexT<T> &v)
{
  return os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
}

inline std::ostream &operator<<(std::ostream &os, const Triangle &t)
{
  return os << '[' << t.a << ", " << t.b << ", " << t.c << ']';
}

#endif
