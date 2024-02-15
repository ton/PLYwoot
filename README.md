# PLYwoot

PLYwoot is a C++17 header-only library providing read/write support for [PLY](https://paulbourke.net/dataformats/ply/) files. A major design goal of PLYwoot was to provide an [iostreams](https://en.cppreference.com/w/cpp/io) based interface without sacrificing performance.

## Features

* **Fast!** In fact, the fastest PLY parser and writer out there according to my own benchmarks :D Refer to the benchmark results at [PLYbench](https://github.com/ton/PLYbench) to see how PLYwoot stacks up to other PLY parsers in terms of both read and write performance.
* Header-only, can be directly embedded in your existing C++17 project. There are some optional [dependencies](#dependencies) to speed up the parsing of ASCII PLY files.
* Read/write support for ASCII, binary little endian, and binary big endian PLY files.
* Direct compile-time mapping of element data in the PLY file to your own types, with implicit type conversion support.
* Allows skipping of properties in the PLY data that are not of interest.
* `rePLY`, a separate tool bundled with PLYwoot allows converting PLY files from one format (ASCII, binary little/big endian) to another.

## Examples

The following examples demonstrate some typical use cases of PLYwoot. For more details on the functions used below, please refer to the [API](#api) documentation.

### Parsing PLY files

Suppose you have a triangle mesh type, that has the following structure:

```cpp
```

### Writing PLY files

## Getting started

Since PLYwoot is header-only, all that is needed is to copy the PLYwoot sources into your project and include `<plywoot/plywoot.hpp>` (taking into account license constraints of course). To build rePLY, a tool to convert PLY files between different formats (ASCII, binary little/big endian), PLYwoot can be built as follows, assuming you have at least CMake version 3.5 installed (see [dependencies](#dependencies).

In case you are using a CMake based project and would like to depend on a system-wide installation of PLYwoot, use the following steps to build the unit tests, rePLY and install PLYwoot:

```sh
$ cmake -DCMAKE_BUILD_TYPE=Release -B build
$ cd build && make install
```

### Using PLYwoot in a CMake project

Assuming PLYwoot has been installed using the previous steps, you should be able to depend on PLYwoot in your CMake project as follows, without having to include the PLYwoot sources in your project:

```cmake
find_package(PLYwoot REQUIRED)
```

PLYwoot exports one target named `PLYwoot::plywoot` which represents the header-only library to depend on.

### Dependencies

To be able to build the unit tests of PLYwoot and the rePLY tool, [CMake](https://cmake.org) is required (at least version 3.5). The unit tests are implemented using the [Catch2](https://github.com/catchorg/Catch2) unit test framework.

By default, PLYwoot will use functionality from C++'s standard library to perform string to floating point and integer conversion for parsing of ASCII PLY files. Performance of parsing ASCII PLY files can be improved significantly by ensuring that the [`fast_float`](https://github.com/fastfloat/fast_float) and/or [`fast_int`](https://github.com/ton/fast_int) libraries are installed.

## Benchmarks

Please refer to the benchmark results at [PLYbench](https://github.com/ton/PLYbench) to see how PLYwoot stacks up to other PLY parsers in terms of both read and write performance. That repository provides all the tools needed to reproduce the benchmark results on your machine, and see how PLYwoot performs on your particular hardware.

## Known issues

## Planned features

The following features are on my list to be implemented for future versions of PLYwoot:

* Add support to create an 'amalgamation' header file of all combined header-files of PLYwoot for easier inclusion in other projects, similar to how SQLite does this for example.
* Make the `Layout` type smart enough such that the helper type `Pack` is no longer needed to ensure consequent properties of the same type are efficiently `memcpy`'d when possible.

## License

PLYwoot is licensed under GPLv3. See [LICENSE](LICENSE) for more details.
