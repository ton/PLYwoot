#ifndef PLYWOOT_REFLECT_HPP
#define PLYWOOT_REFLECT_HPP

#include <tuple>

namespace plywoot { namespace reflect {

template<typename... Ts>
struct Void
{
  typedef void type;
};

/// Type wrapper that wraps some type (`DestT`) that we need to serialize to. A
/// specialization exists that extracts the destination type from some of the
/// reflect helper types.
/// @{
template<typename DestT, typename = void>
struct Type
{
  using dest_type = DestT;
};

template<typename HelperT>
struct Type<HelperT, typename Void<typename HelperT::dest_type>::type>
{
  using dest_type = typename HelperT::dest_type;
};
/// @}

/// Can be embedded in a `Layout` type to read an element list property of some
/// fixed size `N`, with elements of type `T`. The size type is used in the PLY
/// format to store the type of the length of the list.
template<typename T, std::size_t N, typename SizeT>
struct Array
{
  using dest_type = T;
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
  using dest_type = T;
};

/// Used to define the layout of some structure that is either read from or
/// written to by the PLY I/O functions.
// TODO
template<typename... Ts>
class Layout : public std::tuple<Ts...>
{
public:
  template<typename T>
  Layout(std::vector<T> &v)
      : data_{reinterpret_cast<std::uint8_t *>(v.data())}, cdata_{data_}, size_{v.size()}
  {
  }
  template<typename T>
  Layout(const std::vector<T> &v)
      : data_{nullptr}, cdata_{reinterpret_cast<const std::uint8_t *>(v.data())}, size_{v.size()}
  {
  }

  std::uint8_t *data() { return data_; }
  const std::uint8_t *data() const { return cdata_; }

  std::size_t size() const { return size_; }

  constexpr std::size_t numProperties() const { return std::tuple_size<std::tuple<Ts...>>::value; }

private:
  /// Pointer to the writable memory area that contains `n` number of structures
  /// made up of the types associated with this layout.
  std::uint8_t *data_{nullptr};
  /// Pointer to the read-only memory area that contains `n` number of
  /// structures made up of the types associated with this layout.
  const std::uint8_t *cdata_{nullptr};
  /// Number of elements that are or may be stored in the memory block pointer
  /// to by the associated data block.
  std::size_t size_{0};
};

}}

#endif
