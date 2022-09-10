#include "types.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE("Test reading and writing all property types", "[iostream][binary-little-endian]")
{
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
    char b;
    char c;
    unsigned char d;
    short e;
    unsigned short f;
    float g;
    double h;

    bool operator==(const Element &el) const
    {
      return a == el.a && b == el.b && c == el.c && d == el.d && e == el.e && f == el.f &&
             std::abs(g - el.g) < std::numeric_limits<float>::epsilon() &&
             std::abs(h - el.h) < std::numeric_limits<double>::epsilon();
    }
  };

  const std::vector<Element> expected{
      {std::numeric_limits<char>::min(), std::numeric_limits<char>::min(), std::numeric_limits<char>::min(),
       std::numeric_limits<unsigned char>::max(), std::numeric_limits<short>::min(),
       std::numeric_limits<unsigned short>::max(), std::numeric_limits<float>::epsilon(),
       std::numeric_limits<double>::epsilon()}};

  using Layout =
      plywoot::reflect::Layout<char, char, char, unsigned char, short, unsigned short, float, double>;

  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::BinaryLittleEndian};
  plyos.add(element, Layout{expected});
  plyos.write(oss);

  const std::string data{oss.str()};
  std::stringstream iss{data, std::ios::in};
  plywoot::IStream plyis{iss};

  std::vector<Element> elements{plyis.read<Element, Layout>(element)};
  REQUIRE(expected == elements);
}

TEST_CASE("Test reading and writing of a list", "[iostream][binary-little-endian]")
{
  const auto sizeType{plywoot::PlyDataType::Char};
  const plywoot::PlyProperty vertexIndices{"vertex_indices", plywoot::PlyDataType::Int, sizeType};
  const plywoot::PlyElement element{"triangle", 3, {vertexIndices}};

  const std::vector<Triangle> expected{Triangle{0, 1, 2}, Triangle{5, 4, 3}, Triangle{6, 7, 8}};

  using Layout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3, char>>;

  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::BinaryLittleEndian};
  plyos.add(element, Layout{expected});
  plyos.write(oss);

  const std::string data{oss.str()};
  std::stringstream iss{data, std::ios::in};
  plywoot::IStream plyis{iss};

  const std::vector<Triangle> triangles{plyis.read<Triangle, Layout>(element)};
  REQUIRE(expected == triangles);
}

TEST_CASE("Tests reading and writing vertex and face data", "[iostream][binary-little-endian]")
{
  std::ifstream ifs{"test/input/binary_little_endian/cube.ply"};
  const plywoot::IStream plyFile{ifs};

  using Vertex = FloatVertex;

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");

  CHECK(isVertexElementFound);
  CHECK(vertexElement.name() == "vertex");
  CHECK(vertexElement.size() == 8);

  plywoot::PlyElement faceElement;
  bool isFaceElementFound{false};
  std::tie(faceElement, isFaceElementFound) = plyFile.element("face");

  CHECK(isFaceElementFound);
  CHECK(faceElement.name() == "face");
  CHECK(faceElement.size() == 12);

  using VertexLayout = plywoot::reflect::Layout<float, float, float>;
  const std::vector<Vertex> vertices = plyFile.read<Vertex, VertexLayout>(vertexElement);
  const std::vector<Vertex> expectedVertices{{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                             {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(vertices == expectedVertices);

  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3, unsigned char>>;
  const std::vector<Triangle> triangles = plyFile.read<Triangle, TriangleLayout>(faceElement);
  const std::vector<Triangle> expectedTriangles{{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
                                                {0, 1, 5}, {0, 5, 4}, {2, 3, 7}, {2, 7, 6},
                                                {3, 0, 4}, {3, 4, 7}, {1, 2, 6}, {1, 6, 5}};
  REQUIRE(expectedTriangles == triangles);

  // Now write the data to a string stream, read it back in again, and compare.
  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::BinaryLittleEndian};
  plyos.add(vertexElement, VertexLayout{vertices});
  plyos.add(faceElement, TriangleLayout{triangles});
  plyos.write(oss);

  {
    const plywoot::IStream plyis{oss};

    const std::vector<Vertex> writtenVertices = plyis.read<Vertex, VertexLayout>(vertexElement);
    CHECK(vertices == writtenVertices);

    const std::vector<Triangle> writtenTriangles = plyis.read<Triangle, TriangleLayout>(faceElement);
    CHECK(triangles == writtenTriangles);
  }
}
