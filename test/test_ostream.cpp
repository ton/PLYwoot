#include "util.hpp"

#include <plywoot/plywoot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <numeric>
#include <sstream>

namespace
{
  struct Vertex
  {
    double x, y, z;
  };
}

TEST_CASE("Write empty PLY file", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};
  plyos.write(ss);

  const std::string expected{"ply\nformat ascii 1.0\nend_header\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file with a single element and some properties", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Double};
  const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Double};
  const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Double};
  const plywoot::PlyElement element{"vertex", 3, {x, y, z}};

  std::vector<Vertex> vertices{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  plyos.add(element, vertices);
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element vertex 3\n"
      "property double x\n"
      "property double y\n"
      "property double z\n"
      "end_header\n"
      "1 2 3\n"
      "4 5 6\n"
      "7 8 9\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE(
    "Write PLY file with a single element and some properties without using explicit casting which "
    "should fail",
    "[ostream][ascii][casts]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"e", 3, {f}};

  std::vector<int> values{1, 4, 7};
  plyos.add(element, values);
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element e 3\n"
      "property float f\n"
      "end_header\n"
      "1\n"
      "4\n"
      "7\n"};
  REQUIRE(expected != ss.str());
}

TEST_CASE(
    "Write PLY file with a single element and some properties using explicit casting which should "
    "succeed",
    "[ostream][ascii][casts]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty f{"f", plywoot::PlyDataType::Float};
  const plywoot::PlyElement element{"e", 3, {f}};

  std::vector<int> values{1, 4, 7};
  plyos.add<int, int>(element, values);
  plyos.write(ss);

  const std::string expected{
      "ply\n"
      "format ascii 1.0\n"
      "element e 3\n"
      "property float f\n"
      "end_header\n"
      "1\n"
      "4\n"
      "7\n"};
  REQUIRE(expected == ss.str());
}

TEST_CASE("Write PLY file with a single element with a list property", "[ostream][ascii]")
{
  std::stringstream ss;
  plywoot::OStream plyos{plywoot::PlyFormat::Ascii};

  const plywoot::PlyProperty faceIndices{"vertex_indices", plywoot::PlyDataType::Int, true};
  const plywoot::PlyElement element{"face", 10, {faceIndices}};

  std::vector<int> values;
  plyos.add(element, values);
  plyos.write(ss);

  const std::string expected{
      "ply\nformat ascii 1.0\nelement face 10\nproperty list char int "
      "vertex_indices\nend_header\n"};
  REQUIRE(expected == ss.str());
}
