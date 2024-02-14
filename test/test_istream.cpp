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
#include "util.hpp"

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <plywoot/plywoot.hpp>

#include <array>
#include <fstream>
#include <numeric>

TEST_CASE("Input file does not exist", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/missing.ply", std::ios::in};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::InvalidInputStream);
}

TEST_CASE("Input file is not a PLY file", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/invalid.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE("Input file does not contain a format definition", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/missing_format.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE(
    "Input file does contain a format definition, but not in the right order",
    "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/format_in_wrong_order.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE(
    "Input file does contain a format definition, but for some invalid format",
    "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/invalid_format.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::InvalidFormat);
}

TEST_CASE("Input file contains an ASCII format definition", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/ascii.ply"};

  const plywoot::IStream plyFile{ifs};
  REQUIRE(plywoot::PlyFormat::Ascii == plyFile.format());
}

TEST_CASE("Input file contains a binary little endian format definition", "[header][istream]")
{
  std::ifstream ifs{"test/input/header/binary_little_endian.ply"};

  const plywoot::IStream plyFile{ifs};
  REQUIRE(plywoot::PlyFormat::BinaryLittleEndian == plyFile.format());
}

TEST_CASE("Input file contains a binary big endian format definition", "[header][istream]")
{
  std::ifstream ifs{"test/input/header/binary_big_endian.ply"};

  const plywoot::IStream plyFile{ifs};
  REQUIRE(plywoot::PlyFormat::BinaryBigEndian == plyFile.format());
}

TEST_CASE("Element definition does not contain the number of elements", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/missing_element_size.ply"};
  REQUIRE_THROWS_MATCHES(
      plywoot::IStream(ifs), plywoot::UnexpectedToken,
      MessageContains("'end_header'") && MessageContains("'<number>'"));
}

TEST_CASE("No property data for an element value", "[header][istream][error]")
{
  struct S
  {
    char a, b;
  };

  using Layout = plywoot::reflect::Layout<char, char>;

  std::ifstream ifs{"test/input/header/missing_element_property_data.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);
  REQUIRE(elements.front().name() == "e");
  REQUIRE(elements.front().size() == 1);
  REQUIRE_THROWS_AS((plyFile.readElement<S, Layout>()), plywoot::UnexpectedEof);
}

TEST_CASE("Missing property data for an element value", "[header][istream][error]")
{
  struct S
  {
    char a, b;
  };

  using Layout = plywoot::reflect::Layout<char, char>;

  std::ifstream ifs{"test/input/header/missing_element_property_data_2.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);
  REQUIRE(elements.front().name() == "e");
  REQUIRE(elements.front().size() == 1);
  REQUIRE_THROWS_AS((plyFile.readElement<S, Layout>()), plywoot::UnexpectedEof);
}

TEST_CASE("A single element definition without properties is correctly parsed", "[header][istream]")
{
  std::ifstream ifs{"test/input/header/single_element.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);
  REQUIRE(elements.front().name() == "vertex");
  REQUIRE(elements.front().size() == 0);
}

TEST_CASE("Multiple element definitions without properties are correctly parsed", "[header][istream]")
{
  std::ifstream ifs{"test/input/header/multiple_elements.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 2);
  REQUIRE(elements.front().name() == "vertex");
  REQUIRE(elements.front().size() == 0);
  REQUIRE(elements.back().name() == "face");
  REQUIRE(elements.back().size() == 0);
}

TEST_CASE("A single element definition with properties is correctly parsed", "[header][istream]")
{
  std::ifstream ifs{"test/input/header/single_element_with_properties.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  const plywoot::PlyElement &element{elements.front()};
  REQUIRE(element.name() == "vertex");
  REQUIRE(element.size() == 0);
  REQUIRE(element.properties().size() == 9);

  const std::vector<plywoot::PlyProperty> &properties{element.properties()};
  REQUIRE(properties[0].name() == "a");
  REQUIRE_FALSE(properties[0].isList());
  REQUIRE(properties[0].type() == plywoot::PlyDataType::Char);

  REQUIRE(properties[1].name() == "b");
  REQUIRE_FALSE(properties[1].isList());
  REQUIRE(properties[1].type() == plywoot::PlyDataType::UChar);

  REQUIRE(properties[2].name() == "c");
  REQUIRE_FALSE(properties[2].isList());
  REQUIRE(properties[2].type() == plywoot::PlyDataType::Short);

  REQUIRE(properties[3].name() == "d");
  REQUIRE_FALSE(properties[3].isList());
  REQUIRE(properties[3].type() == plywoot::PlyDataType::UShort);

  REQUIRE(properties[4].name() == "e");
  REQUIRE_FALSE(properties[4].isList());
  REQUIRE(properties[4].type() == plywoot::PlyDataType::Int);

  REQUIRE(properties[5].name() == "f");
  REQUIRE_FALSE(properties[5].isList());
  REQUIRE(properties[5].type() == plywoot::PlyDataType::UInt);

  REQUIRE(properties[6].name() == "g");
  REQUIRE_FALSE(properties[6].isList());
  REQUIRE(properties[6].type() == plywoot::PlyDataType::Float);

  REQUIRE(properties[7].name() == "h");
  REQUIRE_FALSE(properties[7].isList());
  REQUIRE(properties[7].type() == plywoot::PlyDataType::Double);

  REQUIRE(properties[8].name() == "i");
  REQUIRE(properties[8].isList());
  REQUIRE(properties[8].type() == plywoot::PlyDataType::Int);
  REQUIRE(properties[8].sizeType() == plywoot::PlyDataType::UChar);
}

TEST_CASE(
    "A single element definition with properties is correctly parsed using alternate type names",
    "[header][istream]")
{
  std::ifstream ifs{"test/input/header/single_element_with_properties_using_type_aliases.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  const plywoot::PlyElement &element{elements.front()};
  REQUIRE(element.name() == "vertex");
  REQUIRE(element.size() == 0);
  REQUIRE(element.properties().size() == 9);

  const std::vector<plywoot::PlyProperty> &properties{element.properties()};
  REQUIRE(properties[0].name() == "a");
  REQUIRE_FALSE(properties[0].isList());
  REQUIRE(properties[0].type() == plywoot::PlyDataType::Char);

  REQUIRE(properties[1].name() == "b");
  REQUIRE_FALSE(properties[1].isList());
  REQUIRE(properties[1].type() == plywoot::PlyDataType::UChar);

  REQUIRE(properties[2].name() == "c");
  REQUIRE_FALSE(properties[2].isList());
  REQUIRE(properties[2].type() == plywoot::PlyDataType::Short);

  REQUIRE(properties[3].name() == "d");
  REQUIRE_FALSE(properties[3].isList());
  REQUIRE(properties[3].type() == plywoot::PlyDataType::UShort);

  REQUIRE(properties[4].name() == "e");
  REQUIRE_FALSE(properties[4].isList());
  REQUIRE(properties[4].type() == plywoot::PlyDataType::Int);

  REQUIRE(properties[5].name() == "f");
  REQUIRE_FALSE(properties[5].isList());
  REQUIRE(properties[5].type() == plywoot::PlyDataType::UInt);

  REQUIRE(properties[6].name() == "g");
  REQUIRE_FALSE(properties[6].isList());
  REQUIRE(properties[6].type() == plywoot::PlyDataType::Float);

  REQUIRE(properties[7].name() == "h");
  REQUIRE_FALSE(properties[7].isList());
  REQUIRE(properties[7].type() == plywoot::PlyDataType::Double);

  REQUIRE(properties[8].name() == "i");
  REQUIRE(properties[8].isList());
  REQUIRE(properties[8].type() == plywoot::PlyDataType::Int);
  REQUIRE(properties[8].sizeType() == plywoot::PlyDataType::UChar);
}

TEST_CASE("Read a PLY file with a comment section", "[header][comments]")
{
  std::ifstream ifs{"test/input/header/single_line_comment.ply"};
  const plywoot::IStream plyFile{ifs};
  REQUIRE(plyFile.elements().size() == 1);
  CHECK(plyFile.elements().front().name() == "vertex");
}

TEST_CASE("Read a PLY file with a comment section consisting of multiple lines", "[header][comments]")
{
  std::ifstream ifs{"test/input/header/multi_line_comment.ply"};
  const plywoot::IStream plyFile{ifs};
  REQUIRE(plyFile.elements().size() == 1);
  CHECK(plyFile.elements().front().name() == "vertex");
}

TEST_CASE("Read a PLY file using keywords for naming properties and elements", "[header][comments]")
{
  std::ifstream ifs{"test/input/header/keywords.ply"};

  const plywoot::IStream plyFile{ifs};
  REQUIRE(plyFile.elements().size() == 1);
  CHECK(plyFile.elements().front().name() == "element");
}

TEST_CASE("Read an element with a single property from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/single_element_with_single_property.ply",
      "test/input/binary/big_endian/single_element_with_single_property.ply",
      "test/input/binary/little_endian/single_element_with_single_property.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
  };

  using Layout = plywoot::reflect::Layout<char>;

  const std::vector<X> xs = plyFile.readElement<X, Layout>();
  REQUIRE(xs.size() == 1);
  REQUIRE(xs.front().c == 86);
}

TEST_CASE("Read multiple elements with a single property from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/multiple_elements_with_single_property.ply",
      "test/input/binary/big_endian/multiple_elements_with_single_property.ply",
      "test/input/binary/little_endian/multiple_elements_with_single_property.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
  };

  using Layout = plywoot::reflect::Layout<char>;

  const std::vector<X> xs = plyFile.readElement<X, Layout>();
  REQUIRE(xs.size() == 10);

  std::vector<char> expected(10);
  std::iota(expected.begin(), expected.end(), 86);
  REQUIRE(std::equal(expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
}

TEST_CASE("Read multiple elements with two properties from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/multiple_elements_with_two_properties.ply",
      "test/input/binary/big_endian/multiple_elements_with_two_properties.ply",
      "test/input/binary/little_endian/multiple_elements_with_two_properties.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    int c{0};
    unsigned char u{0};
  };

  using Layout = plywoot::reflect::Layout<int, unsigned char>;

  const std::vector<X> xs = plyFile.readElement<X, Layout>();
  REQUIRE(xs.size() == 10);

  // c
  {
    std::vector<int> expected(10);
    std::iota(expected.begin(), expected.end(), 86);
    REQUIRE(std::equal(expected.begin(), expected.end(), xs.begin(), [](int c, X x) { return c == x.c; }));
  }

  // u
  {
    std::vector<unsigned char> expected(10);
    std::iota(expected.begin(), expected.end(), 246);
    std::reverse(expected.begin(), expected.end());
    REQUIRE(std::equal(
        expected.begin(), expected.end(), xs.begin(), [](unsigned char u, X x) { return u == x.u; }));
  }
}

TEST_CASE("Retrieve a element and property definition from an IStream given an element name", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube.ply", "test/input/binary/big_endian/cube.ply",
      "test/input/binary/big_endian/cube.ply", "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement faceElement;
  bool isFaceElementFound{false};
  std::tie(faceElement, isFaceElementFound) = plyFile.element("face");

  CHECK(isFaceElementFound);
  CHECK(faceElement.name() == "face");
  CHECK(faceElement.size() == 12);

  plywoot::PlyProperty vertexIndicesProperty;
  bool isVertexIndicesPropertyFound{false};
  std::tie(vertexIndicesProperty, isVertexIndicesPropertyFound) = faceElement.property("vertex_indices");

  CHECK(vertexIndicesProperty.name() == "vertex_indices");
  CHECK(vertexIndicesProperty.type() == plywoot::PlyDataType::Int);
  CHECK(vertexIndicesProperty.isList());
  CHECK(vertexIndicesProperty.sizeType() == plywoot::PlyDataType::UChar);

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");

  CHECK(isVertexElementFound);
  CHECK(vertexElement.name() == "vertex");
  CHECK(vertexElement.size() == 8);
  CHECK(vertexElement.properties().size() == 3);

  plywoot::PlyElement fooElement;
  bool isFooElementFound{false};
  std::tie(fooElement, isFooElementFound) = plyFile.element("foo");

  CHECK(fooElement.size() == 0);
  CHECK(!isFooElementFound);
}

TEST_CASE("Test out of order retrieval of element data", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube_faces_before_vertices.ply",
      "test/input/binary/big_endian/cube_faces_before_vertices.ply",
      "test/input/binary/little_endian/cube_faces_before_vertices.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.find("vertex"));

  using Vertex = DoubleVertex;
  using VertexLayout = plywoot::reflect::Layout<double, double, double>;

  const std::vector<Vertex> result = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expected = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(result == expected);
}

TEST_CASE("Read elements from a PLY file by only partially retrieving all properties set for it", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube_with_material_data.ply",
      "test/input/binary/big_endian/cube_with_material_data.ply",
      "test/input/binary/little_endian/cube_with_material_data.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.find("vertex"));

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;

  const std::vector<Vertex> vertices = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE("Read elements from a PLY file by packing multiple properties together", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube.ply", "test/input/binary/big_endian/cube.ply",
      "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.find("vertex"));

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 3>>;

  const std::vector<Vertex> vertices = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE("Test casting of input property from float to double", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube.ply", "test/input/binary/big_endian/cube.ply",
      "test/input/binary/little_endian/cube.ply");
  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.find("vertex"));

  using Vertex = DoubleVertex;
  using VertexLayout = plywoot::reflect::Layout<double, double, double>;

  const std::vector<Vertex> result = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expected = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                        {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(result == expected);
}

TEST_CASE("Test reading comments interspersed in a PLY header", "[istream]")
{
  auto inputFilename = "test/input/ascii/comments.ply";

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  const std::vector<plywoot::Comment> expected{
      {2, "comment on the third line"},
      {3, "comment on the fourth line"},
      {5, "comment inside an element definition"},
      {7, ""}};
  CHECK(plyFile.comments() == expected);
}

TEST_CASE("Test reading Standford Bunny", "[istream]")
{
  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;
  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  auto inputFilename = "test/input/ascii/bunny.ply";

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  std::vector<Triangle> triangles;
  std::vector<Vertex> vertices;

  while (plyFile.hasElement())
  {
    const plywoot::PlyElement element = plyFile.element();
    if (element.name() == "vertex") { vertices = plyFile.readElement<Vertex, VertexLayout>(); }
    else if (element.name() == "face") { triangles = plyFile.readElement<Triangle, TriangleLayout>(); }
    else { plyFile.skipElement(); }
  }

  CHECK(triangles.back() == Triangle{17277, 17346, 17345});
  CHECK(vertices.back() == Vertex{-0.0400442, 0.15362, -0.00816685});
}

TEST_CASE("Test reading a zipped binary PLY file", "[istream]")
{
  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;
  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  auto inputFilename = "test/input/ascii/bunny.ply.gz";

  std::ifstream ifs{inputFilename};

  boost::iostreams::filtering_streambuf<boost::iostreams::input> buf;
  buf.push(boost::iostreams::gzip_decompressor());
  buf.push(ifs);

  std::istream is{&buf};
  const plywoot::IStream plyFile{is};

  std::vector<Triangle> triangles;
  std::vector<Vertex> vertices;

  while (plyFile.hasElement())
  {
    const plywoot::PlyElement element = plyFile.element();
    if (element.name() == "vertex") { vertices = plyFile.readElement<Vertex, VertexLayout>(); }
    else if (element.name() == "face") { triangles = plyFile.readElement<Triangle, TriangleLayout>(); }
    else { plyFile.skipElement(); }
  }

  CHECK(triangles.back() == Triangle{17277, 17346, 17345});
  CHECK(vertices.back() == Vertex{-0.0400442, 0.15362, -0.00816685});
}

TEST_CASE("Test reading data that can be directly mapped on to the target layout", "[istream]")
{
  auto inputFilename =
      GENERATE("test/input/binary/big_endian/cube.ply", "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;

  const std::vector<Vertex> vertices = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE(
    "Test reading data that can be directly mapped on to the target layout using packed properties",
    "[istream]")
{
  auto inputFilename =
      GENERATE("test/input/binary/big_endian/cube.ply", "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 3>>;

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "vertex");
  const std::vector<Vertex> vertices = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE(
    "Test reading data that can be directly mapped on to the target layout which is a vector of arrays",
    "[istream]")
{
  auto inputFilename =
      GENERATE("test/input/binary/big_endian/cube.ply", "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  using Vertex = std::array<float, 3>;
  using VertexLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 3>>;

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "vertex");
  const std::vector<Vertex> vertices = plyFile.readElement<Vertex, VertexLayout>();
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE(
    "Test reading data that can be directly mapped on to the target layout which is a vector of arrays, but "
    "which originates from a PLY list property, and can therefore not be memcpy'd",
    "[istream]")
{
  auto inputFilename =
      GENERATE("test/input/binary/big_endian/cube.ply", "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "vertex");
  plyFile.skipElement();

  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "face");
  const std::vector<Triangle> triangles = plyFile.readElement<Triangle, TriangleLayout>();
  const std::vector<Triangle> expectedTriangles{{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
                                                {0, 1, 5}, {0, 5, 4}, {2, 3, 7}, {2, 7, 6},
                                                {3, 0, 4}, {3, 4, 7}, {1, 2, 6}, {1, 6, 5}};
  CHECK(expectedTriangles == triangles);
}

TEST_CASE("Test reading a list property followed by non-list properties", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/binary/big_endian/single_element_with_list_property_followed_by_three_properties.ply",
      "test/input/binary/little_endian/single_element_with_list_property_followed_by_three_properties.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "vertex");
  plyFile.skipElement();

  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "face");
  const std::vector<Triangle> triangles = plyFile.readElement<Triangle, TriangleLayout>();
  const std::vector<Triangle> expectedTriangles{{4, 5, 6}, {7, 8, 9}};
  CHECK(expectedTriangles == triangles);
}

TEST_CASE("Read multiple elements with tricky alignment properties from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/alignment.ply", "test/input/binary/big_endian/alignment.ply",
      "test/input/binary/little_endian/alignment.ply");

  {
    std::ifstream ifs{inputFilename};
    const plywoot::IStream plyFile{ifs};
    REQUIRE(plyFile.elements().size() == 1);

    struct X
    {
      char c{0};
      std::vector<int> v;
      char d{0};

      inline bool operator==(const X &x) const { return c == x.c && v == x.v && d == x.d; }
    };

    using Layout = plywoot::reflect::Layout<char, std::vector<int>, char>;

    const std::vector<X> elements = plyFile.readElement<X, Layout>();
    REQUIRE(elements.size() == 5);

    const std::vector<X> expectedElements = {
        {86, {}, 87}, {88, {1}, 89}, {90, {1, 2}, 91}, {92, {1, 2, 3}, 93}, {94, {1, 2, 3, 4}, 95}};
    CHECK(expectedElements == elements);
  }

  {
    // Perform a raw read as well, to verify proper construction and destruction
    // of a `PlyElementData` instance.
    std::ifstream ifs{inputFilename};
    const plywoot::IStream plyFile{ifs};
    REQUIRE(plyFile.elements().size() == 1);

    plywoot::PlyElementData elementData = plyFile.readElement();
  }
}

TEST_CASE("Test skipping over a simple property", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/multiple_elements_with_two_properties.ply",
      "test/input/binary/big_endian/multiple_elements_with_two_properties.ply",
      "test/input/binary/little_endian/multiple_elements_with_two_properties.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  using Layout = plywoot::reflect::Layout<char, plywoot::reflect::Skip>;

  REQUIRE(plyFile.hasElement());
  REQUIRE(plyFile.element().name() == "vertex");

  const std::vector<char> cs = plyFile.readElement<char, Layout>();

  std::vector<char> expected(10);
  std::iota(expected.begin(), expected.end(), 86);
  REQUIRE(expected == cs);
}

TEST_CASE("Test skipping over a list with a variable size", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/element_with_variadic_list_to_skip.ply",
      "test/input/binary/little_endian/element_with_variadic_list_to_skip.ply",
      "test/input/binary/big_endian/element_with_variadic_list_to_skip.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  struct Foo
  {
    char x;
    unsigned char y;
  };

  using FooLayout = plywoot::reflect::Layout<char, plywoot::reflect::Skip, unsigned char>;

  REQUIRE(plyFile.hasElement());
  const plywoot::PlyElement element = plyFile.element();
  REQUIRE(plyFile.element().name() == "foo");

  const std::vector<Foo> foos = plyFile.readElement<Foo, FooLayout>();

  // x
  {
    std::vector<char> expected(10);
    std::iota(expected.begin(), expected.end(), 86);
    REQUIRE(std::equal(
        expected.begin(), expected.end(), foos.begin(), [](char c, Foo foo) { return c == foo.x; }));
  }

  // y
  {
    std::vector<unsigned char> expected(10);
    std::iota(expected.begin(), expected.end(), 246);
    std::reverse(expected.begin(), expected.end());
    REQUIRE(std::equal(
        expected.begin(), expected.end(), foos.begin(), [](unsigned char c, Foo foo) { return c == foo.y; }));
  }
}

TEST_CASE(
    "Test skipping over a list as the last property by not having the property mapped in the layout",
    "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/element_with_last_property_variadic_list.ply",
      "test/input/binary/little_endian/element_with_last_property_variadic_list.ply",
      "test/input/binary/big_endian/element_with_last_property_variadic_list.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  struct Foo
  {
    char x;
    unsigned char y;
  };

  using FooLayout = plywoot::reflect::Layout<char, unsigned char>;

  REQUIRE(plyFile.hasElement());
  const plywoot::PlyElement element = plyFile.element();
  REQUIRE(plyFile.element().name() == "foo");

  const std::vector<Foo> foos = plyFile.readElement<Foo, FooLayout>();

  // x
  {
    std::vector<char> expected(10);
    std::iota(expected.begin(), expected.end(), 86);
    REQUIRE(std::equal(
        expected.begin(), expected.end(), foos.begin(), [](char c, Foo foo) { return c == foo.x; }));
  }

  // y
  {
    std::vector<unsigned char> expected(10);
    std::iota(expected.begin(), expected.end(), 246);
    std::reverse(expected.begin(), expected.end());
    REQUIRE(std::equal(
        expected.begin(), expected.end(), foos.begin(), [](unsigned char c, Foo foo) { return c == foo.y; }));
  }
}
