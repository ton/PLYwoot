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

}

#endif
