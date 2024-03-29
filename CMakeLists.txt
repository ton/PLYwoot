cmake_minimum_required(VERSION 3.5)

project(PLYwoot
  VERSION 0.0.1
  LANGUAGES CXX
  HOMEPAGE_URL "https://github.com/ton/plywoot"
  DESCRIPTION "C++17 header-only PLY I/O library"
)

# Custom cmake modules.
set(PROJECT_CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_CMAKE_MODULE_PATH})

# Specify release flags.
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Put all executables in build/.
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set the appropriate compiler flags.
add_compile_options("$<$<CONFIG:DEBUG>:-Wall;-Wextra;-Wshadow;-Wno-maybe-uninitialized;-Wno-unused-variable;-Wno-unused-function;-Wno-unused-parameter;-Werror;-fno-strict-aliasing;-march=native>")
add_compile_options("$<$<CONFIG:RELEASE>:-Wall;-Wextra;-Wshadow;-Wno-maybe-uninitialized;-Wpedantic;-Werror;-Wno-unused-parameter;-fno-strict-aliasing;-march=native>")

# External (optional) dependencies.
include(FeatureSummary)

find_package(FastFloat QUIET)
set_package_properties(FastFloat PROPERTIES TYPE DESCRIPTION "fast and exact implementation of the C++ from_chars functions for float and double types.")
set_package_properties(FastFloat PROPERTIES TYPE RECOMMENDED PURPOSE "Significantly improves ASCII parser performance.")

find_package(fast_int QUIET)
set_package_properties(fast_int PROPERTIES TYPE DESCRIPTION "header-only C++11 implementation of the C++17 `from_chars` functions for integer types.")
set_package_properties(fast_int PROPERTIES TYPE RECOMMENDED PURPOSE "Significantly improves ASCII parser performance.")

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES)

# Define the header-only library target for plywoot.
add_library(plywoot INTERFACE)
target_include_directories(plywoot
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(plywoot INTERFACE cxx_std_17)
set_property(TARGET plywoot PROPERTY CXX_EXTENSIONS OFF)

target_compile_definitions(plywoot
  INTERFACE
  $<$<TARGET_EXISTS:FastFloat::fast_float>:PLYWOOT_USE_FAST_FLOAT>
  $<$<TARGET_EXISTS:fast_int::fast_int>:PLYWOOT_USE_FAST_INT>
)
target_link_libraries(plywoot
  INTERFACE
  $<$<TARGET_EXISTS:FastFloat::fast_float>:FastFloat::fast_float>
  $<$<TARGET_EXISTS:fast_int::fast_int>:fast_int::fast_int>
)

add_subdirectory(test)
add_subdirectory(tools)

# install target
include(GNUInstallDirs)

# uninstall target
if(NOT TARGET uninstall)
  configure_file(
    "${PROJECT_CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

  add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

# Create installation targets (because of `INSTALL_INTERFACE` above, headers are automatically included.
install(TARGETS plywoot reply
  EXPORT ${PROJECT_NAME}_Targets
)

# Install a CMake config and version file.
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake" VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

# Create file containing installation targets, and install it.
install(EXPORT ${PROJECT_NAME}_Targets
  FILE ${PROJECT_NAME}Targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

# Install build configuration and version information to the installation directories.
install(
  FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}/cmake
)

# Install header directory.
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/plywoot DESTINATION include)
