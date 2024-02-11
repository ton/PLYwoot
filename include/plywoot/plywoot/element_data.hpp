#ifndef PLYWOOT_ELEMENT_DATA_HPP
#define PLYWOOT_ELEMENT_DATA_HPP

#include "types.hpp"

#include <algorithm>
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

  /// Constructs an element data block for the given PLY element. Calculates the
  /// total size in bytes of the data block to represent all data in the given
  /// PLY element. Note that lists are mapped on to `std::vector`, so we can
  /// pre-calculate the data size of the data block.
  explicit PlyElementData(const PlyElement &element) : element_{element}
  {
    // Keep track of the alignment requirements of the memory block that will
    // store all element data. Store the relative offsets of every manually
    // allocated `std::vector` in the element data.
    const std::vector<PlyProperty> &properties = element_.properties();
    for (const PlyProperty &property : properties)
    {
      if (property.isList())
      {
        // Note, the exact type of the vector element does not matter here.
        listOffsets_.push_back(detail::align(bytesPerElement_, alignof(std::vector<int>)));
        bytesPerElement_ = listOffsets_.back() + sizeof(std::vector<int>);
        alignment_ = std::max(alignment_, alignof(std::vector<int>));
      }
      else
      {
        bytesPerElement_ = detail::align(bytesPerElement_, property.type()) + detail::sizeOf(property.type());
        alignment_ = std::max(alignment_, detail::alignof_(property.type()));
      }
    }

    // Consecutive instances of an element in the memory block are aligned given
    // the maximum alignment requirement of an individual property type in an
    // element. The overall alignment requirements of an element determine the
    // overall size of the element in bytes.
    bytesPerElement_ = detail::align(bytesPerElement_, alignment_);

    // Now, allocate a raw block of memory large enough to hold all element
    // data.
    data_ = std::unique_ptr<std::uint8_t[]>(new std::uint8_t[element_.size() * bytesPerElement_]);

    // Finally, allocate all vectors holding the variable length list
    // properties.
    auto listOffset = listOffsets_.begin();
    for (PlyProperty property : element_.properties())
    {
      if (property.isList())
      {
        // Note, the exact type of the vector element does not matter here.
        std::uint8_t *ptr = data_.get() + *listOffset++;
        for (std::size_t i = 0; i < element_.size(); ++i, ptr += bytesPerElement_)
        {
          switch (property.type())
          {
            case PlyDataType::Char:
              new (ptr) std::vector<char>();
              break;
            case PlyDataType::UChar:
              new (ptr) std::vector<unsigned char>();
              break;
            case PlyDataType::Short:
              new (ptr) std::vector<short>();
              break;
            case PlyDataType::UShort:
              new (ptr) std::vector<unsigned short>();
              break;
            case PlyDataType::Int:
              new (ptr) std::vector<int>();
              break;
            case PlyDataType::UInt:
              new (ptr) std::vector<unsigned int>();
              break;
            case PlyDataType::Float:
              new (ptr) std::vector<float>();
              break;
            case PlyDataType::Double:
              new (ptr) std::vector<double>();
              break;
          }
        }
      }
    }
  }

  /// Destructor, deallocates all vectors holding the variable length lists.
  ~PlyElementData()
  {
    auto listOffset = listOffsets_.begin();
    for (const PlyProperty &property : element_.properties())
    {
      if (property.isList())
      {
        // Note, the exact type of the vector element does not matter here.
        std::uint8_t *ptr = data_.get() + *listOffset++;
        for (std::size_t i = 0; i < element_.size(); ++i, ptr += bytesPerElement_)
        {
          switch (property.type())
          {
            case PlyDataType::Char:
              reinterpret_cast<std::vector<char> *>(ptr)->~vector<char>();
              break;
            case PlyDataType::UChar:
              reinterpret_cast<std::vector<unsigned char> *>(ptr)->~vector<unsigned char>();
              break;
            case PlyDataType::Short:
              reinterpret_cast<std::vector<short> *>(ptr)->~vector<short>();
              break;
            case PlyDataType::UShort:
              reinterpret_cast<std::vector<unsigned short> *>(ptr)->~vector<unsigned short>();
              break;
            case PlyDataType::Int:
              reinterpret_cast<std::vector<int> *>(ptr)->~vector<int>();
              break;
            case PlyDataType::UInt:
              reinterpret_cast<std::vector<unsigned int> *>(ptr)->~vector<unsigned int>();
              break;
            case PlyDataType::Float:
              reinterpret_cast<std::vector<float> *>(ptr)->~vector<float>();
              break;
            case PlyDataType::Double:
              reinterpret_cast<std::vector<double> *>(ptr)->~vector<double>();
              break;
          }
        }
      }
    }
  }

  // This type is non-copyable.
  PlyElementData(const PlyElementData &) = delete;
  PlyElementData &operator=(const PlyElementData &) = delete;

  /// Move constructor.
  PlyElementData(PlyElementData &&x) { *this = std::move(x); }

  /// Move assignment operator.
  PlyElementData &operator=(PlyElementData &&x)
  {
    element_ = std::move(x.element_);
    data_.reset(x.data_.release());
    listOffsets_ = std::move(x.listOffsets_);
    bytesPerElement_ = x.bytesPerElement_;

    x.listOffsets_.clear();
    x.element_ = PlyElement();

    return *this;
  }

  /// Returns the associated element definition.
  const PlyElement &element() const { return element_; }

  /// Returns a pointer to the memory block storing element data.
  std::uint8_t *data() const { return data_.get(); }

  /// Alignment requirements of the structures stored in this memory block.
  std::size_t alignment() const { return alignment_; }

private:
  PlyElement element_;
  std::unique_ptr<std::uint8_t[]> data_;
  std::vector<std::size_t> listOffsets_;
  std::size_t bytesPerElement_ = 0;
  std::size_t alignment_ = 0;
};

}

#endif
