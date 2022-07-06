#ifndef UTIL_HPP
#define UTIL_HPP

#include <catch2/matchers/catch_matchers_exception.hpp>
#include <catch2/matchers/catch_matchers_predicate.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace Catch::Matchers;

namespace
{
  class ExceptionMessageContainsMatcher final : public MatcherBase<std::exception>
  {
    std::string m_messagePart;

  public:
    ExceptionMessageContainsMatcher(const std::string &messagePart) : m_messagePart{messagePart} {}

    bool match(const std::exception &e) const override
    {
      return ::strstr(e.what(), m_messagePart.data()) != nullptr;
    }

    std::string describe() const override
    {
      return "exception message contains \"" + m_messagePart + '"';
    }
  };

  ExceptionMessageContainsMatcher MessageContains(const std::string &message) { return {message}; }
}

#endif
