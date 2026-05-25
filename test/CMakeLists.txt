include(FetchBoostContent)

FetchBoostContent_Declare(
  boost_iostreams
  GIT_REPOSITORY https://github.com/boostorg/iostreams
  GIT_TAG boost-1.91.0
)

FetchBoostContent_MakeAvailable(boost_iostreams)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2
  GIT_TAG v3.14.0
)

FetchContent_MakeAvailable(Catch2)

find_package(ZLIB REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

include(Catch)
include(CTest)

# Main test executable; depends on Catch2, requires C++14 support.
add_executable(plywoot-tests
  main.cpp
  test_convert.cpp
  test_istream.cpp
  test_ostream.cpp
  test_iostream.cpp
  test_type_traits.cpp
)
set_target_properties(plywoot-tests
  PROPERTIES OUTPUT_NAME tests
)
target_link_libraries(plywoot-tests
  PRIVATE
    plywoot
    plywoot-compiler-flags
    Boost::iostreams
    Catch2::Catch2WithMain
)

# TODO: This ensures Boost iostreams include directories are considered system
# includes, but it seems to be inconvenient. Is there no better way with
# FetchContent?
target_include_directories(plywoot-tests
  SYSTEM PRIVATE
    $<TARGET_PROPERTY:Boost::iostreams,INTERFACE_INCLUDE_DIRECTORIES>
)

catch_discover_tests(plywoot-tests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

# Seems there is a bug in GCC 12 where it issues a warning in the standard
# library code for a trivial `std::reverse` call.
if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12 AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13)
  set_source_files_properties(test_istream.cpp PROPERTIES COMPILE_FLAGS -Wno-strict-overflow)
endif()
