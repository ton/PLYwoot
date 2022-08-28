#include "types.hpp"
#include "util.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <numeric>

TEST_CASE("Read an element with a single property from a binary PLY file", "[istream][binary-little-endian]")
{
  std::ifstream ifs{"test/input/binary_little_endian/single_element_with_single_property.ply"};
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
