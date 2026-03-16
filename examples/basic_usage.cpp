// examples/basic_usage.cpp
//
// Demonstrates fundamental byte-order queries and scalar conversions
// using the byteorder library.
//
// Build & run:
//   cmake --preset debug && cmake --build --preset debug
//   ./build/debug/bin/basic_usage

#include "byteorder/byteorder.h"
#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>

using namespace endian;

static void print_endian_info()
{
    std::cout << "System byte order: "
              << (std::endian::native == std::endian::little
                      ? "little-endian"
                      : "big-endian")
              << "\nbyteorder version : " << version::str << "\n\n";
}

static void demo_uint32()
{
    std::cout << "--- uint32_t example ---\n";

    constexpr uint32_t host_value = 0x01020304;
    basic_endian<uint32_t> e(host_value);

    std::cout << std::hex << std::uppercase;
    std::cout << "host  (native): 0x" << e.native() << '\n';
    std::cout << "little-endian : 0x" << e.little() << '\n';
    std::cout << "big-endian    : 0x" << e.big()    << '\n';
    std::cout << std::dec;
}

static void demo_uint64_literals()
{
    using namespace endian::literals;
    std::cout << "\n--- user-defined literals ---\n";

    const auto le = 0xDEADBEEF_le;
    const auto be = 0xDEADBEEF_be;

    std::cout << std::hex << std::uppercase;
    std::cout << "0xDEADBEEF_le .native() = 0x" << le.native() << '\n';
    std::cout << "0xDEADBEEF_be .native() = 0x" << be.native() << '\n';
    std::cout << std::dec;
}

static void demo_signed_types()
{
    std::cout << "\n--- signed int32_t ---\n";

    le_int32 val(-1);
    std::cout << "native: " << val.native() << '\n';
    std::cout << std::hex;
    std::cout << "big  : 0x" << static_cast<uint32_t>(val.big()) << '\n';
    std::cout << std::dec;
}

int main()
{
    print_endian_info();
    demo_uint32();
    demo_uint64_literals();
    demo_signed_types();
    return 0;
}
