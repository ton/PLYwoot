#ifndef PLYWOOT_STD_HPP
#define PLYWOOT_STD_HPP

#ifdef PLYWOOT_HAS_FAST_FLOAT
#include <fast_float/fast_float.h>
#endif

#ifdef PLYWOOT_HAS_FAST_INT
#include <fast_int/fast_int.hpp>
#endif

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <sstream>
#include <string>

namespace plywoot { namespace detail {

inline std::size_t roundup(std::size_t num, std::size_t multiple)
{
  const std::size_t mod = num % multiple;
  return mod == 0 ? num : num + multiple - mod;
}

/// Aligns the given input pointer. Implementation is taken from GCCs
/// `std::align` implementation. The given alignment value should be a power
/// of two.
inline void *align(void *ptr, std::size_t alignment)
{
  // Some explanation on the code below; -x is x in two's complement, which
  // means that an alignment value x of power two is converted to (~x + 1).
  // For example, for an alignment value of 4, this turns 0b000100 into
  // 0b111100. The factor (uintptr + alignment - 1u) guarantees that the
  // alignment bit is set unless (uintptr % alignment == 0).
  const auto uintptr = reinterpret_cast<uintptr_t>(ptr);
  return reinterpret_cast<void *>((uintptr + alignment - 1u) & -alignment);
}

/// Aligns the given input pointer. Implementation is taken from GCCs
/// `std::align` implementation. The given alignment value should be a power
/// of two.
inline const void *align(const void *ptr, std::size_t alignment)
{
  // Some explanation on the code below; -x is x in two's complement, which
  // means that an alignment value x of power two is converted to (~x + 1).
  // For example, for an alignment value of 4, this turns 0b000100 into
  // 0b111100. The factor (uintptr + alignment - 1u) guarantees that the
  // alignment bit is set unless (uintptr % alignment == 0).
  const auto uintptr = reinterpret_cast<uintptr_t>(ptr);
  return reinterpret_cast<const void *>((uintptr + alignment - 1u) & -alignment);
}

template<typename Number>
inline Number to_number(const char *first, const char *last, const char **end)
{
#ifdef PLYWOOT_HAS_FAST_INT
  Number n{};
  *end = fast_int::from_chars(first, last, n).ptr;
  return n;
#else
  return std::strtoll(first, const_cast<char **>(end), 10);
#endif
}

template<>
inline float to_number<>(const char *first, const char *last, const char **end)
{
#ifdef PLYWOOT_HAS_FAST_FLOAT
  float x;
  *end = fast_float::from_chars(first, last, x).ptr;
  return x;
#else
  return std::strtof(first, const_cast<char **>(end));
#endif
}

template<>
inline double to_number<double>(const char *first, const char *last, const char **end)
{
#ifdef PLYWOOT_HAS_FAST_FLOAT
  double x;
  *end = fast_float::from_chars(first, last, x).ptr;
  return x;
#else
  return std::strtof(first, const_cast<char **>(end));
#endif
}

/// Returns whether the given string starts with the given prefix.
inline bool starts_with(const std::string &s, const char *prefix) { return s.rfind(prefix, 0) == 0; }

/// Simple string-view like type; very basic, do not use for anything serious.
class string_view
{
public:
  using size_type = std::size_t;

  string_view() noexcept : data_{nullptr}, size_{0} {}
  string_view(const char *first, const char *last) noexcept
      : data_{first}, size_{static_cast<size_type>(last - first)}
  {
  }

  // Returns the first character in this string view.
  char front() const noexcept { return *data_; }
  // Returns the last character in this string view.
  char back() const noexcept { return *(data_ + size_ - 1); }

  // Returns a pointer to the underlying character array.
  const char *data() const noexcept { return data_; }

  // Compares this string view with the given string. Behavior is the same
  // as std::string::compare().
  int compare(const char *s) const noexcept
  {
    const size_type otherSize = std::strlen(s);
    const int result = std::strncmp(data_, s, std::min(size_, otherSize));
    return result == 0 && size_ == otherSize ? 0 : (size_ < otherSize ? -1 : 1);
  }

  // Returns the first position of the given character in this string_view.
  size_type find(char c)
  {
    const char *first = static_cast<const char *>(::memchr(data_, c, size_));
    return first != nullptr ? std::distance(data_, first) : std::string::npos;
  }

  // Returns the number characters in this string view.
  size_type size() const noexcept { return size_; }

private:
  const char *data_;
  std::size_t size_;
};

template<typename T>
std::string to_string(const T &t)
{
  std::stringstream oss;
  oss << t;
  return oss.str();
}

}}

#endif
