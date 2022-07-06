find_package(Catch2 3 REQUIRED)

# Main test executable; depends on Catch2, requires C++14 support.
add_executable(tests main.cpp test_istream.cpp test_ostream.cpp test_iostream.cpp)
target_link_libraries(tests
  PRIVATE
  plywoot
  Catch2::Catch2WithMain
)

# Executables that are only compiled to test compatibility for the various C++
# standards.
add_executable(test-cpp11 test-cpp11.cpp)
target_link_libraries(test-cpp11
  PRIVATE
  plywoot
)
