#ifndef PLYWOOT_EXCEPTIONS_HPP
#define PLYWOOT_EXCEPTIONS_HPP

#include <fmt/format.h>

#include <stdexcept>
#include <string>

namespace plywoot {

/// Base class for all exceptions thrown by plywoot.
class Exception : public std::runtime_error
{
public:
  Exception(const std::string &message) : std::runtime_error(message) {}
};

/// Missing size hint exception is thrown in case the user request to write
/// elements with lists that have no size hint size. For now, it is not possible
/// to write dynamic lists.
struct MissingSizeHint : Exception
{
  MissingSizeHint(const std::string &propertyName)
      : Exception(std::string{"missing size hint for property '"} + propertyName + "'")
  {
  }
};

}

#endif
