#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include "plywoot/ascii_parser_policy.hpp"
#include "plywoot/ascii_writer_policy.hpp"
#include "plywoot/binary_parser_policy.hpp"
#include "plywoot/binary_writer_policy.hpp"
#include "plywoot/header_parser.hpp"
#include "plywoot/parser.hpp"
#include "plywoot/parser_variant.hpp"
#include "plywoot/reflect.hpp"
#include "plywoot/types.hpp"
#include "plywoot/writer.hpp"
#include "plywoot/writer_variant.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace plywoot {

/// Represents an input PLY data stream that can be queried for data.
class IStream
{
public:
  /// Constructs an input PLY data stream from the given input stream. This will
  /// automatically trigger parsing of all PLY data in the input stream.
  IStream(std::istream &is) : IStream{is, detail::HeaderParser{is}} {}

  /// Returns all comment lines.
  const std::vector<Comment> &comments() const { return comments_; }

  /// Returns all elements associated with this PLY file.
  const std::vector<PlyElement> &elements() const { return elements_; }

  /// Returns a pair where the first element is a copy of the element with the
  /// given name in case it exists. The second element is a boolean that
  /// indicates whether the requested element was found. In case a requested
  /// element was not found in the input data, a default constructed element
  /// is returned.
  std::pair<PlyElement, bool> element(const std::string &name) const
  {
    const auto it = std::find_if(
        elements_.begin(), elements_.end(), [&](const PlyElement &e) { return e.name() == name; });
    return it != elements_.end() ? std::pair<PlyElement, bool>{*it, true}
                                 : std::pair<PlyElement, bool>{{}, false};
  }

  /// Returns the format of the input PLY data stream.
  PlyFormat format() const { return format_; }

  /// Positions the read head at the start of the element with the given name,
  /// or at the end of the stream in case the given element is not present in
  /// the stream, skipping over elements that do not match \a elementName.
  /// Returns \c true in case an element with the given name was found,
  /// otherwise, returns \c false.
  bool find(const std::string &elementName) const
  {
    while (currElement_ != elements_.end() && currElement_->name() != elementName) { skipElement(); }
    return currElement_ != elements_.end();
  }

  /// Returns a copy of the current element that can be either read or skipped.
  PlyElement element() const { return hasElement() ? *currElement_ : PlyElement{}; }
  /// Returns whether there are still elements left to parse.
  bool hasElement() const { return currElement_ != elements_.end(); }

  /// Reads the current element from the PLY input data stream, returning a list
  /// of objects of type `T`, where the `Layout` type is used to identify how
  /// properties from the PLY element are mapped on objects of type `T`.
  ///
  /// TODO(ton): add more extensive documentation
  // TODO(ton): probably better to add another parameter 'size' to guard
  template<typename T, typename Layout>
  std::vector<T> readElement() const
  {
    std::vector<T> result(currElement_->size());
    parser_.read(*currElement_++, Layout{result});
    return result;
  }

  /// Skips the current element.
  void skipElement() const { parser_.skip(*currElement_++); }

private:
  /// Constructs a PLY file from the given input stream and header parser.
  IStream(std::istream &is, const detail::HeaderParser &headerParser)
      : parser_{is, headerParser.format()},
        comments_{headerParser.comments()},
        elements_{headerParser.elements()},
        format_{headerParser.format()},
        currElement_{elements_.begin()}
  {
  }

  /// Variant type around some parser type that represents that correct type for
  /// the associated input PLY stream.
  detail::ParserVariant parser_;

  /// All comments defines in the header.
  std::vector<Comment> comments_;
  /// All PLY elements defined in the header.
  std::vector<PlyElement> elements_;
  /// Format of the PLY input data, either ASCII, little-, or big-endian binary.
  PlyFormat format_;

  /// Iterator pointing to the current element.
  mutable std::vector<PlyElement>::const_iterator currElement_;
};

/// Represents an output PLY data stream that can be used to output data to a
/// PLY format.
class OStream
{
public:
  /// Constructs an output PLY data stream of the given format type.
  OStream(PlyFormat format) : format_{format} {}
  /// Constructs an output PLY data stream of the given format type, with the
  /// specified comments that should be written to the header. Line numbers of
  /// the comments should start from line 2, since according to the
  /// specification, comments may not occur in the first two lines of the
  /// header.
  OStream(PlyFormat format, std::vector<Comment> comments) : format_{format}, comments_{std::move(comments)}
  {
    // Ensure comments are sorted on line number (ascending).
    std::stable_sort(comments_.begin(), comments_.end(), [](const Comment &x, const Comment &y) {
      return x.line < y.line;
    });
  }

  /// Queues an element with the associated data for writing. Elements will be
  /// stored in the same order they are queued.
  template<typename... Ts>
  void add(const PlyElement &element, const reflect::Layout<Ts...> &layout)
  {
    // Note; create a copy of the element that specifies as size the number of
    // items in the input layout.
    PlyElement layoutElement{element.name(), layout.size(), element.properties()};

    elementWriteClosures_.emplace_back(
        std::move(layoutElement), [this, layout](detail::WriterVariant &writer, const PlyElement &e) {
          writer.write<Ts...>(e, layout.data(), layout.size());
        });
  }

  /// Writes all data as a PLY file queued through `addElement()` to the given
  /// output stream.
  void write(std::ostream &os) const
  {
    writeHeader(os);

    detail::WriterVariant writer{os, format_};
    for (const auto &elementClosurePair : elementWriteClosures_)
    {
      const PlyElement &element{elementClosurePair.first};
      const auto &writeFn = elementClosurePair.second;
      writeFn(writer, element);
    }
  }

private:
  /// Writes the ASCII PLY header, which defines the format of the PLY data, the
  /// elements and element properties that occur in the data.
  void writeHeader(std::ostream &os) const
  {
    os << "ply\n";

    switch (format_)
    {
      case PlyFormat::Ascii:
        os << "format ascii 1.0\n";
        break;
      case PlyFormat::BinaryBigEndian:
        os << "format binary_big_endian 1.0\n";
        break;
      case PlyFormat::BinaryLittleEndian:
        os << "format binary_little_endian 1.0\n";
        break;
    }

    // Maintain line number to be able to serialize comments at the right
    // location in the header. Comments may only occur after the 'ply' magic
    // 'number', and the format specification. Ideally this state should be
    // limited to the scope of the closure, but that is not supported in C++11.
    std::uint32_t line = 2;
    auto first = comments_.begin();
    const auto last = comments_.end();
    const auto maybeWriteComments = [&]() {
      while (first != last && first->line == line++)
      {
        if (first->text.empty()) { os << "comment\n"; }
        else { os << "comment " << first->text << '\n'; }
        ++first;
      }
    };

    for (const auto &elementClosurePair : elementWriteClosures_)
    {
      maybeWriteComments();
      const PlyElement &element{elementClosurePair.first};
      os << "element " << element.name() << ' ' << element.size() << '\n';

      for (const PlyProperty &property : element.properties())
      {
        maybeWriteComments();
        if (property.isList())
        {
          os << "property list " << property.sizeType() << ' ' << property.type() << ' ' << property.name()
             << '\n';
        }
        else { os << "property " << property.type() << ' ' << property.name() << '\n'; }
      }
    }

    maybeWriteComments();
    os << "end_header\n";
  }

  using ElementWriteClosure = std::function<void(detail::WriterVariant &, const PlyElement &)>;

  /// All queued elements with the associated data.
  std::vector<std::pair<PlyElement, ElementWriteClosure>> elementWriteClosures_;
  /// Format the PLY data should be written in.
  PlyFormat format_;
  /// Comments to write out to the PLY file. Invariant is that comments are
  /// sorted ascending based on their associated line number.
  std::vector<Comment> comments_;
};
}

#endif
