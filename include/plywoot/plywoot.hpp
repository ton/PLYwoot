#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include "plywoot_exceptions.hpp"
#include "plywoot_header_parser.hpp"
#include "plywoot_header_scanner.hpp"
#include "plywoot_std.hpp"

#include <istream>
#include <vector>

namespace plywoot
{
  class PlyFile
  {
  public:
    PlyFile(std::istream &is) : parser_{is} {}

    const std::vector<PlyElement> &elements() const { return parser_.elements(); }

  private:
    void parseHeader();

    HeaderParser parser_;
  };
}

#endif
