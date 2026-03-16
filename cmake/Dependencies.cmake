# ============================================================================
# Dependency Management Module
# ============================================================================
#
# This module centralizes all external dependency management using CMake's
# FetchContent. Instead of requiring users to manually install dependencies,
# they are automatically downloaded and built when needed.
#
# Supported dependencies:
#   - Google Test v1.15.2  → Unit testing and mocking framework
#   - Google Benchmark v1.9.1 → Performance benchmarking framework
#
# How it works:
#   FetchContent first tries to find the dependency via find_package()
#   (FIND_PACKAGE_ARGS). If not found locally, it downloads from GitHub.
#   GIT_SHALLOW TRUE means only the latest commit is downloaded (faster).
#
# Using system-installed versions:
#   If you prefer to use system-installed GTest/Benchmark instead of downloading:
#     sudo apt install libgtest-dev    # Ubuntu/Debian
#     brew install googletest          # macOS
#   CMake will automatically find and use them via find_package().
#
# ============================================================================

include(FetchContent)

# Show download progress — useful to know what's happening during first build
set(FETCHCONTENT_QUIET OFF)

# ============================================================================
# Google Test Setup
# ============================================================================
# Provides: GTest::gtest, GTest::gtest_main, GTest::gmock, GTest::gmock_main
#
# Usage in test/CMakeLists.txt:
#   setup_googletest()
#   target_link_libraries(my_test PRIVATE GTest::gtest GTest::gtest_main)
macro(setup_googletest)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2          # Latest stable release
        GIT_SHALLOW TRUE         # Only download the tagged commit (much faster)
        FIND_PACKAGE_ARGS NAMES GTest  # Try find_package(GTest) first
    )

    # On Windows, force Google Test to use the same runtime library as our project.
    # Without this, you get linker errors about mismatched /MT vs /MD flags.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    # Enable Google Mock (GMock) for creating mock objects in tests
    set(BUILD_GMOCK ON CACHE BOOL "" FORCE)

    # Don't let GTest install itself into system directories
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

    # Download and make available (or use the system version if found)
    FetchContent_MakeAvailable(googletest)
endmacro()

# ============================================================================
# Google Benchmark Setup
# ============================================================================
# Provides: benchmark::benchmark, benchmark::benchmark_main
#
# Usage in bench/CMakeLists.txt:
#   setup_googlebenchmark()
#   target_link_libraries(my_bench PRIVATE benchmark::benchmark)
macro(setup_googlebenchmark)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1           # Latest stable release
        GIT_SHALLOW TRUE         # Only download the tagged commit
        FIND_PACKAGE_ARGS NAMES benchmark  # Try find_package(benchmark) first
    )

    # We don't need to build benchmark's own tests
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark testing" FORCE)

    # Don't install benchmark into system directories
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)

    # Be lenient with warnings in third-party code
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "Disable treating warnings as errors" FORCE)

    # Use our project's GTest instead of downloading another copy
    set(BENCHMARK_USE_BUNDLED_GTEST OFF CACHE BOOL "Use project's GTest" FORCE)

    FetchContent_MakeAvailable(googlebenchmark)

    # Suppress warnings from benchmark source code.
    # Third-party code may not follow our strict warning settings,
    # so we apply more lenient flags to avoid noisy build output.
    if(TARGET benchmark)
        if(MSVC)
            target_compile_options(benchmark PRIVATE /W3 /WX-)
            target_compile_options(benchmark_main PRIVATE /W3 /WX-)
        else()
            target_compile_options(benchmark PRIVATE
                -Wall -Wextra -Wno-error
                -Wno-invalid-offsetof -Wno-error=invalid-offsetof
            )
            target_compile_options(benchmark_main PRIVATE
                -Wall -Wextra -Wno-error
                -Wno-invalid-offsetof -Wno-error=invalid-offsetof
            )
        endif()
    endif()
endmacro()

# ============================================================================
# simd_feature_check Setup
# ============================================================================
# Provides runtime CPU feature detection for SIMD extensions (x86/x86_64):
#   AVX, AVX2, FMA, AVX-512F/CD/DQ/BW/VL and many more extensions.
#
# Upstream repo: https://github.com/korbolkoinc/simd_feature_check
#
# Provides CMake target: simd_featrue_check (note: upstream project has a typo)
# Alias:                 simd_featrue_check::simd_featrue_check
#
# Usage example:
#   setup_simd_feature_check()
#   target_link_libraries(my_target PRIVATE simd_featrue_check)
#
# In C++ code:
#   #include <simd/feature_check.hpp>
#   if (simd::detail::CPUInfo::has_avx2()) { ... }
#   if (simd::detail::CPUInfo::has_avx512f()) { ... }
# ============================================================================
macro(setup_simd_feature_check)
    FetchContent_Declare(
        simd_feature_check
        GIT_REPOSITORY https://github.com/korbolkoinc/simd_feature_check.git
        GIT_TAG        main             # Track the main branch
        GIT_SHALLOW    TRUE             # Only download the latest commit (faster)
    )

    # -----------------------------------------------------------------------
    # Isolate simd_feature_check from our build settings:
    #
    # 1. Save and restore BUILD_* options — setting them to OFF with FORCE
    #    would otherwise permanently override our preset / -D values, causing
    #    our own tests/examples to silently disappear from the build.
    #
    # 2. Clear CMAKE_MODULE_PATH while the subdirectory is processed so that
    #    simd_feature_check's include(Packaging) picks up its OWN
    #    cmake/Packaging.cmake instead of ours (which would trigger a
    #    "CPack.cmake has already been included" warning/error).
    # -----------------------------------------------------------------------
    set(_sfc_saved_BUILD_EXAMPLES   ${BUILD_EXAMPLES})
    set(_sfc_saved_BUILD_TESTS      ${BUILD_TESTS})
    set(_sfc_saved_BUILD_BENCHMARKS ${BUILD_BENCHMARKS})
    set(_sfc_saved_MODULE_PATH      "${CMAKE_MODULE_PATH}")

    set(BUILD_EXAMPLES   OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS      OFF CACHE BOOL "" FORCE)
    set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(CMAKE_MODULE_PATH "")          # simd_feature_check will re-add its own cmake/

    FetchContent_MakeAvailable(simd_feature_check)

    # Restore everything to what the parent project had
    set(CMAKE_MODULE_PATH "${_sfc_saved_MODULE_PATH}")
    set(BUILD_EXAMPLES   ${_sfc_saved_BUILD_EXAMPLES}   CACHE BOOL "" FORCE)
    set(BUILD_TESTS      ${_sfc_saved_BUILD_TESTS}      CACHE BOOL "" FORCE)
    set(BUILD_BENCHMARKS ${_sfc_saved_BUILD_BENCHMARKS} CACHE BOOL "" FORCE)
endmacro()
