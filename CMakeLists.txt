# ============================================================================
# byteorder - CMake Build Configuration
# ============================================================================
#
# High-performance, header-only C++20 byte-order (endianness) conversion
# library with SIMD acceleration (AVX-512 / AVX2 / ARM NEON / SVE).
#
# QUICK START:
#   cmake --preset debug            # Configure with debug preset
#   cmake --build --preset debug    # Build the project
#   ctest --preset debug            # Run tests
#
# CLASSIC USAGE:
#   mkdir build && cd build
#   cmake ..
#   cmake --build .
#
# CONFIGURATION OPTIONS:
#   -DBUILD_EXAMPLES=ON|OFF        - Build example programs
#   -DBUILD_TESTS=ON|OFF           - Build and enable tests
#   -DBUILD_BENCHMARKS=ON|OFF      - Build benchmarking programs
#   -DENABLE_COVERAGE=ON|OFF       - Enable code coverage reporting
#   -DENABLE_SANITIZERS=ON|OFF     - Enable sanitizers in debug builds
#   -DENABLE_LTO=ON|OFF            - Enable Link Time Optimization
#   -DENABLE_AVX512=ON|OFF         - Enable AVX-512 optimizations (x86_64)
#   -DENABLE_NATIVE_OPTIMIZATIONS=ON|OFF - Enable -march=native optimizations
#
# ============================================================================

# CMake 3.21+ is required for:
#   - PROJECT_IS_TOP_LEVEL variable (automatically set)
#   - Better FetchContent integration with FIND_PACKAGE_ARGS
#   - Improved preset support
cmake_minimum_required(VERSION 3.21)

# ============================================================================
# Project Version Control
# ============================================================================
# Change these values when releasing a new version.
# Follow Semantic Versioning (https://semver.org/):
#   MAJOR - breaking API changes
#   MINOR - new features, backward compatible
#   PATCH - bug fixes, backward compatible
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

# Define the project with metadata
# These values are used by CPack, Doxygen, and install targets
project(byteorder
    VERSION ${PROJECT_VERSION}
    DESCRIPTION "High-performance C++20 header-only byte-order conversion library with SIMD acceleration"
    HOMEPAGE_URL "https://github.com/korbolkoinc/byteorder"
    LANGUAGES CXX
)

# ============================================================================
# Top-Level Project Settings
# ============================================================================
# PROJECT_IS_TOP_LEVEL is automatically set by CMake 3.21+
# When this project is the root (not included via add_subdirectory),
# we enable developer-friendly settings
if(PROJECT_IS_TOP_LEVEL)
    # Generate compile_commands.json for IDE and tool support
    # (Used by clang-tidy, clangd, VS Code, etc.)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    # Organize targets into folders in IDEs like Visual Studio
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

# ============================================================================
# Build Configuration Options
# ============================================================================
# These can be set using -D flags when running cmake, or via CMakePresets.json.
# Example: cmake .. -DBUILD_SHARED_LIBS=ON -DENABLE_SANITIZERS=ON
option(BUILD_EXAMPLES "Build example programs demonstrating library usage" ON)
option(BUILD_TESTS "Build unit tests (requires Google Test)" ON)
option(BUILD_BENCHMARKS "Build performance benchmark programs" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers (ASan, UBSan) in debug builds" OFF)
option(ENABLE_PCH "Enable precompiled headers to speed up compilation" OFF)
option(ENABLE_LTO "Enable Link Time Optimization for release builds" OFF)
option(ENABLE_AVX512 "Enable AVX-512 optimizations for x86_64 platforms" OFF)
option(ENABLE_NATIVE_OPTIMIZATIONS "Enable -march=native for maximum performance" OFF)
option(ENABLE_SVE "Enable ARM SVE optimizations for ARM platforms" OFF)

# ============================================================================
# Output Directory Configuration
# ============================================================================
# Place all build artifacts in organized directories for easy access.
# This avoids hunting for binaries scattered across the build tree.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # Static libraries (.a, .lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)  # Shared libraries (.so, .dll)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)  # Executables (.exe)

# ============================================================================
# C++ Standard Configuration
# ============================================================================
set(CMAKE_CXX_STANDARD 20)           # Use C++20 features
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Fail if compiler doesn't support C++20
set(CMAKE_CXX_EXTENSIONS OFF)        # Use -std=c++20, not -std=gnu++20

# ============================================================================
# Include Custom CMake Modules
# ============================================================================
# Our cmake/ directory contains reusable modules for different concerns.
# Each module handles one aspect of the build configuration.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompilerWarnings)    # Strict compiler warning flags (GCC, Clang, MSVC)
include(Sanitizers)          # Runtime error detection (ASan, UBSan, TSan, MSan)
include(StaticAnalyzers)     # Static analysis tools (cppcheck, clang-tidy, IWYU)
include(LTO)                 # Link Time Optimization for smaller/faster binaries
include(Coverage)            # Code coverage instrumentation and report generation
include(Packaging)           # CPack packaging (DEB, RPM, NSIS, ZIP, TGZ)
include(Dependencies)        # External dependency manager (GTest, Benchmark, simd_feature_check)

# ============================================================================
# Generated Configuration Headers
# ============================================================================
# Process Config.h.in to produce a header with compile-time version constants.
#   #include "configs/ProjectConfig.h"
#   std::cout << byteorder_VERSION << std::endl;
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/configs/Config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/configs/ProjectConfig.h"
)

# ============================================================================
# Library Target (header-only — INTERFACE)
# ============================================================================
# byteorder is a header-only template library; no source files are compiled
# into the library itself. The INTERFACE target propagates include paths and
# C++ standard requirements to every consumer automatically.
add_library(${PROJECT_NAME} INTERFACE)

# Namespaced alias — consumers use:
#   target_link_libraries(my_target PRIVATE byteorder::byteorder)
# This works identically whether found via find_package() or add_subdirectory().
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Propagate include directories to consumers
target_include_directories(${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>    # Public headers
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>  # Generated config header
    $<INSTALL_INTERFACE:include>                              # After installation
)

# Require C++20 from any target that links to this library
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_20)

# ============================================================================
# simd_feature_check Dependency
# ============================================================================
# byteorder.h uses simd_feature_check for portable runtime CPU feature detection
# (AVX2, AVX-512F, etc.) instead of hand-rolled cpuid intrinsics.
# Because byteorder is an INTERFACE library, every consumer automatically
# inherits this dependency through the INTERFACE link below.
setup_simd_feature_check()
target_link_libraries(${PROJECT_NAME} INTERFACE simd_featrue_check)

# ============================================================================
# Architecture-specific optimizations
# ============================================================================
# Enable maximum performance optimizations for native builds.
# Use -DENABLE_NATIVE_OPTIMIZATIONS=ON for best performance on the build machine.
if(ENABLE_NATIVE_OPTIMIZATIONS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PROJECT_NAME} INTERFACE -march=native)
        message(STATUS "byteorder: Native optimizations enabled (-march=native)")
    elseif(MSVC)
        target_compile_options(${PROJECT_NAME} INTERFACE /arch:AVX2)
        message(STATUS "byteorder: AVX2 optimizations enabled (/arch:AVX2)")
    endif()
endif()

# Enable AVX-512 optimizations for x86_64 platforms.
# Use -DENABLE_AVX512=ON for servers and high-performance computing.
if(ENABLE_AVX512)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${PROJECT_NAME} INTERFACE -mavx512f -mavx512bw -mavx512vl)
            target_compile_definitions(${PROJECT_NAME} INTERFACE __AVX512F__)
            message(STATUS "byteorder: AVX-512 optimizations enabled")
        elseif(MSVC)
            target_compile_options(${PROJECT_NAME} INTERFACE /arch:AVX512)
            target_compile_definitions(${PROJECT_NAME} INTERFACE __AVX512F__)
            message(STATUS "byteorder: AVX-512 optimizations enabled (MSVC)")
        endif()
    else()
        message(WARNING "byteorder: AVX-512 is only supported on x86_64 platforms")
    endif()
endif()

# Enable ARM SVE (Scalable Vector Extension) for ARM64 platforms.
# Use -DENABLE_SVE=ON for ARM-based servers and high-performance devices.
if(ENABLE_SVE)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64|arm64")
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(${PROJECT_NAME} INTERFACE -march=armv8-a+sve)
            target_compile_definitions(${PROJECT_NAME} INTERFACE __ARM_FEATURE_SVE=1)
            message(STATUS "byteorder: ARM SVE optimizations enabled")
        endif()
    else()
        message(WARNING "byteorder: ARM SVE is only supported on ARM64 platforms")
    endif()
endif()

# ============================================================================
# Main Executable (demo / smoke-test)
# ============================================================================
add_executable(${PROJECT_NAME}_exe src/main.cpp)
target_link_libraries(${PROJECT_NAME}_exe PRIVATE ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME}_exe PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_compile_warnings(${PROJECT_NAME}_exe PRIVATE)
if(ENABLE_SANITIZERS)
    target_enable_sanitizers(${PROJECT_NAME}_exe)
endif()
target_enable_coverage(${PROJECT_NAME}_exe)

# ============================================================================
# Optional Subdirectories
# ============================================================================
# Each subdirectory is conditionally included based on build options.
# This keeps build times fast when you only need specific components.

# Example programs showing how to use the library
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Unit tests with Google Test framework
if(BUILD_TESTS)
    enable_testing()       # Enable CTest integration
    add_subdirectory(test)
endif()

# API documentation with Doxygen (only if Doxygen is installed)
# Generate docs with: cmake --build . --target byteorder_docs
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOCS_TARGET_NAME "${PROJECT_NAME}_docs")
    add_subdirectory(docs)
endif()

# Performance benchmarks with Google Benchmark
if(BUILD_BENCHMARKS)
    add_subdirectory(bench)
endif()

# ============================================================================
# Installation Configuration
# ============================================================================
# This section configures how the project is installed on the system.
# After building, run: cmake --install build/ --prefix /usr/local
#
# The installed package can then be used by other CMake projects:
#   find_package(byteorder REQUIRED)
#   target_link_libraries(your_target PRIVATE byteorder::byteorder)

# GNUInstallDirs provides standard installation directory variables
# (CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_INCLUDEDIR)
include(GNUInstallDirs)

# Install the INTERFACE library target and the demo executable
install(TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_exe
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Install public header files (preserving directory structure)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the generated configuration header
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/generated/configs/ProjectConfig.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/configs
)

# Install CMake targets file so other projects can find_package() us
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Generate and install package configuration files so that consumers can
# use find_package(byteorder REQUIRED) to locate this library.
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/configs/byteorderConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# AnyNewerVersion: patch/minor bumps remain compatible
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# ============================================================================
# Print configuration summary
# ============================================================================
# Display a summary of the current build configuration for easy verification.
message(STATUS "")
message(STATUS "byteorder ${PROJECT_VERSION} configuration summary:")
message(STATUS "  Build type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler:          ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build examples:    ${BUILD_EXAMPLES}")
message(STATUS "  Build tests:       ${BUILD_TESTS}")
message(STATUS "  Build benchmarks:  ${BUILD_BENCHMARKS}")
message(STATUS "  Enable sanitizers: ${ENABLE_SANITIZERS}")
message(STATUS "  Enable LTO:        ${ENABLE_LTO}")
message(STATUS "  Enable AVX-512:    ${ENABLE_AVX512}")
message(STATUS "  Enable native opt: ${ENABLE_NATIVE_OPTIMIZATIONS}")
message(STATUS "  Enable SVE:        ${ENABLE_SVE}")
message(STATUS "")
