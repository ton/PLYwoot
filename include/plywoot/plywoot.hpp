#ifndef PLYWOOT_HPP
#define PLYWOOT_HPP

#include "plywoot/ascii_parser_policy.hpp"
#include "plywoot/ascii_writer_policy.hpp"
#include "plywoot/binary_parser_policy.hpp"
#include "plywoot/binary_writer_policy.hpp"
#include "plywoot/header_parser.hpp"
#include "plywoot/parser.hpp"
#include "plywoot/reflect.hpp"
#include "plywoot/types.hpp"
#include "plywoot/writer.hpp"

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

  /// Reads the given element from the PLY input data stream, expecting elements
  /// where every property can be statically casted to the given property type
  /// in the template argument list.
  template<typename T, typename Layout>
  std::vector<T> read(const PlyElement &element) const
  {
    std::vector<T> result(element.size());
    read(element, Layout{result});
    return result;
  }

  /// Reads the given element from the PLY input data stream, storing data in
  /// the given destination buffer `dest` using the types given as
  /// `PropertyType`'s in the template argument list. In case the number of
  /// properties for the element exceeds the number of properties in the
  /// template argument list, the remaining properties are directly stored
  /// using the property type as defined in the PLY header. This assumes that
  /// the output buffer can hold the required amount of data; failing to
  /// satisfy this precondition results in undefined behavior.
  // TODO(ton): probably better to add another parameter 'size' to guard
  // against overwriting the input buffer.
  template<typename... Ts>
  void read(const PlyElement &element, reflect::Layout<Ts...> layout) const
  {
    // Note; need to clear any eof() flags being set to ensure seeking succeeds.
    is_.clear();
    is_.seekg(endOfHeaderOffset_);

    switch (format_)
    {
      case PlyFormat::Ascii: {
        detail::Parser<detail::AsciiParserPolicy> parser{is_, elements_};
        parser.read<Ts...>(element, layout);
      }
      break;
      case PlyFormat::BinaryLittleEndian: {
        detail::Parser<detail::BinaryLittleEndianParserPolicy> parser{is_, elements_};
        parser.read<Ts...>(element, layout);
      }
      break;
      case PlyFormat::BinaryBigEndian: {
        detail::Parser<detail::BinaryBigEndianParserPolicy> parser{is_, elements_};
        parser.read<Ts...>(element, layout);
      }
      break;
    }
  }

private:
  /// Constructs a PLY file from the given input stream and header parser.
  IStream(std::istream &is, const detail::HeaderParser &parser)
      : is_{is},
        endOfHeaderOffset_{is.tellg()},
        comments_{parser.comments()},
        elements_{parser.elements()},
        format_{parser.format()}
  {
  }

  /// Input stream containing the PLY data.
  std::istream &is_;
  /// Offset in the input stream where the header data ends and the element data
  /// starts.
  std::istream::pos_type endOfHeaderOffset_;

  /// All comments defines in the header.
  std::vector<Comment> comments_;
  /// All PLY elements defined in the header.
  std::vector<PlyElement> elements_;
  /// Format of the PLY input data, either ASCII, little-, or big-endian binary.
  PlyFormat format_;
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
    switch (format_)
    {
      case PlyFormat::Ascii: {
        static detail::Writer<detail::AsciiWriterPolicy> writer;
        elementWriteClosures_.emplace_back(element, [this, layout](std::ostream &os, const PlyElement &e) {
          writer.write<Ts...>(os, e, layout.data(), layout.size());
        });
      }
      break;
      case PlyFormat::BinaryLittleEndian: {
        static detail::Writer<detail::BinaryLittleEndianWriterPolicy> writer;
        elementWriteClosures_.emplace_back(element, [this, layout](std::ostream &os, const PlyElement &e) {
          writer.write<Ts...>(os, e, layout.data(), layout.size());
        });
      }
      break;
      case PlyFormat::BinaryBigEndian: {
        static detail::Writer<detail::BinaryBigEndianWriterPolicy> writer;
        elementWriteClosures_.emplace_back(element, [this, layout](std::ostream &os, const PlyElement &e) {
          writer.write<Ts...>(os, e, layout.data(), layout.size());
        });
      }
      break;
    }
  }

  /// Writes all data as a PLY file queued through `addElement()` to the given
  /// output stream.
  void write(std::ostream &os) const
  {
    writeHeader(os);

    for (const auto &elementClosurePair : elementWriteClosures_)
    {
      const PlyElement &element{elementClosurePair.first};
      const auto &writeFn = elementClosurePair.second;
      writeFn(os, element);
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

  using ElementWriteClosure = std::function<void(std::ostream &, const PlyElement &)>;

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
