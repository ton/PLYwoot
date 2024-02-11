#ifndef PLYWOOT_ELEMENT_DATA_HPP
#define PLYWOOT_ELEMENT_DATA_HPP

#include "types.hpp"

#include <memory>
#include <vector>

namespace plywoot {

/// Abstracts a block of memory representing storing all data in a PLY element
/// mapping the data types used in the PLY file directly on native types.
class PlyElementData
{
public:
  /// Default constructor.
  PlyElementData() = default;

  /// Constructs an element data block for the given PLY element.
  PlyElementData(const PlyElement &element) : element_{element}, data_{allocateData()} {}

  /// Destructor.
  ~PlyElementData()
  {
    // Deallocate the `std::vector` instance holding variable list data.
    uint8_t *offset = data_.get();
    for (std::size_t i = 0; i < element_.size(); ++i)
    {
      for (PlyProperty property : element_.properties())
      {
        if (property.isList())
        {
          switch (property.type())
          {
            case PlyDataType::Char:
              reinterpret_cast<std::vector<char> *>(offset)->~vector<char>();
              break;
            case PlyDataType::UChar:
              reinterpret_cast<std::vector<unsigned char> *>(offset)->~vector<unsigned char>();
              break;
            case PlyDataType::Short:
              reinterpret_cast<std::vector<short> *>(offset)->~vector<short>();
              break;
            case PlyDataType::UShort:
              reinterpret_cast<std::vector<unsigned short> *>(offset)->~vector<unsigned short>();
              break;
            case PlyDataType::Int:
              reinterpret_cast<std::vector<int> *>(offset)->~vector<int>();
              break;
            case PlyDataType::UInt:
              reinterpret_cast<std::vector<unsigned int> *>(offset)->~vector<unsigned int>();
              break;
            case PlyDataType::Float:
              reinterpret_cast<std::vector<float> *>(offset)->~vector<float>();
              break;
            case PlyDataType::Double:
              reinterpret_cast<std::vector<double> *>(offset)->~vector<double>();
              break;
          }

          // Note; the exact element type does not matter for the following types
          // (or does it...?).
          offset = detail::align(offset + sizeof(std::vector<int>), alignof(std::vector<int>));
        }
        else { offset = detail::align(offset + detail::sizeOf(property.type()), property.type()); }
      }
    }
  }

  // This type is non-copyable.
  PlyElementData(const PlyElementData &) = delete;
  PlyElementData &operator=(const PlyElementData &) = delete;

  /// Move constructor.
  PlyElementData(PlyElementData &&x) : element_{std::move(x.element_)}, data_{x.data_.release()} {}
  /// Move assignment operator.
  PlyElementData &operator=(PlyElementData &&x)
  {
    element_ = std::move(x.element_);
    data_.reset(x.data_.release());
    return *this;
  }

  /// Returns the associated element definition.
  const PlyElement &element() const { return element_; }

  /// Returns a pointer to the memory block storing element data.
  std::uint8_t *data() const { return data_.get(); }

private:
  /// Calculates the total size in bytes of the data block to represent all data
  /// in the given PLY element. Note that lists are mapped on to `std::vector`,
  /// so we can pre-calculate the data size of the data block.
  std::uint8_t *allocateData() const
  {
    size_t bytesPerElement = 0;
    for (PlyProperty property : element_.properties())
    {
      if (property.isList())
      {
        // Note, the exact type of the vector element does not matter here.
        bytesPerElement =
            detail::align(bytesPerElement + sizeof(std::vector<int>), alignof(std::vector<int>));
      }
      else
      {
        bytesPerElement = detail::align(bytesPerElement + detail::sizeOf(property.type()), property.type());
      }
    }

    return new std::uint8_t[element_.size() * bytesPerElement];
  }

  PlyElement element_;
  std::unique_ptr<std::uint8_t[]> data_;
};

}

#endif
