#include <plywoot/plywoot.hpp>

#include <fstream>
#include <iostream>

// This code is only here to test that compiling the library with only C++11
// support will work properly.
int main(int argc, char** argv)
{
  std::ifstream ifs{argv[1], std::ios::in};
  plywoot::PlyFile ply_file{ifs};
  std::cout << "Parsed " << ply_file.elements().size() << " elements.\n";
  return 0;
}
