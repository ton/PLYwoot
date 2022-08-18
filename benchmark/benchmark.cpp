#include "miniply/miniply.h"

#include <plywoot/plywoot.hpp>

#include <benchmark/benchmark.h>

#include <cstdint>
#include <fstream>
#include <vector>

namespace {

struct Vertex
{
  float x, y, z;
};

struct Triangle
{
  std::int32_t a, b, c;
};

}

static void BM_plywoot_ascii(benchmark::State &state, const std::string &filename)
{
  benchmark::ClobberMemory();

  for (auto _ : state)
  {
    std::ifstream ifs{filename};
    if (!ifs) { state.SkipWithError((std::string{"could not open: "} + filename).data()); }

    plywoot::IStream plyIn{ifs};

    plywoot::PlyElement vertexElement;
    bool verticesFound{false};
    std::tie(vertexElement, verticesFound) = plyIn.element("vertex");
    if (!verticesFound)
    {
      state.SkipWithError("could not load vertex data; vertex element not found in input data");
    }

    plywoot::PlyElement faceElement;
    bool faceIndicesFound{false};
    std::tie(faceElement, faceIndicesFound) = plyIn.element("face");
    if (!faceIndicesFound)
    {
      state.SkipWithError("could not load triangle data; face element not found in input data");
    }

    const std::vector<Triangle> triangles{
        plyIn.read<Triangle, int, int, int>(faceElement.setSizeHint("vertex_indices", 3))};
    benchmark::DoNotOptimize(triangles);

    const std::vector<Vertex> vertices{plyIn.read<Vertex, float, float, float>(vertexElement)};
    benchmark::DoNotOptimize(vertices);
  }
}

static void BM_miniply_ascii(benchmark::State &state, const std::string &filename)
{
  benchmark::ClobberMemory();

  for (auto _ : state)
  {
    const int verts_per_face = 3;

    miniply::PLYReader reader{filename.data()};
    if (!reader.valid())
    {
      state.SkipWithError((std::string{"could not open: "} + filename).data());
    }

    miniply::PLYElement *facesElem =
        reader.get_element(reader.find_element(miniply::kPLYFaceElement));
    if (!facesElem)
    {
      state.SkipWithError("could not load triangle data; face element not found in input data");
    }

    std::vector<std::uint32_t> listIdxs;
    listIdxs.resize(3);
    facesElem->convert_list_to_fixed_size(
        facesElem->find_property("vertex_indices"), verts_per_face, listIdxs.data());

    std::vector<Triangle> triangles;
    std::vector<Vertex> vertices;

    bool gotVerts = false;
    bool gotFaces = false;
    while (reader.has_element() && (!gotVerts || !gotFaces))
    {
      if (!gotVerts && reader.element_is(miniply::kPLYVertexElement))
      {
        if (!reader.load_element())
        {
          state.SkipWithError("could not load vertex data; vertex element not found in input data");
        }

        uint32_t propIdxs[3];
        if (!reader.find_pos(propIdxs)) { break; }
        vertices.resize(reader.num_rows());
        reader.extract_properties(propIdxs, 3, miniply::PLYPropertyType::Float, vertices.data());
        gotVerts = true;
      }
      else if (!gotFaces && reader.element_is(miniply::kPLYFaceElement))
      {
        if (!reader.load_element())
        {
          state.SkipWithError("could not load triangle data; could not load face element");
        }

        triangles.resize(reader.num_rows());
        reader.extract_properties(
            listIdxs.data(), verts_per_face, miniply::PLYPropertyType::Int, triangles.data());
        gotFaces = true;
      }
      reader.next_element();
    }

    benchmark::DoNotOptimize(triangles);
    benchmark::DoNotOptimize(vertices);
  }
}

BENCHMARK_CAPTURE(BM_plywoot_ascii, "Stanford bunny (ASCII)", "benchmark/input/ascii/bunny.ply");

BENCHMARK_CAPTURE(BM_miniply_ascii, "Stanford bunny (ASCII)", "benchmark/input/ascii/bunny.ply");

BENCHMARK_MAIN();
