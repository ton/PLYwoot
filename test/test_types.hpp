#ifndef PLYWOOT_TEST_TYPES_HPP
#define PLYWOOT_TEST_TYPES_HPP

template<typename T>
struct VertexT
{
  T x, y, z;

  bool operator==(const VertexT &v) const { return x == v.x && y == v.y && z == v.z; }
  bool operator!=(const VertexT &v) const { return !(*this == v); }
};

using FloatVertex = VertexT<float>;
using DoubleVertex = VertexT<double>;

struct Face
{
  int a, b, c;

  bool operator==(const Face &f) const { return a == f.a && b == f.b && c == f.c; }
};

#endif
