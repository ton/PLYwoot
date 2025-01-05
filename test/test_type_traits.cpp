/*
   This file is part of PLYwoot, a header-only PLY parser.

   Copyright (C) 2023-2025, Ton van den Heuvel

   PLYwoot is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
