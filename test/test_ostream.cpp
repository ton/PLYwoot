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

#include "types.hpp"

#include <catch2/catch_test_macros.hpp>
#include <plywoot/plywoot.hpp>

#include <fstream>
#include <numeric>
#include <sstream>

TEST_CASE("Write empty PLY file", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
  plyos.write(ss);

  const std::string expected{"ply\nformat ascii 1.0\nend_header\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write empty PLY file", "[ostream][binary-little-endian]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::BinaryLittleEndian};
  plyos.write(ss);

  const std::string expected{"ply\nformat binary_little_endian 1.0\nend_header\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file with a single element and a single property", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"e", 3, {f}};

  using Layout = plywoot::reflect::Layout<int>;

  std::vector<int> values{1, 4, 7};
  plyos.add(element, Layout{values});
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element e 3\n"
      "property float f\n"
      "end_header\n"
      "1\n"
      "4\n"
      "7\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file with a single element and some properties", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Double};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Double};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Double};
  const plywoot::PlyElement element{"vertex", 3, {x, y, z}};

  using Layout = plywoot::reflect::Layout<double, double, double>;
  using Vertex = DoubleVertex;

  std::vector<Vertex> vertices{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  plyos.add(element, Layout{vertices});
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element vertex 3\n"
      "property double x\n"
      "property double y\n"
      "property double z\n"
      "end_header\n"
      "1 2 3\n"
      "4 5 6\n"
      "7 8 9\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE(
    "Test writing an element with less properties than defined in the memory layout",
    "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"e", 3, {f}};

  using Layout = plywoot::reflect::Layout<int, float, double, std::string>;

  struct MyPair
  {
    int i;
    float f;
    double d;
    std::string s;
  };

  std::vector<MyPair> values{{1, 3.0, 0.0, "skip"}, {4, 86.0, 0.0, "this"}, {7, 42.0, 0.0, "please"}};
  plyos.add(element, Layout{values});
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element e 3\n"
      "property float f\n"
      "end_header\n"
      "1\n"
      "4\n"
      "7\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE(
    "Test writing an element with more properties than defined in the memory layout",
    "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty g{"g", plywoot::PlyDataType::Double, plywoot::PlyDataType::Char};
  const plywoot::PlyProperty h{"h", plywoot::PlyDataType::Int};
  const plywoot::PlyElement element{"e", 3, {f, g, h}};

  using Layout = plywoot::reflect::Layout<int>;

  const std::vector<int> values{1, 4, 7};
  plyos.add(element, Layout{values});
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element e 3\n"
      "property float f\n"
      "property list char double g\n"
      "property int h\n"
      "end_header\n"
      "1 0 0\n"
      "4 0 0\n"
      "7 0 0\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file with a single element with a list property", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const auto sizeType = plywoot::PlyDataType::Char;

  const plywoot::PlyProperty faceIndices{"vertex_indices", plywoot::PlyDataType::Int, sizeType};
  const plywoot::PlyElement element{"face", 10, {faceIndices}};

  using Layout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  std::vector<Triangle> triangles;
  plyos.add(element, Layout{triangles});
  plyos.write(ss);

  const std::string expected{
      "ply\nformat ascii 1.0\nelement face 0\nproperty list char int vertex_indices\nend_header\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file containing a single vertex using a pack of floats", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"vertex", 1, {x, y, z}};

  using Layout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 3>>;
  using Vertex = FloatVertex;

  std::vector<Vertex> vertices = {Vertex{1, 2, 3}};
  plyos.add(element, Layout{vertices});
  plyos.write(ss);

  const std::string expected{
      "ply\nformat ascii 1.0\nelement vertex 1\nproperty float x\nproperty float y\nproperty float "
      "z\nend_header\n"
      "1 2 3\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE(
    "Write PLY file containing a single vertex using a pack of floats, followed by another property",
    "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"vertex", 1, {x, y, z}};

  using Layout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 2>, float>;
  using Vertex = FloatVertex;

  std::vector<Vertex> vertices = {Vertex{1, 2, 3}};
  plyos.add(element, Layout{vertices});
  plyos.write(ss);

  const std::string expected{
      "ply\nformat ascii 1.0\nelement vertex 1\nproperty float x\nproperty float y\nproperty float "
      "z\nend_header\n"
      "1 2 3\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE(
    "Write PLY file containing a single vertex using a pack of floats, and a face using an array of "
    "integers, followed by another property",
    "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Float};
  const plywoot::PlyElement vertexElement{"vertex", 1, {x, y, z}};

  using VertexLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 3>>;
  using Vertex = FloatVertex;

  const plywoot::PlyProperty vertexIndices{
      "vertex_indices", plywoot::PlyDataType::Int, plywoot::PlyDataType::UInt};
  const plywoot::PlyProperty nx{"nx", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty ny{"ny", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty nz{"nz", plywoot::PlyDataType::Float};
  const plywoot::PlyElement faceElement{"face", 1, {vertexIndices, nx, ny, nz}};

  std::vector<Vertex> vertices = {Vertex{1, 2, 3}};
  std::vector<Triangle> triangles = {Triangle{4, 5, 6}, Triangle{7, 8, 9}};

  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  plyos.add(vertexElement, VertexLayout{vertices});
  plyos.add(faceElement, TriangleLayout{triangles});
  plyos.write(ss);

  const std::string expected{R"(ply
format ascii 1.0
element vertex 1
property float x
property float y
property float z
element face 2
property list uint int vertex_indices
property float nx
property float ny
property float nz
end_header
1 2 3
3 4 5 6 0 0 0
3 7 8 9 0 0 0
)"};

  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file containing a list with zero elements", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty p{"l", plywoot::PlyDataType::Float, plywoot::PlyDataType::UChar};
  const plywoot::PlyElement element{"e", 1, {p}};

  using Floats = std::vector<float>;
  using Layout = plywoot::reflect::Layout<Floats>;

  std::vector<Floats> elements(1);
  plyos.add(element, Layout{elements});
  plyos.write(ss);

  const std::string expected{R"(ply
format ascii 1.0
element e 1
property list uchar float l
end_header
0
)"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write multiple elements with tricky alignment properties", "[istream]")
{
  struct X
  {
    char c{0};
    std::vector<int> v;
    char d{0};

    inline bool operator==(const X &x) const { return c == x.c && v == x.v && d == x.d; }
  };

  const std::string expected{R"(ply
format ascii 1.0
element e 5
property char c
property list uchar int v
property char d
end_header
86 0 87
88 1 1 89
90 2 1 2 91
92 3 1 2 3 93
94 4 1 2 3 4 95
)"};

  {
    // Perform writing data using the reflection route.
    const plywoot::PlyProperty c{"c", plywoot::PlyDataType::Char};
    const plywoot::PlyProperty v{"v", plywoot::PlyDataType::Int, plywoot::PlyDataType::UChar};
    const plywoot::PlyProperty d{"d", plywoot::PlyDataType::Char};
    const plywoot::PlyElement element{"e", 5, {c, v, d}};

    using Layout = plywoot::reflect::Layout<char, std::vector<int>, char>;

    const std::vector<X> elements = {
        {86, {}, 87}, {88, {1}, 89}, {90, {1, 2}, 91}, {92, {1, 2, 3}, 93}, {94, {1, 2, 3, 4}, 95}};

    std::stringstream ss;
    plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
    plyos.add(element, Layout{elements});
    plyos.write(ss);

    REQUIRE(expected == ss.str());
  }

  {
    // Perform writing data using a `PlyElementData` instance as well, to verify
    // proper construction and destruction of a `PlyElementData` instance.
    std::stringstream iss{expected};

    const plywoot::IStream plyIs{iss};
    REQUIRE(plyIs.elements().size() == 1);

    const plywoot::PlyElementData elementData = plyIs.readElement();

    std::stringstream oss;
    plywoot::OStream plyOs{plywoot::PlyFormat::Ascii};
    plyOs.add(elementData);
    plyOs.write(oss);

    REQUIRE(expected == oss.str());
  }
}
