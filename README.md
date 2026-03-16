# byteorder

A C++20 header-only library for byte-order conversion. It handles scalar swaps, bulk operations on contiguous data, zero-copy buffer access, and memory-mapped I/O registers — with SIMD acceleration on x86-64 (AVX2, AVX-512) and ARM NEON where available. CPU feature detection at runtime is delegated to the [simd_feature_check](https://github.com/korbolkoinc/simd_feature_check) library.

---

## Requirements

- C++20 compiler (GCC 12+, Clang 14+, MSVC 19.29+)
- CMake 3.21 or later

---

## Installation

### CMake FetchContent

Add the following to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(byteorder
    GIT_REPOSITORY https://github.com/yourorg/byteorder.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(byteorder)

target_link_libraries(my_target PRIVATE byteorder)
```

### System-wide install

```bash
cmake --preset debug
cmake --build --preset debug
cmake --install build/debug --prefix /usr/local
```

Then in your project:

```cmake
find_package(byteorder REQUIRED)
target_link_libraries(my_target PRIVATE byteorder::byteorder)
```

---

## Quick start

```cpp
#include <byteorder/byteorder.h>
#include <cstdint>
#include <iostream>

int main()
{
    using namespace endian;

    constexpr uint32_t value = 0x01020304;
    basic_endian<uint32_t> e(value);

    std::cout << std::hex;
    std::cout << "native : 0x" << e.native() << "\n";  // platform byte order
    std::cout << "little : 0x" << e.little() << "\n";  // always little-endian
    std::cout << "big    : 0x" << e.big()    << "\n";  // always big-endian

    // Free function for one-liners
    uint32_t swapped = byte_swap(value);
}
```

---

## Core API

### Scalar byte swap

`detail::byte_swap` and its public wrapper `endian::byte_swap` work on any trivially copyable type of size 1, 2, 4, or 8 bytes. On supported compilers the function expands to a single intrinsic (`_byteswap_ulong`, `__builtin_bswap32`, etc.). At compile time it uses a portable bit-shifting fallback that qualifies as `constexpr`.

```cpp
constexpr uint16_t a = endian::byte_swap(uint16_t{0x0102});  // 0x0201
constexpr uint32_t b = endian::byte_swap(uint32_t{0xAABBCCDD});  // 0xDDCCBBAA
constexpr uint64_t c = endian::byte_swap(uint64_t{0x0102030405060708ULL});

float f = 1.0f;
float fs = endian::byte_swap(f);  // floating-point types are also supported
```

For strict compile-time contexts where a `consteval` result is needed:

```cpp
consteval uint32_t swapped = endian::byte_swap_consteval(uint32_t{0x12345678});  // 0x78563412
```

### basic_endian

`basic_endian<T>` stores a value internally in native byte order and converts on demand. It supports any type that satisfies `ByteSwappable` (trivially copyable, size 1/2/4/8 bytes, not `bool` or `char`).

```cpp
using namespace endian;

basic_endian<uint16_t> header_field(0x1234);

// Read in whatever order is needed
uint16_t native_val = header_field.native();
uint16_t big_val    = header_field.big();
uint16_t little_val = header_field.little();

// Store a new value
header_field.store_big(0xABCD);

// Convert using a runtime byte_order value
byte_order order = byte_order::network;
uint16_t net = header_field.order(order);
```

Comparison and ordering are defined and operate on native values:

```cpp
basic_endian<uint32_t> x(100), y(200);
bool less = (x < y);  // true
```

### Type aliases

A set of convenience aliases is provided. All of them simply wrap `basic_endian<T>` and are interchangeable — the prefix (`le_` / `be_`) is a naming convention, not enforced storage:

```cpp
endian::le_uint32  port_number(8080);
endian::be_uint64  sequence_id(0x0000000000000001ULL);
endian::be_float32 measurement(3.14f);
```

### User-defined literals

```cpp
using namespace endian::literals;

auto a = 0xDEADBEEF_le;    // basic_endian<uint64_t>
auto b = 0xDEADBEEF_be;    // basic_endian<uint64_t>
auto c = 0x1234_le16;      // basic_endian<uint16_t>
auto d = 0xCAFEBABE_be32;  // basic_endian<uint32_t>
```

---

## Batch operations

All batch functions operate on raw pointers. Separate source and destination pointers are assumed to be non-overlapping (declared with `__restrict` / `__restrict__`).

### Out-of-place swap

```cpp
#include <vector>

std::vector<uint32_t> src = {0x01020304, 0xDEADBEEF, 0xCAFEBABE};
std::vector<uint32_t> dst(src.size());

// Via the basic_endian static method (checks alignment by default)
endian::basic_endian<uint32_t>::convert_batch(src, dst, /*check_alignment=*/false);

// Or directly
endian::batch_byte_swap(src.data(), dst.data(), src.size(), false);
```

### In-place swap

```cpp
std::array<uint16_t, 8> data = {0x0102, 0x0304, 0x0506, 0x0708,
                                 0x090A, 0x0B0C, 0x0D0E, 0x0F10};

endian::batch_byte_swap_inplace(data.data(), data.size());
```

### Non-temporal stores

For large buffers where the data will not be reused immediately, `batch_byte_swap_nt` writes results with non-temporal (streaming) stores to avoid polluting the cache:

```cpp
std::vector<uint32_t> src(65536), dst(65536);
// ... fill src ...
endian::batch_byte_swap_nt(dst.data(), src.data(), src.size());
```

### Prefetch-assisted swap

When processing large arrays in a loop where the next cache lines can be prefetched ahead of time:

```cpp
endian::batch_byte_swap_prefetch(
    src.data(), dst.data(), src.size(),
    /*prefetch_distance=*/8  // elements to look ahead
);
```

SIMD dispatch happens automatically based on runtime CPU feature detection. On x86-64 machines with AVX2 support, 32-bit elements are processed 8 at a time and 64-bit elements 4 at a time; AVX-512 doubles those widths. On hardware without SIMD support the code falls back to scalar `byte_swap` element by element.

---

## Buffer view

`buffer_view` and `const_buffer_view` provide zero-copy access to a `std::byte` buffer, reading and writing values in any byte order without allocating memory.

```cpp
std::vector<std::byte> packet(64, std::byte{0});

// Write a big-endian 32-bit field at offset 4
{
    endian::basic_endian<uint32_t>::buffer_view view(
        std::span<std::byte>(packet.data() + 4, sizeof(uint32_t)));
    view.store_big(0xDEADBEEF);
}

// Read it back
{
    endian::basic_endian<uint32_t>::const_buffer_view view(
        std::span<const std::byte>(packet.data() + 4, sizeof(uint32_t)));
    uint32_t value = view.load_big();  // 0xDEADBEEF
}
```

---

## Network order helpers

`basic_endian<T>::network_order` provides named conversions for network (big-endian) byte order:

```cpp
using net = endian::basic_endian<uint32_t>::network_order;

uint32_t host_value    = 0x01020304;
uint32_t network_value = net::host_to_network(host_value);
uint32_t back          = net::network_to_host(network_value);
```

Matching helpers exist for `little_endian_order` and `big_endian_order`.

---

## Serialization and deserialization

`basic_endian<T>` can serialize itself to and from any stream that exposes `write` and `read` members (e.g. `std::ostream` / `std::istream`). Values are always written in big-endian order.

```cpp
#include <sstream>

endian::basic_endian<uint32_t> field(0x01020304);

std::ostringstream out;
field.serialize(out);    // writes 4 bytes, big-endian

std::istringstream in(out.str());
endian::basic_endian<uint32_t> restored;
restored.deserialize(in);
// restored.native() == 0x01020304

// Batch variant for arrays
std::vector<uint32_t> data = {1, 2, 3, 4};
endian::basic_endian<uint32_t>::serialize_batch(out, data.data(), data.size());
```

---

## MMIO (memory-mapped I/O)

`write_mmio` and `read_mmio` use `std::atomic_ref` for safe, ordered access to device registers. Both produce big-endian bytes on the wire.

```cpp
volatile uint32_t device_register = 0;

endian::basic_endian<uint32_t>::write_mmio(
    &device_register, 0xDEADBEEF,
    std::memory_order_release);

uint32_t current = endian::basic_endian<uint32_t>::read_mmio(
    &device_register,
    std::memory_order_acquire);
```

---

## endian_span

`endian_span<T, Order>` wraps a contiguous range of raw integers and iterates over them as `basic_endian<T>` values. The template parameter `Order` records what byte order the raw data is stored in.

```cpp
std::vector<uint32_t> raw_network_data = {/* big-endian bytes from network */};

endian::endian_span<uint32_t, endian::byte_order::big> span(
    raw_network_data.data(), raw_network_data.size());

for (auto& element : span)
{
    uint32_t host = element.native();  // converted on access
    // ...
}

// Convert the entire buffer in-place to host order
span.convert_to_host();
```

A read-only variant `const_endian_span` is available for immutable data.

---

## CPU feature detection

The library uses [simd_feature_check](https://github.com/korbolkoinc/simd_feature_check) for runtime SIMD capability queries. The API is accessible directly via `simd::detail::CPUInfo`:

```cpp
#include <simd/feature_check.hpp>

#if defined(__x86_64__) || defined(_M_X64)
if (simd::detail::CPUInfo::has_avx2())
    std::cout << "AVX2 is available\n";
#endif

#if defined(__AVX512F__)
if (simd::detail::CPUInfo::has_avx512f())
    std::cout << "AVX-512F is available\n";
#endif
```

ARM NEON availability is a compile-time property. Code compiled with NEON support (`-mfpu=neon` / target feature implied by the ARM ABI) will use NEON intrinsics unconditionally when the `#if defined(__ARM_NEON)` guard is satisfied.

---

## Utilities

The `detail` namespace exposes a few low-level helpers that are occasionally useful in performance-sensitive code:

- `cache_line_padded<T>` — stores a value padded to the hardware destructive-interference size to prevent false sharing between threads.
- `cache_aligned_storage<T, CacheLineSize>` — similar, with an explicit cache-line size parameter.
- `atomic_padded<T>` — padded `std::atomic<T>` with the same false-sharing protection.
- `prefetch_read(addr)` / `prefetch_write(addr)` — hints to the CPU to bring a cache line in before it is needed.
- `align_up<T>(ptr, alignment)` — rounds a pointer or integer up to the next multiple of `alignment`.
- `is_aligned<T>(ptr, alignment)` — tests alignment without undefined behaviour.

---

## Building from source

```bash
# Configure and build (debug profile)
cmake --preset debug
cmake --build --preset debug

# Run the test suite
ctest --preset debug

# Run a specific example
./build/debug/bin/basic_usage
./build/debug/bin/batch_operations
./build/debug/bin/network_protocol
```

CMake options:

| Option               | Default | Description                   |
|----------------------|---------|-------------------------------|
| `BUILD_TESTS`        | ON      | Build the GTest test suite    |
| `BUILD_EXAMPLES`     | ON      | Build example programs        |
| `BUILD_BENCHMARKS`   | OFF     | Build Google Benchmark suite  |
| `ENABLE_SANITIZERS`  | OFF     | Enable ASan/UBSan             |
| `ENABLE_LTO`         | OFF     | Enable link-time optimization |

---

## License

MIT. See [LICENSE](LICENSE).


A production-ready C++20 project template with modern CMake, CI/CD, and best practices already configured.

Skip the setup and start building.

</div>

---

## What's Included

**Build System**
- Modern CMake 3.21+ with presets, `PROJECT_IS_TOP_LEVEL`, and `CONFIGURE_DEPENDS`

**Language**
- C++20 standard with concepts, ranges, and coroutines support

**Testing**
- Google Test v1.15 with auto-discovery and GMock integration

**Benchmarking**
- Google Benchmark v1.9 for performance tracking

**Code Quality**
- Sanitizers: ASan, UBSan, TSan, MSan, LSan
- Static analyzers: clang-tidy, cppcheck, include-what-you-use

**Coverage**
- lcov/gcovr integration with HTML reports and branch coverage

**Performance**
- Link Time Optimization (LTO) and Precompiled Headers (PCH)

**Documentation**
- Doxygen setup with UML diagram support

**Packaging**
- CPack configured for DEB, RPM, NSIS, ZIP, and TGZ

**CI/CD**
- GitHub Actions with multi-platform, multi-compiler pipeline

**Installation**
- CMake export with `find_package()` support

**Code Formatting**
- Professional clang-format configuration (760+ lines)

**Symbol Visibility**
- Export headers for proper shared library support

## Requirements

**Required:**
- CMake 3.21+
- C++ Compiler: GCC 11+, Clang 14+, or MSVC 19.30+
- Git 2.x+

**Optional:**
- Doxygen — API documentation
- lcov or gcovr — Code coverage reports
- clang-tidy — Static analysis
- cppcheck — Additional static analysis

## Quick Start

### Using CMake Presets (Recommended)

```bash
# Clone the repository
git clone https://github.com/hun756/CPP-Starter-Template.git my-project
cd my-project

# Configure and build
cmake --preset debug
cmake --build --preset debug

# Run tests
ctest --preset debug

# Run the executable
./build/debug/bin/cpp_project_template
```

### Classic CMake

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -j$(nproc)
ctest --output-on-failure
```

## Project Structure

```
my-project/
├── .github/
│   ├── workflows/
│   │   └── ci.yml                 # Multi-platform CI pipeline
│   ├── dependabot.yml             # Automated dependency updates
│   └── ISSUE_TEMPLATE/            # Bug report & feature request templates
├── cmake/
│   ├── CompilerWarnings.cmake     # Strict warning flags (GCC/Clang/MSVC)
│   ├── Sanitizers.cmake           # Runtime error detection (ASan, UBSan, TSan)
│   ├── StaticAnalyzers.cmake      # Static analysis (cppcheck, clang-tidy, IWYU)
│   ├── LTO.cmake                  # Link Time Optimization
│   ├── Coverage.cmake             # Code coverage with lcov/gcovr
│   ├── Dependencies.cmake         # FetchContent dependency management
│   ├── Packaging.cmake            # CPack packaging configuration
│   └── configs/
│       ├── Config.h.in            # Version info template → ProjectConfig.h
│       └── *Config.cmake.in       # Package config for find_package()
├── include/myproject/             # Public headers (your API)
│   ├── ModuleA.h
│   └── ModuleB.h
├── src/                           # Implementation files
│   ├── ModuleA.cpp
│   ├── ModuleB.cpp
│   └── main.cpp
├── test/                          # Unit tests (auto-discovered)
│   ├── CMakeLists.txt
│   ├── ModuleATest.cpp
│   └── ModuleBTest.cpp
├── bench/                         # Performance benchmarks
│   ├── CMakeLists.txt
│   ├── CalculatorBench.cpp
│   └── StringProcessorBench.cpp
├── examples/                      # Usage examples (auto-discovered)
│   ├── CMakeLists.txt
│   └── example*.cpp
├── docs/                          # Doxygen documentation
│   ├── CMakeLists.txt
│   └── Doxyfile.in
├── .clang-format                  # Code formatting rules
├── .clang-tidy                    # Static analysis configuration
├── .editorconfig                  # Cross-editor formatting consistency
├── CMakeLists.txt                 # Main build configuration
├── CMakePresets.json              # Build presets (debug, release, CI)
├── CHANGELOG.md                   # Version history
├── CONTRIBUTING.md                # Contribution guidelines
├── CODE_OF_CONDUCT.md             # Community standards
├── LICENSE                        # MIT License
└── README.md
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_SHARED_LIBS` | `OFF` | Build shared libraries instead of static |
| `BUILD_EXAMPLES` | `ON` | Build example programs |
| `BUILD_TESTS` | `ON` | Build unit tests |
| `BUILD_BENCHMARKS` | `OFF` | Build performance benchmarks |
| `ENABLE_COVERAGE` | `OFF` | Enable code coverage instrumentation |
| `ENABLE_SANITIZERS` | `OFF` | Enable sanitizers (ASan, UBSan) in debug builds |
| `ENABLE_PCH` | `OFF` | Enable precompiled headers |
| `ENABLE_LTO` | `OFF` | Enable Link Time Optimization |
| `ENABLE_CPPCHECK` | `OFF` | Enable static analysis with cppcheck |
| `ENABLE_CLANG_TIDY` | `OFF` | Enable static analysis with clang-tidy |

Example — build with sanitizers and shared libraries:

```bash
cmake --preset debug -DBUILD_SHARED_LIBS=ON -DENABLE_SANITIZERS=ON
```

## Available Presets

| Preset | Description |
|--------|-------------|
| `debug` | Debug build with sanitizers enabled |
| `release` | Optimized release build with LTO |
| `relwithdebinfo` | Release with debug information |
| `coverage` | Debug build with coverage instrumentation |
| `ci-linux-gcc` | CI configuration for Linux + GCC |
| `ci-linux-clang` | CI configuration for Linux + Clang |
| `ci-windows-msvc` | CI configuration for Windows + MSVC |
| `ci-macos-clang` | CI configuration for macOS + AppleClang |

## Code Coverage

```bash
# Configure, build, and run tests with coverage
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage

# Generate HTML report
cmake --build --preset coverage --target coverage

# Open the report
open build/coverage/coverage_report/index.html    # macOS
xdg-open build/coverage/coverage_report/index.html # Linux
```

## Documentation

Generate API documentation with Doxygen:

```bash
cmake --preset debug
cmake --build --preset debug --target cpp_project_template_docs
```

## Packaging

Create distributable packages:

```bash
cmake --preset release
cmake --build --preset release
cd build/release && cpack
```

## Using This Library in Your Project

After installation:

```cmake
find_package(cpp_project_template REQUIRED)
target_link_libraries(your_target PRIVATE cpp_project_template::cpp_project_template)
```

Or via `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(
    cpp_project_template
    GIT_REPOSITORY https://github.com/hun756/CPP-Starter-Template.git
    GIT_TAG main
)
FetchContent_MakeAvailable(cpp_project_template)
target_link_libraries(your_target PRIVATE cpp_project_template::cpp_project_template)
```

## Customizing the Template

1. **Rename the project:** Change `cpp_project_template` in `CMakeLists.txt` to `project(your_name ...)`
2. **Rename namespace:** Replace `myproject` with your namespace in `include/` and `src/`
3. **Rename include dir:** Rename `include/myproject/` to `include/your_project/`
4. **Update Config:** Update `cmake/configs/cpp_project_templateConfig.cmake.in` filename
5. **Add source files:** Create `.cpp` files in `src/` — automatically picked up
6. **Add tests:** Create `*Test.cpp` files in `test/` — automatically discovered
7. **Add benchmarks:** Create `*Bench.cpp` files in `bench/` — automatically built
8. **Add examples:** Create `.cpp` files in `examples/` — automatically built

## Contributing

Contributions are welcome! Please read the [Contributing Guide](CONTRIBUTING.md) and [Code of Conduct](CODE_OF_CONDUCT.md) first.

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Commit with [Conventional Commits](https://www.conventionalcommits.org/): `git commit -m 'feat: add my feature'`
4. Push to the branch: `git push origin feature/my-feature`
5. Submit a pull request

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
