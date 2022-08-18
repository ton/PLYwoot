#ifndef PLYWOOT_REFLECT_HPP
#define PLYWOOT_REFLECT_HPP

#include <tuple>

namespace plywoot { namespace reflect {

template<typename U, std::size_t N>
struct Array
{
  using value_type = U;
};

/// Can be embedded in a `Layout` type to skip an element property in the input
/// PLY file (only useful when reading data from a PLY stream).
struct Skip
{
};

/// Can be used in a `Layout` type to step over a member variables in the
/// destination structure (only useful when reading data from a PLY stream).
template<typename T>
struct Stride
{
};

/// Used to define the layout of some structure that is either read from or
/// written to by the PLY I/O functions.
// TODO
template<typename... Ts>
class Layout : public std::tuple<Ts...>
{
public:
  template<typename T>
  Layout(const std::vector<T> &v) : data_{reinterpret_cast<const std::uint8_t *>(v.data())}, size_{v.size()} {}
  Layout(const std::uint8_t *data, std::size_t size) : data_{data}, size_{size} {}

  const std::uint8_t *data() const { return data_; }
  std::size_t size() const { return size_; }

  constexpr std::size_t numProperties() const { return std::tuple_size<std::tuple<Ts...>>::value; }

private:
  /// Pointer to the memory area that contains `n` number of structures made up of
  /// the types associated with this layout.
  const std::uint8_t *data_{nullptr};
  /// Number of elements that are or may be stored in the memory block pointer
  /// to by the associated data block.
  std::size_t size_{0};
};

}}

#endif
