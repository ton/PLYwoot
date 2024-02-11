#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <plywoot/plywoot.hpp>

#include <fstream>
#include <iostream>  // Remove!
#include <sstream>
#include <vector>

using namespace plywoot;

namespace {

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

  inline friend bool operator==(const Element &x, const Element &y)
  {
    return x.a == y.a && x.b == y.b && x.c == y.c && x.d == y.d && x.e == y.e && x.f == y.f && x.g == y.g &&
           x.h == y.h;
  }
};
using ElementLayout =
    plywoot::reflect::Layout<char, unsigned char, short, unsigned short, int, unsigned int, float, double>;

using UChars = std::vector<unsigned char>;
using UCharsLayout = plywoot::reflect::Layout<UChars>;

using Ints = std::vector<int>;
using IntsLayout = plywoot::reflect::Layout<Ints>;

}

TEST_CASE("Test converting an ASCII PLY file to binary little and big endian.", "[convert]")
{
  auto targetFormat = GENERATE(
      plywoot::PlyFormat::Ascii, plywoot::PlyFormat::BinaryLittleEndian, plywoot::PlyFormat::BinaryBigEndian);
  auto inputFilename = GENERATE(
      "test/input/ascii/all.ply", "test/input/binary/little_endian/all.ply",
      "test/input/binary/big_endian/all.ply");

  // First, read the input file to determine
  std::ifstream ifs{inputFilename};
  const plywoot::IStream plyFile{ifs};

  REQUIRE(plyFile.find("element"));
  const std::vector<Element> inputElements = plyFile.readElement<Element, ElementLayout>();
  REQUIRE(inputElements.size() == 10);

  // Verify the contents of the elements.
  const std::vector<Element> expectedElements = {
      {86, 255, -32768, 65535, -2147483648, 2147483647, 1.0, -1.0},
      {87, 254, -32767, 65534, -2147483647, 2147483646, 2.0, -2.0},
      {88, 253, -32766, 65533, -2147483646, 2147483645, 3.0, -3.0},
      {89, 252, -32765, 65532, -2147483645, 2147483644, 4.0, -4.0},
      {90, 251, -32764, 65531, -2147483644, 2147483643, 5.0, -5.0},
      {91, 250, -32763, 65530, -2147483643, 2147483642, 6.0, -6.0},
      {92, 249, -32762, 65529, -2147483642, 2147483641, 7.0, -7.0},
      {93, 248, -32761, 65528, -2147483641, 2147483640, 8.0, -8.0},
      {94, 247, -32760, 65527, -2147483640, 2147483639, 9.0, -9.0},
      {95, 246, -32759, 65526, -2147483639, 2147483638, 9.9, -9.9},
  };
  REQUIRE(expectedElements == inputElements);

  REQUIRE(plyFile.find("uchar_list_size_type_uchar"));
  const std::vector<UChars> inputUChars = plyFile.readElement<UChars, UCharsLayout>();
  const std::vector<UChars> expectedUChars = {{}, {255}, {255, 254}, {255, 254, 253}, {255, 254, 253, 252}};
  REQUIRE(expectedUChars == inputUChars);

  REQUIRE(plyFile.find("int_list_size_type_int"));
  const std::vector<Ints> inputInts = plyFile.readElement<Ints, IntsLayout>();
  const std::vector<Ints> expectedInts = {
      {},
      {-2147483648},
      {-2147483648, -2147483647},
      {-2147483648, -2147483647, -2147483646},
      {-2147483648, -2147483647, -2147483646, -2147483645}};
  REQUIRE(expectedInts == inputInts);

  // Convert the input file to another format.
  {
    std::ifstream refIfs{inputFilename};
    std::stringstream oss;
    plywoot::convert(refIfs, oss, targetFormat);

    const plywoot::IStream bleIs{oss};
    REQUIRE(bleIs.format() == targetFormat);
    REQUIRE(bleIs.find("element"));
    CHECK(expectedElements == bleIs.readElement<Element, ElementLayout>());

    REQUIRE(bleIs.find("uchar_list_size_type_uchar"));
    CHECK(expectedUChars == bleIs.readElement<UChars, UCharsLayout>());

    REQUIRE(bleIs.find("int_list_size_type_int"));
    CHECK(expectedInts == bleIs.readElement<Ints, IntsLayout>());
  }
}
