#ifndef PLYWOOT_REFLECT_HPP
#define PLYWOOT_REFLECT_HPP

#include <tuple>
#include <vector>

namespace plywoot { namespace reflect {

/// Meta type that is used to help with SFINAE resolution.
template<typename... Ts>
struct Void
{
  using type = void;
};

/// Type wrapper that wraps some type (`DestT`) that we need to serialize to. A
/// specialization exists that extracts the destination type from some of the
/// reflect helper types.
/// @{
template<typename T, typename = void>
struct Type
{
  using DestT = T;
};

template<typename T>
struct Type<T, typename Void<typename T::DestT>::type>
{
  using DestT = typename T::DestT;
};
/// @}

/// Can be embedded in a `Layout` type to read an element list property of some
/// fixed size `N`, with elements of type `T`. The size type is used in the PLY
/// format to store the type of the length of the list.
template<typename T, std::size_t N>
struct Array
{
  using DestT = T;
};

/// Can be embedded in a `Layout` type to skip an element property in the input
/// PLY file (only useful when reading data from a PLY stream).
struct Skip
{
};

/// Can be used in a `Layout` type to step over member variables in the
/// destination structure (only useful when reading data from a PLY stream).
template<typename T>
struct Stride
{
  using DestT = T;
};

/// Can be used in a `Layout` type to pack together a sequence of properties of
/// the same type, such that they will be parsed in one go, speeding up parsing.
template<typename T, std::size_t N>
struct Pack
{
  using DestT = T;
  static constexpr std::size_t size = N;
};

/// Used to define the layout of some structure that is either read from or
/// written to by the PLY I/O functions. Note that it is important that the
/// order of data types specified in the layout matches the order of the data
/// types in the data type that is read from or written to. This will
/// automatically take default C/C++ padding into account. In case not all
/// properties in some layout structure are written, use `reflect::Stride` to
/// skip properties. Properties at the end of the structure that are not read
/// from or written do not need to be specified and will automatically be
/// skipped.
template<typename... Ts>
class Layout : public std::tuple<Ts...>
{
public:
  /// Constructor for an empty layout (no data will be read/written).
  Layout() = default;

  /// Constructs a layout representation of some element, and specifies a target
  /// list of elements that will be written to by the PLY parser.
  template<typename T>
  Layout(std::vector<T> &v)
      : data_{reinterpret_cast<std::uint8_t *>(v.data())}, cdata_{data_}, size_{v.size()}
  {
  }

  /// Constructs a layout representation of some element, and specifies a target
  /// list of elements that will be read from by the PLY writer.
  template<typename T>
  Layout(const std::vector<T> &v)
      : data_{nullptr}, cdata_{reinterpret_cast<const std::uint8_t *>(v.data())}, size_{v.size()}
  {
  }

  /// Returns a pointer to the read-only memory area that contains `n` number of
  /// structures made up of the types associated with this layout.
  /// @{
  std::uint8_t *data() { return data_; }
  const std::uint8_t *data() const { return cdata_; }
  /// @}

  /// Returns the number of elements that are or may be stored in the memory
  /// block pointer to by the associated data block.
  std::size_t size() const { return size_; }

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
