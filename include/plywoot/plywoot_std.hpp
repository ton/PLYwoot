#ifndef PLYWOOT_STD_HPP
#define PLYWOOT_STD_HPP

#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>

namespace plywoot
{
  namespace pstd
  {
    inline size_t roundup(std::size_t num, std::size_t multiple)
    {
      std::size_t mod = num % multiple;
      return mod == 0 ? num : num + multiple - mod;
    }

    template<typename T>
    struct CharToInt
    {
      template<typename U>
      T operator()(U&& u) const { return u; }

      int operator()(signed char c) const { return static_cast<int>(c); }
      unsigned operator()(unsigned char c) const { return static_cast<unsigned>(c); }
    };

    template<typename Number>
    inline Number to_number(char *buf)
    {
      return std::atoi(buf);
    }

    template<>
    inline float to_number<>(char *buf)
    {
      return std::atof(buf);
    }

    template<>
    inline double to_number<double>(char *buf)
    {
      return std::atof(buf);
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
      size_type find(char c, size_type pos = 0)
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
  }
}

#endif
