find_package(benchmark REQUIRED)

add_executable(benchmarks benchmark.cpp miniply/miniply.cpp)
set_source_files_properties(miniply/miniply.cpp PROPERTIES COMPILE_FLAGS -Wno-shadow)
target_link_libraries(benchmarks
  PRIVATE
  benchmark::benchmark
  plywoot
)
