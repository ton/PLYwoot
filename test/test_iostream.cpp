#include "test_types.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <limits>
#include <numeric>
#include <sstream>

TEST_CASE("Test reading and writing all property types", "[iostream]")
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

  const std::vector<Element> expected{
      {std::numeric_limits<char>::min(), std::numeric_limits<unsigned char>::max(),
       std::numeric_limits<short>::min(), std::numeric_limits<unsigned short>::max(),
       std::numeric_limits<int>::min(), std::numeric_limits<unsigned int>::max(),
       std::numeric_limits<float>::epsilon(), std::numeric_limits<double>::epsilon()}};

  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
  plyos.add(element, expected);
  plyos.write(oss);

  const std::string ascii{oss.str()};
  std::stringstream iss{ascii, std::ios::in};
  plywoot::IStream plyis{iss};

  std::vector<Element> elements{plyis.read<Element>(element)};
  REQUIRE(expected == elements);
}

TEST_CASE("Test reading and writing of a list", "[iostream]")
{
  const auto sizeType{plywoot::PlyDataType::Char};
  const std::size_t sizeHint{3};
  const plywoot::PlyProperty vertexIndices{"vertex_indices", plywoot::PlyDataType::Int, sizeType, sizeHint};
  const plywoot::PlyElement element{"triangle", 3, {vertexIndices}};

  struct Triangle
  {
    int a, b, c;

    bool operator==(const Triangle &t) const { return a == t.a && b == t.b && c == t.c; }
  };

  const std::vector<Triangle> expected{Triangle{0, 1, 2}, Triangle{5, 4, 3}, Triangle{6, 7, 8}};

  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
  plyos.add(element, expected);
  plyos.write(oss);

  const std::string ascii{oss.str()};
  std::stringstream iss{ascii, std::ios::in};
  plywoot::IStream plyis{iss};

  std::vector<Triangle> triangles{plyis.read<Triangle>(element)};
  REQUIRE(expected == triangles);
}

TEST_CASE("Test reading and writing all property types with casts", "[iostream][casts]")
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
    float h;

    bool operator==(const Element &el) const
    {
      return a == el.a && b == el.b && c == el.c && d == el.d && e == el.e && f == el.f &&
             std::abs(g - el.g) < std::numeric_limits<float>::epsilon() &&
             std::abs(h - el.h) < std::numeric_limits<float>::epsilon();
    }
  };

  const std::vector<Element> expected{
      {std::numeric_limits<char>::min(), std::numeric_limits<char>::min(), std::numeric_limits<char>::min(),
       std::numeric_limits<unsigned char>::max(), std::numeric_limits<short>::min(),
       std::numeric_limits<unsigned short>::max(), std::numeric_limits<float>::epsilon(),
       std::numeric_limits<float>::epsilon()}};

  std::stringstream oss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
  plyos.add<Element, char, char, char, unsigned char, short, unsigned short, float, float>(element, expected);
  plyos.write(oss);

  const std::string ascii{oss.str()};
  std::stringstream iss{ascii, std::ios::in};
  plywoot::IStream plyis{iss};

  std::vector<Element> elements{
      plyis.read<Element, char, char, char, unsigned char, short, unsigned short, float, float>(element)};
  REQUIRE(expected == elements);
}

TEST_CASE("Tests reading and writing vertex and face data", "[iostream][casts]")
{
  std::ifstream ifs{"test/input/cube.ply"};
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

  const std::vector<Vertex> vertices = plyFile.read<Vertex, float, float, float>(vertexElement);
  CHECK(vertices.size() == 8);

  const std::vector<Face> faces = plyFile.read<Face>(faceElement);
  CHECK(faces.size() == 12);

  // Now write the data to a string stream, read it back in again, and compare.
  std::stringstream oss;
  plywoot::PlyElement faceElementWithSizeHint{faceElement.setSizeHint("vertex_indices", 3)};

  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
  plyos.add<Vertex, float, float, float>(vertexElement, vertices);
  plyos.add(faceElementWithSizeHint, faces);
  plyos.write(oss);

  {
    const plywoot::IStream plyis{oss};

    const std::vector<Vertex> writtenVertices = plyis.read<Vertex, float, float, float>(vertexElement);
    CHECK(vertices == writtenVertices);

    const std::vector<Face> writtenFaces = plyis.read<Face>(faceElement);
    CHECK(faces == writtenFaces);
  }
}
