#ifndef PLYWOOT_TYPES_HPP
#define PLYWOOT_TYPES_HPP

#include <algorithm>
#include <vector>

namespace plywoot {

enum class PlyDataType { Char, UChar, Short, UShort, Int, UInt, Float, Double };

inline std::ostream &operator<<(std::ostream &os, PlyDataType dataType)
{
  switch (dataType)
  {
    case PlyDataType::Char:
      return os << "char";
    case PlyDataType::UChar:
      return os << "uchar";
    case PlyDataType::Short:
      return os << "short";
    case PlyDataType::UShort:
      return os << "ushort";
    case PlyDataType::Int:
      return os << "int";
    case PlyDataType::UInt:
      return os << "uint";
    case PlyDataType::Float:
      return os << "float";
    case PlyDataType::Double:
      return os << "double";
  }

  return os;
}

enum class PlyFormat {
  Ascii,
  BinaryBigEndian,
  BinaryLittleEndian,
};

struct PlyProperty
{
  PlyProperty() = default;
  PlyProperty(std::string name, PlyDataType type) : name_{std::move(name)}, type_{type} {}
  PlyProperty(std::string name, PlyDataType type, PlyDataType sizeType, std::size_t sizeHint)
      : name_{std::move(name)}, type_{type}, isList_{true}, sizeType_{sizeType}, sizeHint_{sizeHint}
  {
  }

  /// Returns the name of this property.
  const std::string &name() const { return name_; }
  /// Returns the type of this property.
  PlyDataType type() const { return type_; }
  /// Returns whether this property represents a list property.
  bool isList() const { return isList_; }
  /// Returns the size type of this property.
  PlyDataType sizeType() const { return sizeType_; }
  /// Returns the size hint set for this property, in case no such hint is
  /// set, this returns zero.
  std::size_t sizeHint() const { return sizeHint_; }

  size_t numBytes() const
  {
    switch (type_)
    {
      case PlyDataType::Char:
      case PlyDataType::UChar:
        return 1;
      case PlyDataType::Short:
      case PlyDataType::UShort:
        return 2;
      case PlyDataType::Int:
      case PlyDataType::UInt:
      case PlyDataType::Float:
        return 4;
      case PlyDataType::Double:
        return 8;
    }
  }

  inline friend bool operator==(const PlyProperty &x, const PlyProperty &y)
  {
    // Note; the size hint is not part of the identity of a property.
    return x.type_ == y.type_ && x.isList_ == y.isList_ && x.sizeType_ == y.sizeType_ &&
           x.name_ == y.name_;
  }

  inline friend bool operator!=(const PlyProperty &x, const PlyProperty &y) { return !(x == y); }

private:
  std::string name_;
  PlyDataType type_;

  bool isList_{false};
  PlyDataType sizeType_{PlyDataType::Char};
  std::size_t sizeHint_{0};
};

struct PlyElement
{
  /// Default constructor.
  PlyElement() = default;
  /// Constructor taking a name and size for this element.
  PlyElement(std::string name, std::size_t size) : name_{std::move(name)}, size_{size} {}
  /// Constructor taking a name and size for this element, as well as a list
  /// of initial properties to associate with this element.
  PlyElement(std::string name, std::size_t size, std::vector<PlyProperty> properties)
      : name_{std::move(name)}, size_{size}, properties_{std::move(properties)}
  {
  }

  /// Returns the name of this element.
  const std::string &name() const { return name_; }
  /// Returns the size of this element.
  std::size_t size() const { return size_; }
  /// Returns the properties associated with this element.
  const std::vector<PlyProperty> &properties() const { return properties_; }

  /// Returns a copy of this element, with a size hint set for the property
  /// with the given name. In case the property is not found, this just
  /// returns a copy of this element.
  PlyElement setSizeHint(const std::string &propertyName, std::size_t sizeHint) const
  {
    std::vector<PlyProperty> propertiesCopy;
    for (const PlyProperty &p : properties_)
    {
      if (p.name() == propertyName && p.isList())
      {
        propertiesCopy.emplace_back(p.name(), p.type(), p.sizeType(), sizeHint);
      }
      else { propertiesCopy.push_back(p); }
    }

    return PlyElement{name_, size_, std::move(propertiesCopy)};
  }

  /// Returns a pair where the first element is a copy of the property with
  /// the given name in case it exists. The second element is a Boolean that
  /// indicates whether the requested property was found. In case a requested
  /// property was not found for this element, a default constructed property
  /// is returned.
  std::pair<PlyProperty, bool> property(const std::string &propertyName) const
  {
    const auto it = std::find_if(properties_.begin(), properties_.end(), [&](const PlyProperty &p) {
      return p.name() == propertyName;
    });
    return it != properties_.end() ? std::pair<PlyProperty, bool>{*it, true}
                                   : std::pair<PlyProperty, bool>{{}, false};
  }

  template<typename... Args>
  PlyProperty &addProperty(Args &&...args)
  {
    properties_.emplace_back(std::forward<Args>(args)...);
    return properties_.back();
  }

  inline friend bool operator==(const PlyElement &x, const PlyElement &y)
  {
    return x.size_ == y.size_ && x.name_ == y.name_ && x.properties_ == y.properties_;
  }
  inline friend bool operator!=(const PlyElement &x, const PlyElement &y) { return !(x == y); }

private:
  std::string name_;
  std::size_t size_;
  std::vector<PlyProperty> properties_;
};

}

#endif