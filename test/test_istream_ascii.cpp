#include "types.hpp"
#include "util.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <numeric>

TEST_CASE("Read an element with a single property from an ASCII PLY file", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/ascii/single_element_with_single_property.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
  };

  using Layout = plywoot::reflect::Layout<char>;

  const std::vector<X> xs = plyFile.read<X, Layout>(elements.front());
  REQUIRE(xs.size() == 1);
  REQUIRE(xs.front().c == 86);
}

TEST_CASE("Read multiple elements with a single property from an ASCII PLY file", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/ascii/multiple_elements_with_single_property.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
  };

  using Layout = plywoot::reflect::Layout<char>;

  const std::vector<X> xs = plyFile.read<X, Layout>(elements.front());
  REQUIRE(xs.size() == 10);

  std::vector<char> expected(10);
  std::iota(expected.begin(), expected.end(), 86);
  REQUIRE(std::equal(expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
}

TEST_CASE("Read multiple elements with two properties from an ASCII PLY file", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/ascii/multiple_elements_with_two_properties.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    int c{0};
    unsigned char u{0};
  };

  using Layout = plywoot::reflect::Layout<int, unsigned char>;

  const std::vector<X> xs = plyFile.read<X, Layout>(elements.front());
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

TEST_CASE(
    "Retrieve a element and property definition from an IStream given an element name",
    "[istream][ascii]")
{
  std::ifstream ifs{"test/input/ascii/cube.ply"};
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

TEST_CASE("Test out of order retrieval of element data", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/ascii/cube_faces_before_vertices.ply"};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement faceElement;
  bool isFaceElementFound{false};
  std::tie(faceElement, isFaceElementFound) = plyFile.element("face");
  REQUIRE(isFaceElementFound);

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");
  REQUIRE(isVertexElementFound);

  using Vertex = DoubleVertex;
  using VertexLayout = plywoot::reflect::Layout<double, double, double>;

  const std::vector<Vertex> result = plyFile.read<Vertex, VertexLayout>(vertexElement);
  const std::vector<Vertex> expected = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(result == expected);
}

TEST_CASE(
    "Read elements from a PLY file by only partially retrieving all properties set for it",
    "[istream][ascii]")
{
  std::ifstream ifs{"test/input/ascii/cube_with_material_data.ply"};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");
  REQUIRE(isVertexElementFound);

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;

  const std::vector<Vertex> result = plyFile.read<Vertex, VertexLayout>(vertexElement);
  const std::vector<Vertex> expected = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
    {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(result == expected);
}
