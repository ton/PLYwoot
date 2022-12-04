#include "types.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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

  const std::vector<Element> elements{plyis.read<Element, Layout>(element)};
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

  const std::vector<Triangle> triangles{plyis.read<Triangle, Layout>(element)};
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

  const std::vector<Element> elements{plyis.read<Element, Layout>(element)};
  CHECK(expectedElements == elements);
}

TEST_CASE("Tests reading and writing vertex and face data", "[iostream]")
{
  auto inputFilename = GENERATE("test/input/ascii/cube.ply", "test/input/binary/little_endian/cube.ply");
  auto format = GENERATE(plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian);

  std::ifstream ifs{inputFilename};
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

  using TriangleLayout = plywoot::reflect::Layout<plywoot::reflect::Array<int, 3>>;
  const std::vector<Triangle> triangles = plyFile.read<Triangle, TriangleLayout>(faceElement);
  const std::vector<Triangle> expectedTriangles{{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
                                                {0, 1, 5}, {0, 5, 4}, {2, 3, 7}, {2, 7, 6},
                                                {3, 0, 4}, {3, 4, 7}, {1, 2, 6}, {1, 6, 5}};
  REQUIRE(expectedTriangles == triangles);

  // Now write the data to a string stream, read it back in again, and compare.
  std::stringstream oss;
  plywoot::OStream plyos{format};
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
  const std::vector<Vertex> outputVertices{plyis.read<Vertex, Layout>(element)};

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
  // TODO(ton): inconvenient to have to specify the size below...add() should
  // just override the element size by the amount of input data!
  const plywoot::PlyElement element{"e", numbers.size(), {x}};

  std::stringstream oss;
  plywoot::OStream plyos{format};
  plyos.add(element, Layout{numbers});
  plyos.write(oss);

  std::stringstream iss{oss.str(), std::ios::in};
  plywoot::IStream plyis{iss};
  const std::vector<int> outputNumbers{plyis.read<int, Layout>(element)};
  CHECK(numbers == outputNumbers);
}

TEST_CASE("Test writing an element with more properties than defined in the memory layout", "[ostream]")
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
  const std::vector<int> outputValues{plyis.read<int, Layout>(element)};
  CHECK(values == outputValues);
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
  plyos.add(elements.front(), plywoot::reflect::Layout<>{});
  plyos.write(oss);

  // The text written by the writer should be equal to the text in the original
  // input file.
  CHECK(readAll("test/input/ascii/comments.ply") == oss.str());
}
