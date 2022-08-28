#include "types.hpp"
#include "util.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <numeric>

TEST_CASE("Input file does not exist", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/missing.ply", std::ios::in};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnexpectedToken);
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

TEST_CASE(
    "Input file contains a binary big endian format definition, which is not supported",
    "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/binary_big_endian.ply"};
  REQUIRE_THROWS_AS(plywoot::IStream(ifs), plywoot::UnsupportedFormat);
}

TEST_CASE("Input file contains an ASCII format definition", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/ascii.ply"};

  const plywoot::IStream plyFile{ifs};
  REQUIRE(plywoot::PlyFormat::Ascii == plyFile.format());
}

TEST_CASE("Input file contains a binary little endian format definition", "[header][istream][error]")
{
  std::ifstream ifs{"test/input/header/binary_little_endian.ply"};

  const plywoot::IStream plyFile{ifs};
  REQUIRE(plywoot::PlyFormat::BinaryLittleEndian == plyFile.format());
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
