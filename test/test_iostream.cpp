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

#include "types.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <plywoot/plywoot.hpp>

#include <fstream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string readAll(const std::string &filename)
{
  std::ifstream ifs{filename};

  ifs.seekg(0, std::ios_base::end);
  const auto size = ifs.tellg();
  ifs.seekg(0, std::ios_base::beg);

  std::string text(size, '\0');
  ifs.read(&text[0], size);

  return text;
}

}

TEST_CASE("Test reading and writing all property types", "[iostream]")
{
  auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  const plywoot::PlyProperty a{"a", plywoot::PlyDataType::Char};
  const plywoot::PlyProperty b{"b", plywoot::PlyDataType::UChar};
  const plywoot::PlyProperty c{"c", plywoot::PlyDataType::Short};
  const plywoot::PlyProperty d{"d", plywoot::PlyDataType::UShort};
  const plywoot::PlyProperty e{"e", plywoot::PlyDataType::Int};
  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::UInt};
  const plywoot::PlyProperty g{"g", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty h{"h", plywoot::PlyDataType::Double};
  const plywoot::PlyElement element{"e", 1, {a, b, c, d, e, f, g, h}};

  struct Element
  {
    char a;
    unsigned char b;
    short c;
    unsigned short d;
    int e;
    unsigned int f;
    float g;
    double h;

    bool operator==(const Element &el) const
    {
      return a == el.a && b == el.b && c == el.c && d == el.d && e == el.e && f == el.f &&
             std::abs(g - el.g) < std::numeric_limits<float>::epsilon() &&
             std::abs(h - el.h) < std::numeric_limits<double>::epsilon();
    }
  };

  const std::vector<Element> expectedElements{
      {std::numeric_limits<char>::min(), std::numeric_limits<unsigned char>::max(),
       std::numeric_limits<short>::min(), std::numeric_limits<unsigned short>::max(),
       std::numeric_limits<int>::min(), std::numeric_limits<unsigned int>::max(),
       std::numeric_limits<float>::epsilon(), std::numeric_limits<double>::epsilon()}};

  using Layout =
      plywoot::reflect::Layout<char, unsigned char, short, unsigned short, int, unsigned int, float, double>;

  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(element, Layout{expectedElements});
  plyos.write(oss);

  const std::string data{oss.str()};
  std::stringstream iss{data, std::ios::in};
  plywoot::IStream plyis{iss};

  const std::vector<Element> elements{plyis.readElement<Element, Layout>()};
  CHECK(expectedElements == elements);
}

TEST_CASE("Test reading and writing of a list", "[iostream]")
{
  auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  const auto sizeType = plywoot::PlyDataType::Char;
  const plywoot::PlyProperty vertexIndices{"vertex_indices", plywoot::PlyDataType::Int, sizeType};
  const plywoot::PlyElement element{"triangle", 3, {vertexIndices}};

  const std::vector<Triangle> expectedTriangles{Triangle{0, 1, 2}, Triangle{5, 4, 3}, Triangle{6, 7, 8}};

  using Layout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(element, Layout{expectedTriangles});
  plyos.write(oss);

  const std::string data{oss.str()};
  std::stringstream iss{data, std::ios::in};
  plywoot::IStream plyis{iss};

  const std::vector<Triangle> triangles{plyis.readElement<Triangle, Layout>()};
  CHECK(expectedTriangles == triangles);
}

TEST_CASE("Test reading and writing of variable length lists", "[iostream]")
{
  auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  const auto sizeType = plywoot::PlyDataType::Char;
  const plywoot::PlyProperty numbers{"numbers", plywoot::PlyDataType::Int, sizeType};
  const plywoot::PlyElement element{"e", 3, {numbers}};

  struct Element
  {
    std::vector<int> numbers;

    bool operator==(const Element &x) const { return x.numbers == numbers; }
  };

  const std::vector<Element> expectedElements{Element{{0, 1, 2}}, Element{{3, 4}}, Element{{5, 6, 7, 8}}};

  using Layout = plywoot::reflect::Layout<std::vector<int>>;

  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(element, Layout{expectedElements});
  plyos.write(oss);

  std::stringstream iss{oss.str(), std::ios::in};
  plywoot::IStream plyis{iss};

  const std::vector<Element> elements{plyis.readElement<Element, Layout>()};
  CHECK(expectedElements == elements);
}

TEST_CASE("Tests reading and writing vertex and face data", "[iostream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube.ply", "test/input/binary/big_endian/cube.ply",
      "test/input/binary/little_endian/cube.ply");
  auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;
  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;

  std::vector<Triangle> triangles;
  std::vector<Vertex> vertices;

  while (plyFile.hasElement())
  {
    const plywoot::PlyElement element{plyFile.element()};
    if (element.name() == "vertex") { vertices = plyFile.readElement<Vertex, VertexLayout>(); }
    else if (element.name() == "face") { triangles = plyFile.readElement<Triangle, TriangleLayout>(); }
    else
      plyFile.skipElement();
  }

  const std::vector<Vertex> expectedVertices{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                             {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(vertices == expectedVertices);

  const std::vector<Triangle> expectedTriangles{{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
                                                {0, 1, 5}, {0, 5, 4}, {2, 3, 7}, {2, 7, 6},
                                                {3, 0, 4}, {3, 4, 7}, {1, 2, 6}, {1, 6, 5}};
  REQUIRE(expectedTriangles == triangles);

  // Now write the data to a string stream, read it back in again, and compare.
  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(plyFile.element("vertex").first, VertexLayout{vertices});
  plyos.add(plyFile.element("face").first, TriangleLayout{triangles});
  plyos.write(oss);

  {
    const plywoot::IStream plyis{oss};

    REQUIRE(plyis.element().name() == "vertex");
    const std::vector<Vertex> writtenVertices = plyis.readElement<Vertex, VertexLayout>();
    CHECK(vertices == writtenVertices);

    REQUIRE(plyis.element().name() == "face");
    const std::vector<Triangle> writtenTriangles = plyis.readElement<Triangle, TriangleLayout>();
    CHECK(triangles == writtenTriangles);
  }
}

TEST_CASE("Skip input data that cannot be mapped while reading and writing", "[iostream]")
{
  const auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  using Vertex = FloatVertex;
  const std::vector<Vertex> inputVertices{{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}, {7.0, 8.0, 9.0}};

  using Layout = plywoot::reflect::Layout<float, float, float>;

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"e", 3, {x, y}};

  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(element, Layout{inputVertices});
  plyos.write(oss);

  std::stringstream iss{oss.str(), std::ios::in};
  plywoot::IStream plyis{iss};
  REQUIRE(plyis.element().name() == "e");
  const std::vector<Vertex> outputVertices{plyis.readElement<Vertex, Layout>()};

  const std::vector<Vertex> expectedOutputVertices{{1.0, 2.0, 0.0}, {4.0, 5.0, 0.0}, {7.0, 8.0, 0.0}};
  CHECK(expectedOutputVertices == outputVertices);
}

TEST_CASE("Test casting of input property from integer to some floating point type", "[iostream]")
{
  const auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  const std::vector<int> numbers{1, 2, 3, 4, 5};

  using Layout = plywoot::reflect::Layout<int>;

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Double};
  const plywoot::PlyElement element{"e", {x}};

  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(element, Layout{numbers});
  plyos.write(oss);

  std::stringstream iss{oss.str(), std::ios::in};
  plywoot::IStream plyis{iss};

  REQUIRE(plyis.element().name() == "e");
  const std::vector<int> outputNumbers{plyis.readElement<int, Layout>()};
  CHECK(numbers == outputNumbers);
}

TEST_CASE("Test writing an element with more properties than defined in the memory layout", "[iostream]")
{
  const auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  std::stringstream oss;
  plywoot::OStream plyos{format};

  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty g{"g", plywoot::PlyDataType::Double, plywoot::PlyDataType::Char};
  const plywoot::PlyProperty h{"h", plywoot::PlyDataType::Int};
  const plywoot::PlyElement element{"e", 3, {f, g, h}};

  using Layout = plywoot::reflect::Layout<int>;

  const std::vector<int> values{1, 4, 7};
  plyos.add(element, Layout{values});
  plyos.write(oss);

  std::stringstream iss{oss.str(), std::ios::in};
  plywoot::IStream plyis{iss};

  REQUIRE(plyis.element().name() == "e");
  const std::vector<int> outputValues{plyis.readElement<int, Layout>()};
  CHECK(values == outputValues);
}

TEST_CASE("Test reading a double vertex from a PLY file that contains a float vertex.", "[iostream]")
{
  auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  std::stringstream oss;
  plywoot::OStream plyos{format};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"vertex", 1, {x, y, z}};

  using FloatLayout = plywoot::reflect::Layout<float, float, float>;
  using DoubleLayout = plywoot::reflect::Layout<double, double, double>;

  std::vector<FloatVertex> vertices = {FloatVertex{1.0f, 2.0f, 3.0f}};
  plyos.add(element, FloatLayout{vertices});
  plyos.write(oss);

  // Now read in the vertex using a pack of doubles.
  const std::string data{oss.str()};
  std::stringstream iss{data, std::ios::in};
  plywoot::IStream plyis{iss};

  const std::vector<DoubleVertex> expectedVertices = {DoubleVertex{1.0, 2.0, 3.0}};
  CHECK(expectedVertices == plyis.readElement<DoubleVertex, DoubleLayout>());
}

TEST_CASE(
    "Test reading a double vertex from a PLY file that contains a float vertex using a pack layout.",
    "[iostream]")
{
  auto format = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);

  std::stringstream oss;
  plywoot::OStream plyos{format};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"vertex", 1, {x, y, z}};

  using FloatLayout = plywoot::reflect::Layout<float, float, float>;
  using DoubleLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<double, 3>>;

  std::vector<FloatVertex> vertices = {FloatVertex{1.0f, 2.0f, 3.0f}};
  plyos.add(element, FloatLayout{vertices});
  plyos.write(oss);

  // Now read in the vertex using a pack of doubles.
  const std::string data{oss.str()};
  std::stringstream iss{data, std::ios::in};
  plywoot::IStream plyis{iss};

  const std::vector<DoubleVertex> expectedVertices = {DoubleVertex{1.0, 2.0, 3.0}};
  CHECK(expectedVertices == plyis.readElement<DoubleVertex, DoubleLayout>());
}

TEST_CASE("Test reading and writing of comments", "[iostream]")
{
  std::ifstream ifs{"test/input/ascii/comments.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  std::vector<plywoot::Comment> comments{plyFile.comments()};
  std::random_device rd;
  std::shuffle(comments.begin(), comments.end(), std::mt19937{rd()});

  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii, comments};
  plyos.add(elements.front(), plywoot::reflect::Layout<char>{});
  plyos.write(oss);

  // The text written by the writer should be equal to the text in the original
  // input file.
  CHECK(readAll("test/input/ascii/comments.ply") == oss.str());
}
