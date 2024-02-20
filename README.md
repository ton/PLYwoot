# PLYwoot

![Doxygen action status](https://github.com/ton/PLYwoot/actions/workflows/doxygen.yml/badge.svg)
![Unit test action status](https://github.com/ton/PLYwoot/actions/workflows/unittest.yml/badge.svg)

PLYwoot is a C++17 header-only library providing read/write support for [PLY](https://paulbourke.net/dataformats/ply/) files. A major design goal of PLYwoot was to provide an [iostreams](https://en.cppreference.com/w/cpp/io) based interface without sacrificing performance.

## Features

* **Fast!** In fact, the fastest PLY parser and writer out there according to my own benchmarks :D Refer to the benchmark results at [PLYbench](https://github.com/ton/PLYbench) to see how PLYwoot stacks up to other PLY parsers in terms of both read and write performance.
* Header-only, can be directly embedded in your existing C++17 project. There are some optional [dependencies](#Dependencies) to speed up the parsing of ASCII PLY files.
* Read/write support for ASCII, binary little endian, and binary big endian PLY files.
* Direct compile-time mapping of element data in the PLY file to your own types, with implicit type conversion support.
* Allows skipping of properties in the PLY data that are not of interest.
* `rePLY`, a separate tool bundled with PLYwoot allows converting PLY files from one format (ASCII, binary little/big endian) to another.

## Getting started

Since PLYwoot is header-only, all that is needed is to copy the PLYwoot sources into your project and [`#include <plywoot/plywoot.hpp>`](include/plywoot/plywoot.hpp) (taking into account license constraints of course). To build `rePLY`, a tool to convert PLY files between different formats (ASCII, binary little/big endian), PLYwoot can be built as follows, assuming you have at least CMake version 3.5 installed (see [dependencies](#Dependencies)).

In case you are using a CMake based project and would like to depend on a system-wide installation of PLYwoot, use the following steps to build the unit tests, `rePLY` and install PLYwoot:

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

## Parsing PLY files

This section will demonstrate how to use PLYwoot to parse a PLY file for the typical use case of parsing triangle mesh data. For more details on the functions used below, please refer to the [API documentation](https://ton.github.io/PLYwoot).

Suppose we have the following two very *naive* types `Triangle` and `Vertex` to represent a triangle and vertex in a triangle mesh:

```cpp
struct Triangle
{
    std::vector<std::int32_t> indices;
};

struct Vertex
{
    double x, y, z;
};
```

Furthermore, suppose we have a PLY file that contains the following element data, that is, it contains 1612868 vertices containing an (`x`, `y`, `z`) tuple representing the vertex coordinates together with an RGB tuple representing the vertex color. Finally, the PLY file contains 3224192 triangles represented by indices into the vertex list. PLY does not support fixed lists, so the triangle data is stored using variable length lists, which in practice will always be a list of three elements. PLYwoot provides functionality to pass on this assumption to the parser to speed up parsing.

```ply
element vertex 1612868
property float x
property float y
property float z
property uchar red
property uchar green
property uchar blue
element face 3224192
property list uchar int vertex_indices
```

### Parsing the vertex element

PLYwoot allows you to directly map the `x`, `y`, and `z` properties on the `Vertex` type, the so-called 'target type', and ignore the color data. For that, PLYwoot needs to know the memory layout of the `Vertex` type. Reflection is not yet standardized in C++, so we have to come up with a work-around to pass on this information to PLYwoot. PLYwoot provides a `plywoot::reflect::Layout` type which enables specifying the mapping of PLY properties onto member types of the `Vertex` struct in this case. This is done using a `plywoot::reflect::Layout` template type. For example, to create a property map for vertex element in the PLY data above onto the `Vertex` type, the following layout type can be specified:

```cpp
using namespace plywoot::reflect;
using VertexLayout = Layout<double, double, double, Skip, Skip, Skip>;
```

This tells PLYwoot to map the first three properties in the PLY element, that is the three float properties `x`, `y`, and `z` respectively onto `double` member variables. We are not interested in the `red`, `green`, and `blue` properties in the PLY data, so those can be skipped when parsing the PLY data, which is indicated by the last three `plywoot::reflect::Skip` types. `plywoot::reflect::Skip` types occuring at the end of a layout specification do not necessarily need to be specified, PLYwoot is smart enough to skip any extra element properties for which no mapping was defined, so the `VertexLayout` type can be further simplified to:

```cpp
using namespace plywoot::reflect;
using VertexLayout = Layout<double, double, double>;
```

Now, suppose we have some input stream `is` containing our PLY data, then the `vertex` element in the PLY data can now be parsed as follows:

```cpp
using VertexLayout = plywoot::reflect::Layout<double, double, double>;

plywoot::IStream ply_is{is};
const std::vector<Vertex> vertices = ply_is.readElement<Vertex, VertexLayout>();
```

#### Improving read performance by packing properties

PLYwoot will copy each individual PLY property separately in the last code fragment that we saw. This can be improved a bit further. In case we know that the struct member variables are laid out consecutively in memory using standard C++ alignment rules, the three `x`, `y`, and `z` PLY properties can be directly `memcpy`'d into each `Vertex` instance. This can be done by packing the three target member types into a `plywoot::reflect::Pack` type as follows:

```cpp
using VertexLayout = plywoot::reflect::Layout<plywoot::reflect::Pack<double, 3>>;
```

This tells PLYwoot that three consecutive PLY properties should be mapped on `double` target member types. Parsing performance is greatly improved in this way, as the three PLY properties `x`, `y`, and `z` will be `memcpy`'d at the same time into a `Vertex` instance. In principle, `plywoot::reflect::Pack` could be an implementation detail in the sense that PLYwoot could be smart enough to do automatic packing of target member types, but this has not been implemented yet.

#### Skipping over properties in the target type

As we saw, using `plywoot::reflect::Skip` can be used to skip over unwanted PLY property data. But suppose we would like to skip over a member variable in the target type, this can be done as well. For example, suppose our `Vertex` type has a slightly different form, where each vertex also stores a UV-coordinate. For the sake of this argument, assume the UV-coordinate is laid out before the X, Y, Z coordinate in memory:

```cpp
struct Vertex
{
    double u, v;
    double x, y, z;
};
```

The UV-coordinates can not be initialized directly from the PLY data that we saw earlier. For this, PLYwoot provides a `plywoot::reflect::Stride` type, that allows skipping over types in the target type, as follows:

```cpp
using namespace plywoot::reflect;
using VertexLayout = Layout<Stride<double>, Stride<double>, Pack<double, 3>>;
```

### Parsing the triangle element

To parse the triangle data in the PLY file listed above to our target type `Triangle`, the following layout can be used:

```cpp
using namespace plywoot::reflect;
using TriangleLayout = Layout<std::vector<std::int32_t>>;
```

This tells PLYwoot that the first property in the PLY `face` element which is a variable length list `vertex_indices` needs to be mapped onto an `std::vector` instance. This works, but is not very efficient. PLYwoot will not make any assumption on the length of the lists in the input data, and will have to initialize a vector of indices for each triangle it reads. Typically, a triangle type will have the following form:

```cpp
struct Triangle
{
    std::int32_t a, b, c;
};
```

In case we know up front that each face in the PLY data is encoded by three vertex indices, this assumption can be embedded in the layout map by mapping the PLY list property using `plywoot::reflect::Array`, as follows:

```cpp
using namespace plywoot::reflect;
using TriangleLayout = Layout<Array<std::int32_t, 3>>;
```

`plywoot::reflect::Array` is very similar to `plywoot::reflect::Pack` we saw before, except that it maps a *single list* PLY property onto the target type, instead of multiple PLY properties at once. As long as the target type holds one or more member types that have the same memory representation as the list data in the PLY file, `plywoot::reflect::Array` can be used. Thus, in case `Triangle` has the form below, the above `TriangleLayout` type will still work, since the memory representation of the two forms of `Triangle` are the same:

```cpp
struct Triangle
{
    std::array<std::int32_t, 3> indices;
};
```

This way of mapping the PLY list property will improve parser performance dramatically. In this case, the PLY list type exactly matches the target member type (`std::int32_t`), and as such the triangle data will be directly `memcpy`'d into the result vector. Note that implicit type conversions are still supported in this way. In case `Triangle` has the following form:

```cpp
struct Triangle
{
    std::array<std::uint32_t, 3> indices;
};
```

Thus, an array with elements of type `std::uint32_t` instead of `std::int32_t`, the `TriangleLayout` defined above needs to be adapted to read:

```cpp
using namespace plywoot::reflect;
using TriangleLayout = Layout<Array<std::uint32_t, 3>>;
```

Then, reading the same PLY data will still work, and PLYwoot will take care of the implicit type conversion from a signed integer in the PLY data to an unsigned integer in the target type. Just note that a direct `memcpy` is then no longer performed. The latter restriction may be too tight in some cases, and future PLYwoot versions can improve on this.

## Writing PLY files

Writing PLY files makes use of the same reflection mechanism used for parsing PLY files. You will need to tell PLYwoot how your source type maps onto PLY properties in the resulting PLY file. Suppose we would like to write out a list of vertices and triangles to a PLY file with the following elements and respective properties:

```ply
element vertex
property float x
property float y
property float z
element face
property list uchar int vertex_indices
```

To do this, first define the elements and properties:

```cpp
std::vector<Vertex> vertices;
std::vector<Triangle> triangles;

const plywoot::PlyProperty x{"x", plywoot::PlyDataType::Float};
const plywoot::PlyProperty y{"y", plywoot::PlyDataType::Float};
const plywoot::PlyProperty z{"z", plywoot::PlyDataType::Float};
const plywoot::PlyElement vertex_element{"vertex", vertices.size(), {x, y, z}};

const plywoot::PlyProperty vertex_indices{"vertex_indices", plywoot::PlyDataType::Int, plywoot::PlyDataType::UChar};
const plywoot::PlyElement face_element{"face", triangles.size(), {vertex_indices}};
```

Then, define the source layout in a similar way as was done for parsing the PLY data:

```cpp
using namespace plywoot::reflect;
using VertexLayout = Layout<Pack<double, 3>>;
using TriangleLayout = Layout<Pack<int, 3>>;
```

Finally, add the elements together with the data to be written for that element to a `plywoot::OStream`, and write the data to some `std::ostream` instance as follows:

```cpp
plywoot::OStream ply_os{plywoot::PlyFormat::Ascii}
ply_os.add(vertex_element, VertexLayout{vertices});
ply_os.add(face_element, TriangleLayout{triangles});

std::ofstream ofs{"output.ply", std::ios::out | std::ios::trunc}
ply_os.write(ofs);
```

Note that the data to be written is passed in as an argument to the `plywoot::reflect::Layout` type encoding the source to PLY property type mapping. In this case, an ASCII PLY file is written. Binary little and big endian output format types are supported as well.

## Dependencies

To be able to build the unit tests of PLYwoot and the `rePLY` tool, [CMake](https://cmake.org) is required (at least version 3.5). The unit tests are implemented using the [Catch2](https://github.com/catchorg/Catch2) unit test framework. One of the unit tests depends on [Boost](https://www.boost.org) to implement reading PLY data from a compressed stream.

By default, PLYwoot will use functionality from C++'s standard library to perform string to floating point and integer conversion for parsing of ASCII PLY files. Performance of parsing ASCII PLY files can be improved significantly by ensuring that the [`fast_float`](https://github.com/fastfloat/fast_float) and/or [`fast_int`](https://github.com/ton/fast_int) libraries are installed.

## API documentation

The complete API documentation generated by [Doxygen](https://www.doxygen.nl) can be found at: [https://ton.github.io/PLYwoot](https://ton.github.io/PLYwoot). The API documentation also contains various examples and use-cases that illustrate PLYwoot's usage.

## Benchmarks

Please refer to the benchmark results at [PLYbench](https://github.com/ton/PLYbench) to see how PLYwoot stacks up to other PLY parsers in terms of both read and write performance. That repository provides all the tools needed to reproduce the benchmark results on your machine, and see how PLYwoot performs on your particular hardware.

## Planned features

The following features are on my list to be implemented for future versions of PLYwoot:

* Add support to create an 'amalgamation' header file of all combined header-files of PLYwoot for easier inclusion in other projects, similar to how SQLite does this for example.
* Make the `Layout` type smart enough such that the helper type `Pack` is no longer needed to ensure consecutive properties of the same type are efficiently `memcpy`'d when possible.

## License

PLYwoot is licensed under GPLv3. See [LICENSE](./LICENSE) for more details.
