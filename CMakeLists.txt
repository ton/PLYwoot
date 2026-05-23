cmake_minimum_required(VERSION 3.14)

project(PLYwoot
  VERSION 0.0.1
  LANGUAGES CXX
  HOMEPAGE_URL "https://github.com/ton/plywoot"
  DESCRIPTION "C++17 header-only PLY I/O library"
)

# Custom cmake modules.
set(PROJECT_CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_CMAKE_MODULE_PATH})

# Hide 'up-to-date' installation messages.
set(CMAKE_INSTALL_MESSAGE LAZY)

# Put all executables in build/.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Improve configure step performance for big FetchContent dependencies, and do
# not place fetched dependencies in the builds directory but rather in `.deps`.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_BASE_DIR "${PROJECT_SOURCE_DIR}/.deps" CACHE PATH "FetchContent base directory")

# Options supported by this build.
option(PLYWOOT_WITH_FAST_FLOAT "Enable building tests and tools with `fast_float` support" OFF)
option(PLYWOOT_WITH_FAST_INT "Enable building tests and tools with `fast_int` support" OFF)
option(PLYWOOT_WITH_TEST "Enable building the unit test binary" OFF)
option(PLYWOOT_WITH_TOOLS "Enable building PLYwoot tools (rePLY only for now)" OFF)

# External (optional) dependencies.
include(FetchContent)

if(PLYWOOT_WITH_FAST_FLOAT)
  FetchContent_Declare(
    FastFloat
    GIT_REPOSITORY https://github.com/fastfloat/fast_float.git
    GIT_TAG v8.2.5
  )
  FetchContent_MakeAvailable(FastFloat)
endif()

if(PLYWOOT_WITH_FAST_INT)
  FetchContent_Declare(
    fast_int
    GIT_REPOSITORY https://github.com/ton/fast_int.git
    GIT_TAG v0.0.1
  )
  FetchContent_MakeAvailable(fast_int)
endif()

# Define the header-only library target for plywoot.
add_library(plywoot INTERFACE)
target_include_directories(plywoot
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(plywoot INTERFACE cxx_std_17)
set_property(TARGET plywoot PROPERTY CXX_EXTENSIONS OFF)

# Conditionally add `fast_int` and `FastFloat` dependencies in case they have
# been found.
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

add_library(plywoot-compiler-flags INTERFACE)

# Set the appropriate compiler flags (GCC)
target_compile_options(plywoot-compiler-flags INTERFACE "$<$<CXX_COMPILER_ID:GNU>:-Wall;-Werror;-Wextra;-Wshadow;-Wno-maybe-uninitialized;-Wno-unused-parameter;-fno-strict-aliasing>")
target_compile_options(plywoot-compiler-flags INTERFACE "$<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:-g;-Wno-unused-variable;-Wno-unused-function>")
target_compile_options(plywoot-compiler-flags INTERFACE "$<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:-Wpedantic;-O3;-DNDEBUG;-march=native>")

# Set the appropriate compiler flags (MSVC)
target_compile_options(plywoot-compiler-flags INTERFACE "$<$<CXX_COMPILER_ID:MSVC>:/W4;/WX;/permissive>")
target_compile_options(plywoot-compiler-flags INTERFACE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:DEBUG>>:/Od>")
target_compile_options(plywoot-compiler-flags INTERFACE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RELEASE>>:/O2>")

# Define additional targets depending on the current configuration.
if(PLYWOOT_WITH_TEST)
  enable_testing()
  add_subdirectory(test)
else()
  message(STATUS "Building unit tests is disabled, set PLYWOOT_WITH_TEST to ON to build the unit test binary.")
endif()

if(PLYWOOT_WITH_TOOLS)
  add_subdirectory(tools)
else()
  message(STATUS "Building tools is disabled, set PLYWOOT_WITH_TOOLS to ON to build tools.")
endif()

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
if(PLYWOOT_WITH_TOOLS)
  install(TARGETS plywoot plywoot-reply
    EXPORT ${PROJECT_NAME}_Targets
  )
else()
  install(TARGETS plywoot
    EXPORT ${PROJECT_NAME}_Targets
  )
endif()

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
