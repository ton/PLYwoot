#ifndef PLYWOOT_TEST_TYPES_HPP
#define PLYWOOT_TEST_TYPES_HPP

#include <ostream>

template<typename T>
struct VertexT
{
  T x, y, z;

  bool operator==(const VertexT &v) const { return x == v.x && y == v.y && z == v.z; }
  bool operator!=(const VertexT &v) const { return !(*this == v); }
};

using FloatVertex = VertexT<float>;
using DoubleVertex = VertexT<double>;

struct Triangle
{
  int a, b, c;

  bool operator==(const Triangle &f) const { return a == f.a && b == f.b && c == f.c; }
};

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const VertexT<T> &v)
{
  return os << '(' << v.x << ", " << v.y << ", " << v.z << ')';
}

inline std::ostream &operator<<(std::ostream &os, const Triangle &t)
{
  return os << '[' << t.a << ", " << t.b << ", " << t.c << ']';
}

#endif
