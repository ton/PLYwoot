find_package(benchmark REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(profiler REQUIRED IMPORTED_TARGET libprofiler)

add_executable(benchmarks benchmark.cpp miniply/miniply.cpp)
set_source_files_properties(miniply/miniply.cpp PROPERTIES COMPILE_FLAGS -Wno-shadow)
target_link_libraries(benchmarks
  PRIVATE
  benchmark::benchmark
  plywoot
  PkgConfig::profiler
)
