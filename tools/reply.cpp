#include <plywoot/plywoot.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>

/// Converts the given input PLY file to the requested format, and either sends
/// the resulting data to the standard output stream in case no output filename
/// is specified, otherwise creates the output filename.
std::optional<std::string> convert(
    const std::filesystem::path &inputFilename,
    const std::optional<std::filesystem::path> &outputFilename,
    plywoot::PlyFormat format)
{
  std::ifstream ifs{inputFilename};
  if (!ifs) { return "could not open input file " + std::string(inputFilename); }

  std::ofstream of;
  if (outputFilename) { of.open(*outputFilename); }

  std::ostream &os = (outputFilename ? of : std::cout);
  if (!os)
  {
    return outputFilename ? "failed to open " + std::string(*outputFilename) + " for writing"
                          : "can not write to standard output";
  }

  try
  {
    convert(ifs, os, format);
  }
  catch (const std::exception &e)
  {
    return "could not read input PLY file " + std::string(inputFilename) + "; " + e.what();
  }

  return std::nullopt;
}

void printUsage()
{
  std::cout << R"HEREDOC(usage: reply [-a | -b | -B] [-h] INPUT_FILENAME [OUTPUT_FILENAME]

rePLY converts the given input PLY file to either an ASCII (-a), binary little
endian (-b), or binary big endian PLY file (-B). In case no output filename is
specified, rePLY will output converted PLY data on the standard output stream.

  INPUT_FILENAME         name of the input PLY file to convert (required)
  OUTPUT_FILENAME        name of the output PLY file (optional)

Target format options

  -a                     converts the input PLY file to ASCII
  -b                     converts the input PLY file to binary little endian
  -B                     converts the input PLY file to binary big endian

Miscellaneous options

  -h|--help              shows this help text
)HEREDOC";
}

/// Calculates an error message given the input CLI arguments in case some
/// invalid input was specified.
///
/// \param inputFilename input PLY filename
/// \param outputFilename output PLY filename
/// \param requestedFormat optional requested output PLY format
/// \return an optional validation error message, empty in case all input
///     arguments are valid
std::optional<std::string> maybeValidationError(
    const std::optional<std::filesystem::path> &inputFilename,
    const std::optional<std::filesystem::path> &outputFilename,
    const std::optional<plywoot::PlyFormat> &requestedFormat)
{
  if (!inputFilename) { return "missing input file specified, specify '-h' to display usage information"; }

  if (!std::filesystem::exists(*inputFilename))
  {
    return "specified input file " + std::string(*inputFilename) + " does not exist";
  }

  // In case no target format type was specified, issue an error.
  if (!requestedFormat)
  {
    return "no target format specified, specify '-h' to display usage information";
  }

  return std::nullopt;
}

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printUsage();
    return 0;
  }

  const std::map<std::string, plywoot::PlyFormat> formatOptions{
      {"-a", plywoot::PlyFormat::Ascii},
      {"-b", plywoot::PlyFormat::BinaryLittleEndian},
      {"-B", plywoot::PlyFormat::BinaryBigEndian}};

  std::optional<plywoot::PlyFormat> requestedFormat = std::nullopt;
  std::optional<std::filesystem::path> inputFilename = std::nullopt;
  std::optional<std::filesystem::path> outputFilename = std::nullopt;

  // Parse command-line arguments.
  std::optional<std::string> errorMessage;
  for (int i = 1; i < argc && !errorMessage; ++i)
  {
    const std::string arg{argv[i]};

    // Parse command-line switch.
    if (arg.front() == '-')
    {
      auto formatOption = formatOptions.find(arg);
      if (formatOption != formatOptions.end())
      {
        if (requestedFormat) { errorMessage = "specify a single output format"; }
        else { requestedFormat = formatOption->second; }
      }
      else if (arg == "-h" || arg == "--help")
      {
        printUsage();
        return 0;
      }
      else
      {
        errorMessage = "invalid option '" + arg + "' specified, specify '-h' to display usage information";
      }
    }
    else if (!inputFilename) { inputFilename = arg; }
    else if (!outputFilename) { outputFilename = arg; }
  }

  // In case initial parsing of the command-line arguments succeeded, check that
  // the specified arguments are valid.
  if (!errorMessage)
  {
    errorMessage = maybeValidationError(inputFilename, outputFilename, requestedFormat);
  }

  // In case validation succeeded, now either display information on the input
  // file or convert it.
  if (!errorMessage)
  {
    errorMessage = convert(*inputFilename, outputFilename, *requestedFormat);
  }

  // In case some error occurred, print an error message.
  if (errorMessage)
  {
    std::cerr << "Error, " << *errorMessage << ".\n";
    return 1;
  }

  return 0;
}
