#include "util.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <numeric>

TEST_CASE("Input file does not exist", "[istream][error]")
{
  std::ifstream ifs{"test/input/missing.ply", std::ios::in};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE("Input file is not a PLY file", "[istream][error]")
{
  std::ifstream ifs{"test/input/invalid.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE("Input file does not contain a format definition", "[istream][error][format]")
{
  std::ifstream ifs{"test/input/missing_format.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE(
    "Input file does contain a format definition, but not in the right order",
    "[istream][error][format]")
{
  std::ifstream ifs{"test/input/format_in_wrong_order.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
}

TEST_CASE(
    "Input file does contain a format definition, but for some invalid format",
    "[istream][error][format]")
{
  std::ifstream ifs{"test/input/invalid_format.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::InvalidFormat);
}

TEST_CASE(
    "Input file does contains a binary little endian format definition, which is not supported",
    "[istream][error][format]")
{
  std::ifstream ifs{"test/input/binary_little_endian.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnsupportedFormat);
}

TEST_CASE(
    "Input file contains a binary little endian format definition, which is not supported",
    "[istream][error][format]")
{
  std::ifstream ifs{"test/input/binary_big_endian.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnsupportedFormat);
}

TEST_CASE("Element definition does not contain the number of elements", "[istream][error][ascii]")
{
  std::ifstream ifs{"test/input/missing_element_size.ply"};
  REQUIRE_THROWS_MATCHES(
      plywoot::IStream(ifs), plywoot::UnexpectedToken,
      MessageContains("'end_header'") && MessageContains("'<number>'"));
}

TEST_CASE("No property data for an element value", "[istream][error][ascii]")
{
  struct S
  {
    char a, b;
  };

  std::ifstream ifs{"test/input/missing_element_property_data.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);
  REQUIRE(elements.front().name == "e");
  REQUIRE(elements.front().size == 1);
  REQUIRE_THROWS_AS(plyFile.read<S>(elements.front()), plywoot::UnexpectedEof);
}

TEST_CASE("Missing property data for an element value", "[istream][error][ascii]")
{
  struct S
  {
    char a, b;
  };

  std::ifstream ifs{"test/input/missing_element_property_data_2.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);
  REQUIRE(elements.front().name == "e");
  REQUIRE(elements.front().size == 1);
  REQUIRE_THROWS_AS(plyFile.read<S>(elements.front()), plywoot::UnexpectedEof);
}

TEST_CASE("A single element definition without properties is correctly parsed", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/single_element.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);
  REQUIRE(elements.front().name == "vertex");
  REQUIRE(elements.front().size == 0);
}

TEST_CASE(
    "Multiple element definitions without properties are correctly parsed",
    "[istream][ascii]")
{
  std::ifstream ifs{"test/input/multiple_elements.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 2);
  REQUIRE(elements.front().name == "vertex");
  REQUIRE(elements.front().size == 0);
  REQUIRE(elements.back().name == "face");
  REQUIRE(elements.back().size == 0);
}

TEST_CASE("A single element definition with properties is correctly parsed", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/single_element_with_properties.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  const plywoot::PlyElement &element{elements.front()};
  REQUIRE(element.name == "vertex");
  REQUIRE(element.size == 0);
  REQUIRE(element.properties.size() == 9);

  REQUIRE(element.properties[0].name == "a");
  REQUIRE_FALSE(element.properties[0].isList);
  REQUIRE(element.properties[0].type == plywoot::PlyDataType::Char);

  REQUIRE(element.properties[1].name == "b");
  REQUIRE_FALSE(element.properties[1].isList);
  REQUIRE(element.properties[1].type == plywoot::PlyDataType::UChar);

  REQUIRE(element.properties[2].name == "c");
  REQUIRE_FALSE(element.properties[2].isList);
  REQUIRE(element.properties[2].type == plywoot::PlyDataType::Short);

  REQUIRE(element.properties[3].name == "d");
  REQUIRE_FALSE(element.properties[3].isList);
  REQUIRE(element.properties[3].type == plywoot::PlyDataType::UShort);

  REQUIRE(element.properties[4].name == "e");
  REQUIRE_FALSE(element.properties[4].isList);
  REQUIRE(element.properties[4].type == plywoot::PlyDataType::Int);

  REQUIRE(element.properties[5].name == "f");
  REQUIRE_FALSE(element.properties[5].isList);
  REQUIRE(element.properties[5].type == plywoot::PlyDataType::UInt);

  REQUIRE(element.properties[6].name == "g");
  REQUIRE_FALSE(element.properties[6].isList);
  REQUIRE(element.properties[6].type == plywoot::PlyDataType::Float);

  REQUIRE(element.properties[7].name == "h");
  REQUIRE_FALSE(element.properties[7].isList);
  REQUIRE(element.properties[7].type == plywoot::PlyDataType::Double);

  REQUIRE(element.properties[8].name == "i");
  REQUIRE(element.properties[8].isList);
  REQUIRE(element.properties[8].type == plywoot::PlyDataType::Int);
  REQUIRE(element.properties[8].sizeType == plywoot::PlyDataType::UChar);
}

TEST_CASE("Read an element with a single property from an ASCII PLY file", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/single_element_with_single_property.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
  };

  const std::vector<X> xs = plyFile.read<X>(elements.front());
  REQUIRE(xs.size() == 1);
  REQUIRE(xs.front().c == 86);
}

TEST_CASE(
    "Read multiple elements with a single property from an ASCII PLY file",
    "[istream][ascii]")
{
  std::ifstream ifs{"test/input/multiple_elements_with_single_property.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
  };

  const std::vector<X> xs = plyFile.read<X>(elements.front());
  REQUIRE(xs.size() == 10);

  std::vector<char> expected(10);
  std::iota(expected.begin(), expected.end(), 86);
  REQUIRE(std::equal(
      expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
}

TEST_CASE(
    "Read multiple elements with a single property from an ASCII PLY file with type casts",
    "[istream][ascii][cast]")
{
  std::ifstream ifs{"test/input/multiple_elements_with_single_property.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    int c{0};
  };

  const std::vector<X> xs = plyFile.read<X, int>(elements.front());
  REQUIRE(xs.size() == 10);

  std::vector<char> expected(10);
  std::iota(expected.begin(), expected.end(), 86);
  REQUIRE(std::equal(
      expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
}

TEST_CASE("Read multiple elements with two properties from an ASCII PLY file", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/multiple_elements_with_two_properties.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    char c{0};
    unsigned char u{0};
  };

  const std::vector<X> xs = plyFile.read<X>(elements.front());
  REQUIRE(xs.size() == 10);

  // c
  {
    std::vector<char> expected(10);
    std::iota(expected.begin(), expected.end(), 86);
    REQUIRE(std::equal(
        expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
  }

  // u
  {
    std::vector<unsigned char> expected(10);
    std::iota(expected.begin(), expected.end(), 246);
    std::reverse(expected.begin(), expected.end());
    REQUIRE(std::equal(
        expected.begin(), expected.end(), xs.begin(),
        [](unsigned char u, X x) { return u == x.u; }));
  }
}

TEST_CASE("Retrieve a element and property definition from an IStream given an element name", "[istream][ascii]")
{
  std::ifstream ifs{"test/input/cube.ply"};
  const plywoot::IStream plyFile{ifs};

  plywoot::PlyElement faceElement;
  bool isFaceElementFound{false};
  std::tie(faceElement, isFaceElementFound) = plyFile.element("face");

  CHECK(isFaceElementFound);
  CHECK(faceElement.name == "face");
  CHECK(faceElement.size == 12);

  plywoot::PlyProperty vertexIndicesProperty;
  bool isVertexIndicesPropertyFound{false};
  std::tie(vertexIndicesProperty, isVertexIndicesPropertyFound) = faceElement.property("vertex_indices");

  CHECK(vertexIndicesProperty.name == "vertex_indices");
  CHECK(vertexIndicesProperty.type == plywoot::PlyDataType::Int);
  CHECK(vertexIndicesProperty.isList);
  CHECK(vertexIndicesProperty.sizeType == plywoot::PlyDataType::UChar);

  plywoot::PlyElement vertexElement;
  bool isVertexElementFound{false};
  std::tie(vertexElement, isVertexElementFound) = plyFile.element("vertex");

  CHECK(isVertexElementFound);
  CHECK(vertexElement.name == "vertex");
  CHECK(vertexElement.size == 8);
  CHECK(vertexElement.properties.size() == 3);

  plywoot::PlyElement fooElement;
  bool isFooElementFound{false};
  std::tie(fooElement, isFooElementFound) = plyFile.element("foo");

  CHECK(fooElement.size == 0);
  CHECK(!isFooElementFound);
}

TEST_CASE(
    "Read multiple elements with two properties from an ASCII PLY file with type casts",
    "[istream][ascii][casts]")
{
  std::ifstream ifs{"test/input/multiple_elements_with_two_properties.ply"};
  const plywoot::IStream plyFile{ifs};
  const std::vector<plywoot::PlyElement> elements{plyFile.elements()};
  REQUIRE(elements.size() == 1);

  struct X
  {
    int c{0};
    unsigned char u{0};
  };

  const std::vector<X> xs = plyFile.read<X, int, unsigned char>(elements.front());
  REQUIRE(xs.size() == 10);

  // c
  {
    std::vector<char> expected(10);
    std::iota(expected.begin(), expected.end(), 86);
    REQUIRE(std::equal(
        expected.begin(), expected.end(), xs.begin(), [](char c, X x) { return c == x.c; }));
  }

  // u
  {
    std::vector<unsigned char> expected(10);
    std::iota(expected.begin(), expected.end(), 246);
    std::reverse(expected.begin(), expected.end());
    REQUIRE(std::equal(
        expected.begin(), expected.end(), xs.begin(),
        [](unsigned char u, X x) { return u == x.u; }));
  }
}

TEST_CASE("Read a PLY file with a comment section", "[ascii][comments]")
{
  std::ifstream ifs{"test/input/single_line_comment.ply"};
  const plywoot::IStream plyFile{ifs};
  REQUIRE(plyFile.elements().size() == 1);
  CHECK(plyFile.elements().front().name == "vertex");
}

TEST_CASE("Read a PLY file with a comment section consisting of multiple lines", "[ascii][comments]")
{
  std::ifstream ifs{"test/input/multi_line_comment.ply"};
  const plywoot::IStream plyFile{ifs};
  REQUIRE(plyFile.elements().size() == 1);
  CHECK(plyFile.elements().front().name == "vertex");
}
