/*
   This file is part of PLYwoot, a header-only PLY parser.

   Copyright (C) 2023-2024, Ton van den Heuvel

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

#include <plywoot/plywoot.hpp>

#include <fstream>
#include <iostream>

// This code is only here to test that compiling the library with only C++11
// support will work properly.
int main(int argc, char **argv)
{
  std::ifstream ifs{argv[1], std::ios::in};
  plywoot::IStream plyFile{ifs};
  std::cout << "Parsed " << plyFile.elements().size() << " elements.\n";
  return 0;
}
