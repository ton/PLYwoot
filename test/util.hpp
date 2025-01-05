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

#ifndef PLYWOOT_TEST_UTIL_HPP
#define PLYWOOT_TEST_UTIL_HPP

#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace Catch::Matchers;

namespace {

class ExceptionMessageContainsMatcher final : public MatcherBase<std::exception>
{
  std::string m_messagePart;

public:
  ExceptionMessageContainsMatcher(const std::string &messagePart) : m_messagePart{messagePart} {}

  bool match(const std::exception &e) const override
  {
    return ::strstr(e.what(), m_messagePart.data()) != nullptr;
  }

  std::string describe() const override { return "exception message contains \"" + m_messagePart + '"'; }
};

inline ExceptionMessageContainsMatcher MessageContains(const std::string &message) { return {message}; }

}

#endif
