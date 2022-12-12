#include "types.hpp"
#include "util.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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
  REQUIRE_THROWS_AS((plyFile.read<S, Layout>(elements.front())), plywoot::UnexpectedEof);
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
  REQUIRE_THROWS_AS((plyFile.read<S, Layout>(elements.front())), plywoot::UnexpectedEof);
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

TEST_CASE("Read an element with a single property from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/single_element_with_single_property.ply",
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

  const std::vector<X> xs = plyFile.read<X, Layout>(elements.front());
  REQUIRE(xs.size() == 1);
  REQUIRE(xs.front().c == 86);
}

TEST_CASE("Read multiple elements with a single property from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/multiple_elements_with_single_property.ply",
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

  const std::vector<X> xs = plyFile.read<X, Layout>(elements.front());
  REQUIRE(xs.size() == 10);

  std::vector<char> expected(10);
  std::iota(expected.begin(), expected.end(), 86);
  REQUIRE(std::equal(expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
}

TEST_CASE("Read multiple elements with two properties from a PLY file", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/multiple_elements_with_two_properties.ply",
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

TEST_CASE("Retrieve a element and property definition from an IStream given an element name", "[istream]")
{
  auto inputFilename = GENERATE("test/input/ascii/cube.ply", "test/input/binary/little_endian/cube.ply");

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
      "test/input/binary/little_endian/cube_faces_before_vertices.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

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

TEST_CASE("Read elements from a PLY file by only partially retrieving all properties set for it", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube_with_material_data.ply",
      "test/input/binary/little_endian/cube_with_material_data.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");
  REQUIRE(isVertexElementFound);

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;

  const std::vector<Vertex> vertices = plyFile.read<Vertex, VertexLayout>(vertexElement);
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE("Read elements from a PLY file by packing multiple properties together", "[istream]")
{
  auto inputFilename = GENERATE(
      "test/input/ascii/cube.ply",
      "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");
  REQUIRE(isVertexElementFound);

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<float, 3>>;

  const std::vector<Vertex> vertices = plyFile.read<Vertex, VertexLayout>(vertexElement);
  const std::vector<Vertex> expectedVertices = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  CHECK(expectedVertices == vertices);
}

TEST_CASE("Test casting of input property from float to double", "[istream]")
{
  auto inputFilename = GENERATE("test/input/ascii/cube.ply", "test/input/binary/little_endian/cube.ply");

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

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

// Note; this test case exists to not solely rely on the binary big endian
// writer to generate input for the parser.
TEST_CASE("Test reading a random binary big endian PLY file found somewhere on the internet", "[istream]")
{
  auto inputFilename = "test/input/binary/big_endian/cube.ply";

  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");
  REQUIRE(isVertexElementFound);

  using Vertex = FloatVertex;
  using VertexLayout = plywoot::reflect::Layout<float, float, float>;

  const std::vector<Vertex> result = plyFile.read<Vertex, VertexLayout>(vertexElement);
  const std::vector<Vertex> expected = {{0, 0, 0}, {0, 1, 0}, {1, 0, 0}, {1, 1, 0},
                                        {0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 1, 1}};
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
