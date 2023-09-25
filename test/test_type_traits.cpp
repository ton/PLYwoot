#include <catch2/catch_test_macros.hpp>
#include <plywoot/plywoot.hpp>

using namespace plywoot;

TEST_CASE("Test isMemcpyable type trait for a single property", "[type_traits]")
{
  const std::vector<PlyProperty> properties = {PlyProperty("double", PlyDataType::Double)};
  CHECK(!detail::isMemcpyable<float>(properties.begin(), properties.end()));
  CHECK(detail::isMemcpyable<double>(properties.begin(), properties.end()));
}

TEST_CASE("Test isMemcpyable type trait for multiple properties", "[type_traits]")
{
  const std::vector<PlyProperty> properties = {
      PlyProperty("double", PlyDataType::Double), PlyProperty("float", PlyDataType::Float)};

  CHECK(detail::isMemcpyable<double, float>(properties.begin(), properties.end()));
  CHECK(!detail::isMemcpyable<float, double>(properties.begin(), properties.end()));
}

TEST_CASE("Test isMemcpyable type trait in combination with pack types", "[type_traits]")
{
  const std::vector<PlyProperty> properties = {
      PlyProperty("double", PlyDataType::Double), PlyProperty("float", PlyDataType::Float),
      PlyProperty("float", PlyDataType::Float), PlyProperty("float", PlyDataType::Float)};

  CHECK(detail::isMemcpyable<double, float, float, float>(properties.begin(), properties.end()));
  CHECK(detail::isMemcpyable<double, reflect::Pack<float, 3>>(properties.begin(), properties.end()));
  CHECK(!detail::isMemcpyable<double, reflect::Pack<float, 2>>(properties.begin(), properties.end()));
  CHECK(!detail::isMemcpyable<reflect::Pack<double, 4>>(properties.begin(), properties.end()));
}

TEST_CASE("Test isMemcpyable type trait in combination with arrays", "[type_traits]")
{
  {
    const std::vector<PlyProperty> properties = {
        PlyProperty("double", PlyDataType::Double), PlyProperty("float", PlyDataType::Float),
        PlyProperty("float", PlyDataType::Float), PlyProperty("float", PlyDataType::Float)};

    CHECK(detail::isMemcpyable<double, float, float, float>(properties.begin(), properties.end()));
    CHECK(!detail::isMemcpyable<double, reflect::Array<float, 3>>(properties.begin(), properties.end()));
    CHECK(!detail::isMemcpyable<double, reflect::Array<float, 2>>(properties.begin(), properties.end()));
    CHECK(!detail::isMemcpyable<reflect::Array<double, 4>>(properties.begin(), properties.end()));
  }

  // A list property can never be memcpy'd, since the source PLY data stores the
  // size of each list property along with the elements in the list itself.
  {
    const std::vector<PlyProperty> properties = {PlyProperty("float", PlyDataType::Float, PlyDataType::UInt)};

    CHECK(!detail::isMemcpyable<reflect::Array<float, 1>>(properties.begin(), properties.end()));
    CHECK(!detail::isMemcpyable<reflect::Array<float, 2>>(properties.begin(), properties.end()));
    CHECK(!detail::isMemcpyable<reflect::Array<float, 3>>(properties.begin(), properties.end()));
    CHECK(!detail::isMemcpyable<double, reflect::Array<float, 2>>(properties.begin(), properties.end()));
  }
}
