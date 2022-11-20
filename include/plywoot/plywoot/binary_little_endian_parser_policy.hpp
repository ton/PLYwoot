#ifndef PLYWOOT_BINARY_LITTLE_ENDIAN_PARSER_POLICY_HPP
#define PLYWOOT_BINARY_LITTLE_ENDIAN_PARSER_POLICY_HPP

#include "buffered_istream.hpp"

#include <map>
#include <vector>

namespace plywoot { namespace detail {

/// Defines a parser policy that deals with binary input streams.
class BinaryLittleEndianParserPolicy
{
public:
  /// Constructs a binary little endian parser policy.
  BinaryLittleEndianParserPolicy(BufferedIStream &is, const std::vector<PlyElement> &elements)
      : is_{is}, elements_{elements}
  {
  }

  /// Seeks to the start of the data for the given element. Returns whether
  /// seeking was successful.
  bool seekTo(const PlyElement &element) const
  {
    std::size_t numBytes{0};
    auto first{elements_.begin()};
    const auto last{elements_.end()};
    while (first != last && *first != element) { numBytes += elementSizeInBytes(*first++); }

    if (first != last && *first == element)
    {
      is_.seekToBegin();
      is_.skip(numBytes);
    }

    return first != last;
  }

  /// Reads a number of the given type `T` from the input stream.
  template<typename T>
  T readNumber() const
  {
    return is_.read<T>();
  }

  /// Skips a number of the given type `T` in the input stream.
  template<typename T>
  void skipNumber() const
  {
    is_.skip(sizeof(T));
  }

  /// Skips the data of all properties from `first` to `last` in the input
  /// stream.
  void skipProperties(PlyPropertyConstIterator first, PlyPropertyConstIterator last) const
  {
    while (first != last)
    {
      is_.skip(sizeOf(first->isList() ? first->sizeType() : first->type()));
      ++first;
    }
  }

private:
  /// Calculates and returns the size in bytes of the given PLY element. Uses
  /// memoization; the size of every unique element is only calculated once
  size_t elementSizeInBytes(const PlyElement &element) const
  {
    auto it = elementSize_.lower_bound(element.name());
    if (it == elementSize_.end() || it->first != element.name())
    {
      std::size_t numBytes{0};
      for (const PlyProperty &p : element.properties())
      {
        if (!p.isList()) { numBytes += element.size() * sizeOf(p.type()); }
        else
        {
          is_.seekToBegin();
          is_.skip(numBytes);

          std::size_t sizeSum = 0;
          for (size_t i = 0; i < element.size(); ++i)
          {
            std::size_t size = 0;
            switch (p.sizeType())
            {
              case PlyDataType::Char:
                size = is_.read<char>();
                break;
              case PlyDataType::UChar:
                size = is_.read<unsigned char>();
                break;
              case PlyDataType::Short:
                size = is_.read<short>();
                break;
              case PlyDataType::UShort:
                size = is_.read<unsigned short>();
                break;
              case PlyDataType::Int:
                size = is_.read<int>();
                break;
              case PlyDataType::UInt:
                size = is_.read<unsigned int>();
                break;
              case PlyDataType::Float:
                size = is_.read<float>();
                break;
              case PlyDataType::Double:
                size = is_.read<double>();
                break;
            }

            sizeSum += size;
            is_.skip(size * sizeOf(p.type()));
          }

          numBytes += element.size() * sizeOf(p.sizeType()) + sizeSum * sizeOf(p.type());
        }
      }
      it = elementSize_.insert(it, std::make_pair(element.name(), numBytes));
    }
    return it->second;
  }

  mutable detail::BufferedIStream is_;
  const std::vector<PlyElement> &elements_;
  mutable std::map<std::string, std::ptrdiff_t> elementSize_;
};

}}

#endif
